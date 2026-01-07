/*
1. 입력핀들 정의
2. 해당 핀의 개수만큼 배열생성하고 그 배열값에 안눌린 상태를 저장
3. 핀값을 받음
4. 만약 핀값이 안눌린 상태로 저장된 배열값과 비교해 다르면 알림
5. 그리고 바뀐값을 저장하고 다시 상태 비교
*/

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

void button_finder_task(void *pvParameters);
int candidate_pins[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

void app_main(void)
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
            
            // 상태가 변했을 때만 출력
            if (current_state != last_states[i]) {
                printf("[알림] GPIO %d 번 핀의 상태가 %d 로 변했습니다!\n", 
                       candidate_pins[i], current_state);
                last_states[i] = current_state;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // 10ms마다 스캔
    }
}

