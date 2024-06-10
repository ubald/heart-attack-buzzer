#include "accessory.h"

#include "config.h"
#include "lock.h"

//#include <stdio.h>
#include <string.h>
#include <esp_log.h>

#include <hap.h>
#include <hap_apple_servs.h>
#include <hap_apple_chars.h>
//#include <hap_fw_upgrade.h>

#include <iot_button.h>

#include <app_wifi.h>
#include <app_hap_setup_payload.h>

#define HAP_SERV_UUID_DOORBELL "00000121-0000-1000-8000-0026BB765291"
#define HAP_CHAR_LOCK_CURRENT_STATE_UNSECURED 0
#define HAP_CHAR_LOCK_CURRENT_STATE_SECURED 1
#define HAP_CHAR_LOCK_CURRENT_STATE_JAMMED 2
#define HAP_CHAR_LOCK_CURRENT_STATE_UNKNOWN 3
#define HAP_CHAR_LOCK_TARGET_STATE_UNSECURED 0
#define HAP_CHAR_LOCK_TARGET_STATE_SECURED 1

static const char *TAG = "HAP Buzzer";

/**
 * Mandatory identify routine for the accessory.
 * In a real accessory, something like LED blink should be implemented
 * got visual identification
 */
static int identify(/*hap_acc_t *ha*/) {
    ESP_LOGI(TAG, "Accessory identified");
    return HAP_SUCCESS;
}

/**
 * @brief The network reset button callback handler.
 * Useful for testing the Wi-Fi re-configuration feature of WAC2
 */
static void reset_network_handler(/*void *arg*/) {
    ESP_LOGI(TAG, "Resetting network");
    hap_reset_network();
}

/**
 * @brief The factory reset button callback handler.
 */
static void reset_to_factory_handler(/*void *arg*/) {
    ESP_LOGI(TAG, "Resetting to factory");
    hap_reset_to_factory();
}

/**
 * Callback for handling writes on the Light Bulb Service
 */
static int lockWrite(hap_write_data_t write_data[], int count, void *serv_priv, void *write_priv) {
    int              i;
    int              ret = HAP_SUCCESS;
    hap_write_data_t *write;

    for (i = 0; i < count; i++) {
        write = &write_data[i];

        // Setting a default error value
        *(write->status) = HAP_STATUS_VAL_INVALID;

        if (!strcmp(hap_char_get_type_uuid(write->hc), HAP_CHAR_UUID_LOCK_TARGET_STATE)) {
            ESP_LOGI(TAG, "Received Write for Lock Target State %d", write->val.i);
            int state = lockSetState(write->val.i == HAP_CHAR_LOCK_TARGET_STATE_UNSECURED);
            write->val.i = state == 1 ? HAP_CHAR_LOCK_TARGET_STATE_SECURED: HAP_CHAR_LOCK_TARGET_STATE_UNSECURED;
            ESP_LOGI(TAG, "Confirming Lock Target State %d", write->val.i);
            *(write->status) = HAP_STATUS_SUCCESS;
        } else {
            *(write->status) = HAP_STATUS_RES_ABSENT;
        }

        // If the characteristic write was successful, update it in hap core
        if (*(write->status) == HAP_STATUS_SUCCESS) {
            hap_char_update_val(write->hc, &(write->val));
        } else {
            // Else, set the return value appropriately to report error
            ret = HAP_FAIL;
        }
    }
    return ret;
}

static void ringHandler(void *arg) {
    hap_acc_t  *acc       = hap_get_first_acc();
    hap_serv_t *serv      = hap_acc_get_serv_by_uuid(acc, HAP_SERV_UUID_DOORBELL);
    hap_char_t *ring_char = hap_serv_get_char_by_uuid(serv, HAP_CHAR_UUID_PROGRAMMABLE_SWITCH_EVENT);
    //hap_char_t *ring_char = hap_serv_get_char_by_uuid(serv, HAP_CHAR_UUID_CONTACT_SENSOR_STATE);
    //hap_char_t *ring_char = hap_serv_get_char_by_uuid(serv, HAP_CHAR_UUID_ACTIVE);
    //hap_char_t *ring_char = hap_serv_get_char_by_uuid(serv, HAP_CHAR_UUID_STATUS_ACTIVE);
    ESP_LOGI(TAG, "Ding dong!");
    //hap_val_t appliance_value = {
    //    //.b = true,
    //    .i = HAP_CHAR_LOCK_TARGET_STATE_UNSECURED,
    //};
    ////appliance_value.b = gpio_get_level(RING_GPIO);
    //int       ret             = hap_char_update_val(ring_char, &appliance_value);
    //if (ret != HAP_SUCCESS) {
    //    ESP_LOGE(TAG, "Failed to set value");
    //}
}

