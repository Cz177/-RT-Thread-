/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: MIT
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-10-18     Meco Man     the first version
 * 2022-05-10     Meco Man     improve rt-thread initialization process
 */

#ifdef __RTTHREAD__

#include <lvgl.h>
#include "lv_port_indev.h"
#include "lv_port_disp.h"
#include <rtthread.h>
#include "touch.h"
#include "ui.h"

#define DBG_TAG    "LVGL"
#define DBG_LVL    DBG_INFO
#include <rtdbg.h>

rt_device_t touch_dev = RT_NULL;

#ifndef PKG_LVGL_THREAD_STACK_SIZE
#define PKG_LVGL_THREAD_STACK_SIZE 4096
#endif /* PKG_LVGL_THREAD_STACK_SIZE */

#ifndef PKG_LVGL_THREAD_PRIO
#define PKG_LVGL_THREAD_PRIO (RT_THREAD_PRIORITY_MAX*2/3)
#endif /* PKG_LVGL_THREAD_PRIO */


static struct rt_thread lvgl_thread;

#ifdef rt_align
rt_align(RT_ALIGN_SIZE)
#else
ALIGN(RT_ALIGN_SIZE)
#endif
static rt_uint8_t lvgl_thread_stack[PKG_LVGL_THREAD_STACK_SIZE];

#if LV_USE_LOG
static void lv_rt_log(const char *buf)
{
    LOG_I(buf);
}
#endif /* LV_USE_LOG */

static void lvgl_thread_entry(void *parameter)
{
#if LV_USE_LOG
    lv_log_register_print_cb(lv_rt_log);
#endif /* LV_USE_LOG */
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();
//    lv_demo_music();
    ui_init();

    /* handle the tasks of LVGL */
    while(1)
    {
        lv_task_handler();
        rt_thread_mdelay(LV_DISP_DEF_REFR_PERIOD);
    }
}

static int lvgl_thread_init(void)
{
    touch_dev = rt_device_find("gt1151");
    if(touch_dev == RT_NULL){
        LOG_E("Failed to find GT1151");
        return -1;
    }
    rt_device_open(touch_dev, RT_DEVICE_FLAG_RDONLY);

     /* 读 ID */
     rt_uint8_t read_id[4];
     rt_device_control(touch_dev, RT_TOUCH_CTRL_GET_ID, read_id);
     LOG_I("id = %d %d %d %d", read_id[0] - '0', read_id[1] - '0', read_id[2] - '0', read_id[3] - '0');

     /* 获取设备信息 */
     struct rt_touch_info info;
     rt_device_control(touch_dev, RT_TOUCH_CTRL_GET_INFO, &info);
     LOG_I("point_support_num  :%d", info.point_num);                  /* 支持的触点个数 */
     LOG_I("range_x    :%d", info.range_x);                    /* X 轴分辨率 */
     LOG_I("range_y    :%d", info.range_y);                  /* Y 轴分辨率*/

     /* 设置工作模式为轮询模式 */
     rt_device_control(touch_dev, RT_TOUCH_CTRL_SET_MODE, (void *)RT_DEVICE_FLAG_RDONLY);

    rt_err_t err;
    err = rt_thread_init(&lvgl_thread,
                       "LVGL",
                       lvgl_thread_entry,
                       RT_NULL,
                       &lvgl_thread_stack[0],
                       sizeof(lvgl_thread_stack),
                       PKG_LVGL_THREAD_PRIO, 10);
    if(err != RT_EOK)
    {
        LOG_E("Failed to create LVGL thread");
        return -1;
    }
    rt_thread_startup(&lvgl_thread);

    return 0;
}
INIT_ENV_EXPORT(lvgl_thread_init);

#endif /*__RTTHREAD__*/
