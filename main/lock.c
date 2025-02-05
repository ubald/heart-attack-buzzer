#include "lock.h"
#include "config.h"
#include "esp_log.h"
#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>

static const char *TAG = "Lock";

void lockInit(void)
{
    ESP_LOGI(TAG, "Initializing lock.");

    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << LOCK_GPIO,
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf);
    gpio_set_level(LOCK_GPIO, 0);
}

int lockSetState(bool unlock)
{
    if (unlock) {
        ESP_LOGI(TAG, "Unlocking");
        gpio_set_level(LOCK_GPIO, 1);
        vTaskDelay(125);
    }

    ESP_LOGI(TAG, "Locking");
    gpio_set_level(LOCK_GPIO, 0);

    return 1;
}
