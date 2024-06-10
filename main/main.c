#include <freertos/FreeRTOS.h>
#include "accessory.h"

#define ACCESSORY_TASK_PRIORITY  1
#define ACCESSORY_TASK_STACKSIZE (4 * 1024)
#define ACCESSORY_TASK_NAME "hap_buzzer"

void app_main() {
    xTaskCreate(
        accessory_thread_entry,
        ACCESSORY_TASK_NAME,
        ACCESSORY_TASK_STACKSIZE,
        NULL,
        ACCESSORY_TASK_PRIORITY,
        NULL
    );
}