/**
 * The main thread for handling the Light Bulb Accessory
 */
void accessory_thread_entry(void *arg) {
    ESP_LOGI(TAG, "Booting buzzer");

    // Initialize the HAP core
    hap_init(HAP_TRANSPORT_WIFI);

    // Initialize the mandatory parameters for the accessory which will be
    // added as the mandatory services internally.
    hap_acc_cfg_t cfg = {
        .name = "Kondo Buzzer",
        .manufacturer = "Ubald",
        .model = "Deluxe",
        .serial_num = "1234567890",
        .fw_rev = "0.9.0",
        .hw_rev = "1.0",
        .pv = "1.1.0",
        .identify_routine = identify,
        .cid = HAP_CID_VIDEO_DOORBELL,
    };

    // Create accessory object
    hap_acc_t *accessory = hap_acc_create(&cfg);
    if (!accessory) {
        ESP_LOGE(TAG, "Failed to create accessory");
        goto accessory_error;
    }

    // Add a dummy Product Data
    uint8_t productData[] = {'E', 'S', 'P', '3', '2', 'H', 'A', 'P'};
    hap_acc_add_product_data(accessory, productData, sizeof(productData));

    // Add Wi-Fi Transport service required for HAP Spec R16
    hap_acc_add_wifi_transport_service(accessory, 0);

    {
        // Doorbell service
        hap_serv_t *service = hap_serv_create(HAP_SERV_UUID_DOORBELL);
        if (!service) {
            ESP_LOGE(TAG, "Failed to create accessory service");
            goto accessory_error;
        }

        // Doorbell button characteristic
        int ret = hap_serv_add_char(service, hap_char_programmable_switch_event_create(0));
        ret |= hap_serv_add_char(service, hap_char_name_create("Doorbell"));
        if (ret != HAP_SUCCESS) {
            ESP_LOGE(TAG, "Failed to add optional characteristics to Accessory");
            goto accessory_error;
        }
        hap_acc_add_serv(accessory, service);
    }

    {
        // Lock service
        hap_serv_t *service = hap_serv_lock_mechanism_create(
            HAP_CHAR_LOCK_CURRENT_STATE_SECURED,
            HAP_CHAR_LOCK_TARGET_STATE_SECURED
        );
        if (!service) {
            ESP_LOGE(TAG, "Failed to create lock service");
            goto accessory_error;
        }

        int ret = hap_serv_add_char(service, hap_char_name_create("Lock"));
        if (ret != HAP_SUCCESS) {
            ESP_LOGE(TAG, "Failed to add optional characteristics to Accessory");
            goto accessory_error;
        }

        hap_serv_set_write_cb(service, lockWrite);
        hap_acc_add_serv(accessory, service);

        // Register the ring button
        button_handle_t ringHandle = iot_button_create(RING_GPIO, BUTTON_ACTIVE_LOW);
        iot_button_set_evt_cb(ringHandle, BUTTON_CB_PUSH, ringHandler, service);
        iot_button_set_serial_cb(ringHandle, 1, 100, ringHandler, service);
    }

    hap_add_accessory(accessory);

    lockInit();

    // Register a common button for reset Wi-Fi network and reset to factory.
    button_handle_t resetHandle = iot_button_create(RESET_GPIO, BUTTON_ACTIVE_LOW);
    iot_button_add_on_release_cb(resetHandle, RESET_NETWORK_BUTTON_TIMEOUT, reset_network_handler, NULL);
    iot_button_add_on_press_cb(resetHandle, RESET_TO_FACTORY_BUTTON_TIMEOUT, reset_to_factory_handler, NULL);

#ifdef CONFIG_EXAMPLE_USE_HARDCODED_SETUP_CODE
    hap_set_setup_code(CONFIG_EXAMPLE_SETUP_CODE);
    hap_set_setup_id(CONFIG_EXAMPLE_SETUP_ID);
#ifdef CONFIG_APP_WIFI_USE_WAC_PROVISIONING
    app_hap_setup_payload(CONFIG_EXAMPLE_SETUP_CODE, CONFIG_EXAMPLE_SETUP_ID, true, cfg.cid);
#else
    app_hap_setup_payload(CONFIG_EXAMPLE_SETUP_CODE, CONFIG_EXAMPLE_SETUP_ID, false, cfg.cid);
#endif
#endif

    hap_enable_mfi_auth(HAP_MFI_AUTH_HW);
    app_wifi_init();
    hap_start();
    app_wifi_start(portMAX_DELAY);
    vTaskDelete(NULL);

    accessory_error:
    hap_acc_delete(accessory);
    vTaskDelete(NULL);
}
