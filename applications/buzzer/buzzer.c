/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-06-25     Lee       the first version
 */


#include <rtthread.h>
#include <rtdevice.h>
#include <board.h>
#include "pv_event.h"


#define BUZZER_PIN      GET_PIN(G, 11)
#define BUZZER_VOICE_TIME       80          //蜂鸣器发声时间 毫秒ms

rt_thread_t tid_servo = RT_NULL;

void buzzer_thread_entry(void *parameter){

    while(1){
        rt_event_recv(global_event, EVENT_TOUCH_SCREEN, RT_EVENT_FLAG_AND|RT_EVENT_FLAG_CLEAR, RT_WAITING_FOREVER, RT_NULL);
        rt_pin_write(BUZZER_PIN, 1);
        rt_thread_mdelay(BUZZER_VOICE_TIME);
        rt_pin_write(BUZZER_PIN, 0);
    }

}

void buzzer_thread_init(){

    rt_pin_mode(BUZZER_PIN, PIN_MODE_OUTPUT);
    rt_pin_write(BUZZER_PIN, 0);

    tid_servo = rt_thread_create("buzzer",
                                buzzer_thread_entry, RT_NULL,
                                256, 20, 5);
    if(tid_servo != RT_NULL){
        rt_thread_startup(tid_servo);
    }
}
