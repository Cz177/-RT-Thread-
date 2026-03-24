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
#include "board.h"
#include "pv_event.h"

#define MOTOR_PWM_PERIOD           1000000      //电机PWM周期1ms
#define MOTOR_PWM_DEV_NAME         "pwm15"      //电机PWM设备名称
#define MOTOR_PWM_CHANNEL           2            //TIM15 CH2
#define MOTOR_IN1_PIN              GET_PIN(H,2) //电机方向控制引脚IN1
#define MOTOR_IN2_PIN              GET_PIN(H,3) //电机方向控制引脚IN2

#define PULSE_ENCODER_DEV_NAME    "pulse3"    //脉冲编码器名称

#define HALL_PIN                    GET_PIN(A,15)
#define ZERO_OUT_ENCODER_COUNT_THRESHOLD            600

rt_mq_t motor_mq = RT_NULL;         //编码电机消息队列句柄
rt_device_t encoder_dev = RT_NULL;   // 脉冲编码器设备句柄
struct rt_device_pwm *motor_pwm_dev = RT_NULL; // 电机PWM设备句柄

static rt_thread_t encoder_motor_thread = RT_NULL;

void motor_set_pwm(int32_t pwm_val);
void motor_zero_out();

void encodermotor_entry(void *parameter){
    rt_uint32_t count;
    rt_int32_t pwm;
    rt_uint8_t pos;
    //PID 参数
    float_t Kp = 0.35, Ki = 0.05, Kd = 0;
    float_t integral = 0;
    rt_int32_t target = 0, actual = 0, error = 0, error_prev = 0;
    uint8_t t;

    while(1){

        //等待坐标信息
        if(rt_mq_recv(motor_mq, &pos, sizeof(pos), RT_WAITING_FOREVER) == RT_EOK){
            target = pos*290;
            rt_kprintf("target :%d\n", target);
        }

        //进入PID控制循环
        while(1){
            rt_device_read(encoder_dev, 0, &count, 1);  // 读取脉冲编码器计数值
            rt_device_control(encoder_dev, PULSE_ENCODER_CMD_CLEAR_COUNT, RT_NULL); //清空脉冲编码器计数值

            //PID计算
            actual += count;
            error_prev = error;
            error = target - actual;
            rt_kprintf("error: %d",error);
            if(abs(error) < 5){
                if(++t == 10){              //PID控制位置OK
                    t = 0;
                    motor_set_pwm(0);
                    rt_event_send(global_event, EVENT_POSITION_OK);
                    error = error_prev = 0;
                    integral = 0;
                    rt_kprintf("postion ok\n");
                    break;
                }
            }else {
                t = 0;
            }
            if(abs(error) < 30)
               integral += error;
            else
               integral = 0;

            if(integral > 1000) integral = 1000;
            if(integral < -1000) integral = -1000;
            pwm = (int32_t)(Kp*error + Ki*integral + Kd*(error - error_prev));

            if(pwm>30)
               pwm = 30;
            else if(pwm<-30)
               pwm = -30;
            motor_set_pwm(pwm);

            rt_thread_mdelay(20);
        }
        rt_event_recv(global_event, EVENT_PICK_FINNISH, RT_EVENT_FLAG_AND, RT_WAITING_FOREVER, NULL);
        rt_thread_delay(20);
    }
}

void encoder_thread_init(void)
{
    rt_err_t res = RT_EOK;
    //电机消息队列，接收存储位置信息
    motor_mq = rt_mq_create("M_mq", sizeof(uint8_t), 5, RT_IPC_FLAG_PRIO);
    if(motor_mq == RT_NULL){
        rt_kprintf("motor_mq create failed\n");
        return;
    }

    //查询编码器设备
    encoder_dev = rt_device_find(PULSE_ENCODER_DEV_NAME);
    if (encoder_dev == RT_NULL){
       rt_kprintf("can't find %s device!\n", PULSE_ENCODER_DEV_NAME);
       return;
    }

    //以只读方式打开编码器设备
    res = rt_device_open(encoder_dev, RT_DEVICE_OFLAG_RDONLY);
    if (res != RT_EOK){
       rt_kprintf("open %s device failed! error code:%d\n", PULSE_ENCODER_DEV_NAME, res);
       return;
    }

    //查询PWM设备
    motor_pwm_dev = (struct rt_device_pwm *)rt_device_find(MOTOR_PWM_DEV_NAME);
    if(motor_pwm_dev == RT_NULL){
        rt_kprintf("can't find %s device!\n", MOTOR_PWM_DEV_NAME);
        return;
    }

    //设置初始占空比为0 使能PWM设备
    rt_pwm_set(motor_pwm_dev, MOTOR_PWM_CHANNEL, MOTOR_PWM_PERIOD, 0);
    rt_pwm_enable(motor_pwm_dev, MOTOR_PWM_CHANNEL);

    //设置电机方向控制引脚
    rt_pin_mode(MOTOR_IN1_PIN, PIN_MODE_OUTPUT);
    rt_pin_mode(MOTOR_IN2_PIN, PIN_MODE_OUTPUT);
    //设置霍尔传感器引脚
    rt_pin_mode(HALL_PIN,PIN_MODE_INPUT_PULLUP);

    //电机归零
    motor_zero_out();

    //创建编码电机线程
    encoder_motor_thread = rt_thread_create("motor",
                                    encodermotor_entry, RT_NULL,
                                    1024, 10, 10);

    if(encoder_motor_thread != RT_NULL){
        rt_thread_startup(encoder_motor_thread);
    }

}

void motor_set_pwm(int32_t pwm_val){
    int32_t pulse;
    if(pwm_val > 100){
        pwm_val = 100;
    }else if(pwm_val < -100){
        pwm_val = -100;
    }
    if(pwm_val >= 0){
        rt_pin_write(MOTOR_IN1_PIN, 1);
        rt_pin_write(MOTOR_IN2_PIN, 0);
        pulse = pwm_val*10000;

    }else {
        rt_pin_write(MOTOR_IN1_PIN, 0);
        rt_pin_write(MOTOR_IN2_PIN, 1);
        pulse = (-pwm_val)*10000;
    }
    if(motor_pwm_dev != RT_NULL){
        rt_pwm_set(motor_pwm_dev, MOTOR_PWM_CHANNEL, MOTOR_PWM_PERIOD, pulse);
    }
}

void motor_zero_out(){      //归零
    rt_uint32_t count;
    static uint8_t pin_state=0;
    static uint8_t pin_state_prev=0;

    motor_set_pwm(30);
    while(1){

        pin_state_prev = pin_state;
        pin_state = rt_pin_read(HALL_PIN);
        if((pin_state==PIN_HIGH) && (pin_state_prev==PIN_LOW)){
            /* 读取脉冲编码器计数值 */
            rt_device_read(encoder_dev, 0, &count, 1);
            /* 清空脉冲编码器计数值 */
            rt_device_control(encoder_dev, PULSE_ENCODER_CMD_CLEAR_COUNT, RT_NULL);
            rt_kprintf("zero out encoder count :%d\n", count);
            if(count > ZERO_OUT_ENCODER_COUNT_THRESHOLD){
              motor_set_pwm(0);
              break;
          }
        }
       rt_thread_mdelay(20);
    }
}

