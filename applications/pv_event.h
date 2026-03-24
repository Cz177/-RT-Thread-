/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-06-24     Lee       the first version
 */
#ifndef APPLICATIONS_PV_EVENT_H_
#define APPLICATIONS_PV_EVENT_H_

#define EVENT_PICK_FINNISH         (1 << 2)
#define EVENT_POSITION_OK          (1 << 3)
#define EVENT_TOUCH_SCREEN         (1 << 4)
#define EVENT_ZERO_OK              (1 << 5)

extern rt_event_t global_event;

#endif /* APPLICATIONS_PV_EVENT_H_ */
