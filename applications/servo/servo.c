/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-06-22     Lee       the first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "pv_event.h"

#define SERVO_PWM_PERIOD        20000000
#define SERVO1_OPEN_PWM         1625000
#define SERVO1_CLOSE_PWM        500000
#define SERVO23_OPEN_PWM        500000
#define SERVO23_CLOSE_PWM       1625000

#define SERVO12_PWM_DEV_NAME        "pwm4"  /* PWM设备名称 */
#define SERVO3_PWM_DEV_NAME        "pwm2"  /* PWM设备名称 */

#define SERVO1_DEV_CHANNEL     4       /* 舵机1PWM通道 */
#define SERVO2_DEV_CHANNEL     3       /* 舵机2PWM通道 */
#define SERVO3_DEV_CHANNEL     2       /* 舵机3PWM通道 */

struct rt_device_pwm *servo_pwm_dev1;      /* 舵机1&2PWM设备句柄 */
struct rt_device_pwm *servo_pwm_dev3;      /* 舵机3PWM设备句柄 */

rt_mq_t servo_mq = RT_NULL;          //舵机消息队列句柄

static rt_thread_t servo_thread = RT_NULL;

void servo_trhread_entry(void *parameter){
    rt_uint8_t layer = 0;
    struct rt_device_pwm *pwm_dev = RT_NULL;
    int channel = 0;
    rt_uint32_t open_pwm = 0, close_pwm = 0;
    rt_uint32_t e;

    //初始化舵机，关闭取件门
    rt_pwm_set(servo_pwm_dev1, SERVO1_DEV_CHANNEL, SERVO_PWM_PERIOD, SERVO1_CLOSE_PWM);
    rt_pwm_enable(servo_pwm_dev1, SERVO1_DEV_CHANNEL);
    rt_pwm_set(servo_pwm_dev1, SERVO2_DEV_CHANNEL, SERVO_PWM_PERIOD, SERVO23_CLOSE_PWM);
    rt_pwm_enable(servo_pwm_dev1, SERVO2_DEV_CHANNEL);
    rt_pwm_set(servo_pwm_dev3, SERVO3_DEV_CHANNEL, SERVO_PWM_PERIOD, SERVO23_CLOSE_PWM);
    rt_pwm_enable(servo_pwm_dev3, SERVO3_DEV_CHANNEL);

    while(1){
        pwm_dev = RT_NULL;
        channel = 0;
        //等待发送层数信息，用以控制哪个舵机
        if(rt_mq_recv(servo_mq, &layer, sizeof(layer), RT_WAITING_FOREVER) == RT_EOK){
            rt_kprintf("levle: %d\n",layer);
        }
        //等待电机PID完成位置选取
        rt_event_recv(global_event, EVENT_POSITION_OK, (RT_EVENT_FLAG_AND|RT_EVENT_FLAG_CLEAR), RT_WAITING_FOREVER, &e);
        rt_kprintf("servo_start");
        if(layer == 0){
            pwm_dev = servo_pwm_dev1;
            channel = SERVO1_DEV_CHANNEL;
            open_pwm = SERVO1_OPEN_PWM;
            close_pwm = SERVO1_CLOSE_PWM;
        }else if(layer == 1) {
            pwm_dev = servo_pwm_dev1;
            channel = SERVO2_DEV_CHANNEL;
            open_pwm = SERVO23_OPEN_PWM;
            close_pwm = SERVO23_CLOSE_PWM;
        }else if(layer == 2){
            pwm_dev = servo_pwm_dev3;
            channel = SERVO3_DEV_CHANNEL;
            open_pwm = SERVO23_OPEN_PWM;
            close_pwm = SERVO23_CLOSE_PWM;
        }

        rt_pwm_set(pwm_dev, channel, SERVO_PWM_PERIOD, open_pwm);    //开启取件门
        rt_event_recv(global_event, EVENT_PICK_FINNISH, (RT_EVENT_FLAG_AND|RT_EVENT_FLAG_CLEAR), RT_WAITING_FOREVER, &e);   //等待完成取件，由LVGL按键发出
        rt_pwm_set(pwm_dev, channel, SERVO_PWM_PERIOD, close_pwm);     //关闭取件门
        rt_thread_delay(20);
    }

}

void servo_thread_init(void){
    //舵机消息队列，接收层数
    servo_mq = rt_mq_create("S_mq", sizeof(rt_uint8_t), 5, RT_IPC_FLAG_PRIO);
    if(servo_mq == RT_NULL){
         rt_kprintf("servo mq create failed\n");
         return;
    }

    //查询舵机设备
    servo_pwm_dev1 = (struct rt_device_pwm *)rt_device_find(SERVO12_PWM_DEV_NAME);
    if (servo_pwm_dev1 == RT_NULL){
        rt_kprintf("can't find %s device!\n", SERVO12_PWM_DEV_NAME);
        return;
    }
    servo_pwm_dev3 = (struct rt_device_pwm *)rt_device_find(SERVO3_PWM_DEV_NAME);
    if (servo_pwm_dev3 == RT_NULL){
        rt_kprintf("can't find %s device!\n", SERVO3_PWM_DEV_NAME);
        return;
    }

    //创建舵机线程
    servo_thread = rt_thread_create("servo",
                                servo_trhread_entry, RT_NULL,
                                512, 11, 10);
    if(servo_thread != RT_NULL){
        rt_thread_startup(servo_thread);
    }
}

