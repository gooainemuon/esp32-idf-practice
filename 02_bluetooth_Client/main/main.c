/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* Includes */
#include <stdio.h>    // sprintf 용
#include <string.h>   // strlen 용
#include "host/ble_gatt.h" // ble_gattc_write_flat 용
#include "common.h"
#include "gap.h"
#include "led.h"
//통신정보 저장
extern uint16_t connection_handle;
extern bool is_connected;

/* Library function declarations */
void ble_store_config_init(void);

/* Private function declarations */
static void on_stack_reset(int reason);
static void on_stack_sync(void);
static void nimble_host_config_init(void);
static void nimble_host_task(void *param);

#define BUT_PIN 0

int candidate_pins[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

void button_task(void *pvParameters)
{
    int num_pins = sizeof(candidate_pins) / sizeof(int);
    int last_states[num_pins];

    // 모든 후보 핀을 입력(Pull-up) 모드로 설정
    for (int i = 0; i < num_pins; i++) {
        gpio_reset_pin(candidate_pins[i]);
        gpio_set_direction(candidate_pins[i], GPIO_MODE_INPUT);
        gpio_set_pull_mode(candidate_pins[i], GPIO_PULLUP_ONLY);
        last_states[i] = gpio_get_level(candidate_pins[i]);
    }

    while (1) {
        for (int i = 0; i < num_pins; i++) {
            int current_state = gpio_get_level(candidate_pins[i]);
            
            // 상태가 변했을 때 전송
            if (current_state != last_states[i]) {
                char buf[100];
                int msg_len = sprintf(buf, "[알림] GPIO %d번 상태가 %d로 변함!", 
                      candidate_pins[i], current_state);

                printf("[알림] GPIO %d 번 핀의 상태가 %d 로 변했습니다!\n", 
                       candidate_pins[i], current_state);
                
                if (is_connected){
                    ble_gattc_write_flat(connection_handle, 1 , buf, (uint16_t)msg_len, NULL, NULL);
                }
                last_states[i] = current_state;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // 10ms마다 스캔
    }
}

static void on_stack_reset(int reason) {
    /* On reset, print reset reason to console */
    ESP_LOGI(TAG, "nimble stack reset, reset reason: %d", reason);
}

static void on_stack_sync(void) {
    /* On stack sync, do advertising initialization */
    adv_init();
}

static void nimble_host_config_init(void) {
    /* Set host callbacks */
    ble_hs_cfg.reset_cb = on_stack_reset;
    ble_hs_cfg.sync_cb = on_stack_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    /* Store host configuration */
    ble_store_config_init();
}

static void nimble_host_task(void *param) {
    /* Task entry log */
    ESP_LOGI(TAG, "nimble host task has been started!");


    /* This function won't return until nimble_port_stop() is executed */
    nimble_port_run();

    /* Clean up at exit */
    vTaskDelete(NULL);
}

void app_main(void) {
    /* Local variables */
    int rc = 0;
    esp_err_t ret = ESP_OK;

    /* LED initialization */
    led_init();

    /* NVS flash initialization */
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed to initialize nvs flash, error code: %d ", ret);
        return;
    }

    /* Button initialization */
    xTaskCreate(button_task, "button_task", 4096, NULL, 5, NULL);

    /* NimBLE stack initialization */
    ret = nimble_port_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "failed to initialize nimble stack, error code: %d ",
                 ret);
        return;
    }

#if CONFIG_BT_NIMBLE_GAP_SERVICE
    /* GAP service initialization */
    rc = gap_init();
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to initialize GAP service, error code: %d", rc);
        return;
    }
#endif

    /* NimBLE host configuration initialization */
    nimble_host_config_init();

    /* Start NimBLE host task thread and return */
    xTaskCreate(nimble_host_task, "NimBLE Host", 4*1024, NULL, 5, NULL);
    return;
}
