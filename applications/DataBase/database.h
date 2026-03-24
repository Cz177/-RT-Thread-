/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-05-25     Lee       the first version
 */
#ifndef APPLICATIONS_DATABASE_DATABASE_H_
#define APPLICATIONS_DATABASE_DATABASE_H_

#include "flashdb.h"
#include "cJSON.h"
#include "rtthread.h"

#define MAX_COMPONENT_INFO_LEN          128
#define MAX_FILTER_INFO_LEN             16


typedef enum{
    COMPONENTS_RESRISTOR,
    COMPONENTS_CAPACITOR,
    COMPONENTS_INDUCTOR,
}eCOMPONENTS_TYPE;

typedef struct{
    char para_info[MAX_COMPONENT_INFO_LEN];
    uint32_t quantity;
}component_info;

typedef component_info *component_info_t;

//typedef struct {
//    char type[MAX_FILTER_INFO_LEN];
//    char val[MAX_FILTER_INFO_LEN];
//    char package[MAX_FILTER_INFO_LEN];
//    char ratedval[MAX_FILTER_INFO_LEN];
//    char tolerance[MAX_FILTER_INFO_LEN];
//    char otherinfo[MAX_FILTER_INFO_LEN];
//}filter_info;

typedef struct {
    char type[MAX_FILTER_INFO_LEN];
    const char *val;
    const char *package;
    const char *ratedval;
    const char *tolerance;
    const char *otherinfo;
}filter_info;

typedef filter_info *filter_info_t;

extern struct fdb_kvdb components_kv_db;

int database_init(void);
int components_add_kv(char *jsoninfo, uint32_t quantity);
int components_find_kv(filter_info_t filter);



#endif /* APPLICATIONS_DATABASE_DATABASE_H_ */
