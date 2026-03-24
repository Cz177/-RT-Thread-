/*
 * Copyright (c) 2006-2022, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2022-02-01     Rudy Lo      The first version
 * 2022-06-20     Rbb666       Add GT9147 Touch Device
 */

#include <lvgl.h>
#include "touch.h"

extern rt_device_t touch_dev;
static void touchpad_read(lv_indev_drv_t *indev, lv_indev_data_t *data)
{
    static lv_coord_t last_x = 0;
    static lv_coord_t last_y = 0;

    struct rt_touch_data touch_data;
    rt_memset(&touch_data, 0, sizeof(touch_data));

    int read_num = rt_device_read(touch_dev, 0, &touch_data, 1);

    if ((read_num > 0) && (touch_data.event == RT_TOUCH_EVENT_DOWN))//有触点并且事件为按下
    {
        last_x = touch_data.x_coordinate;
        last_y = touch_data.y_coordinate;
        data->state = LV_INDEV_STATE_PR;
        //rt_kprintf("\t x: %d, y: %d\n", touch_data.x_coordinate, touch_data.y_coordinate);
    }
    else// 没有触点，表示释放
    {
        data->state = LV_INDEV_STATE_REL;
    }

    data->point.x = last_x;
    data->point.y = last_y;
}


lv_indev_t * indev_touchpad;

void lv_port_indev_init(void)
{
    static lv_indev_drv_t indev_drv; /* Descriptor of a input device driver */
    lv_indev_drv_init(&indev_drv); /* Basic initialization */
    indev_drv.type = LV_INDEV_TYPE_POINTER; /* Touch pad is a pointer-like device */
    indev_drv.read_cb = touchpad_read; /* Set your driver function */

    /* Register the driver in LVGL and save the created input device object */
    indev_touchpad = lv_indev_drv_register(&indev_drv);
}
