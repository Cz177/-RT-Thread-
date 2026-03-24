/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2025-05-25     Lee       the first version
 */

#include "stdlib.h"
#include "database.h"
#include "lvgl.h"

#define FDB_LOG_TAG "[kvdb][components_db]"

#define MAX_COMPONENT_COUNT         36

extern rt_mq_t motor_mq;
extern rt_mq_t servo_mq;

extern void add_component_to_list(const char *key, component_info_t info);
extern void event_finish_cb(lv_event_t * e);

struct fdb_kvdb components_kv_db;
struct fdb_default_kv_node default_kv_table[]={
        {"boot_count", 0, sizeof(uint16_t)}
};

int database_init(void){
    fdb_err_t res;

    struct fdb_default_kv default_kv;
    default_kv.kvs = default_kv_table;
    default_kv.num = sizeof(default_kv_table)/sizeof(default_kv_table[0]);

    fdb_kvdb_control(&components_kv_db, FDB_KVDB_CTRL_SET_UNLOCK, NULL);

    res = fdb_kvdb_init(&components_kv_db, "components_db", "flashDB", &default_kv, NULL);
    if(res != FDB_NO_ERR){
        FDB_INFO("Error: failed to initialize components_db\n");
       return -1;
    }

//    extern void kvdb_basic_sample(fdb_kvdb_t kvdb);
//    kvdb_basic_sample(&device_kv_db);

    return 0;
}


int components_add_kv(char *jsoninfo, uint32_t quantity){
    struct fdb_kv_iterator iterator;
    fdb_kv_t cur_kv;
    struct fdb_blob blob;

    component_info_t info = (component_info_t)rt_malloc(sizeof(component_info));
    if(info == NULL){
        rt_kprintf("add kv info malloc failed\n");
        return -1;
    }

    fdb_kv_iterator_init(&components_kv_db, &iterator);
    while (fdb_kv_iterate(&components_kv_db, &iterator)) {
        cur_kv = &(iterator.curr_kv);
        if(cur_kv->value_len != sizeof(component_info)){
            continue;
        }
        memset((void *)info, 0, sizeof(component_info));
        fdb_blob_read((fdb_db_t)&components_kv_db, fdb_kv_to_blob(cur_kv, fdb_blob_make(&blob, info, sizeof(component_info))));
        /*
         * balabala do what ever you like with blob...
         */

        if(strcmp((const char*)jsoninfo, info->para_info) == 0){
            //存在相同器件
            static const char * btns[] = {""};
            lv_obj_t * msgbox = lv_msgbox_create(NULL, "INFO", "same components exit", btns, true);
            lv_obj_center(msgbox);

            rt_kprintf("same component exist: %s\n", info->para_info);
            rt_free(info);
            return 1;
        }

    }
    //没有相同器件，添加新器件
    memset((void *)info, 0, sizeof(component_info));
    uint8_t i;
    for (i = 0; i < MAX_COMPONENT_COUNT; i++) {
        struct fdb_kv temp_kv;
        char key[3];
        rt_snprintf(key, sizeof(key), "%02d", i);
        if (fdb_kv_get_obj(&components_kv_db, key, &temp_kv) == NULL){       // 找到一个空位 key,添加信息
            info->quantity = quantity;
            strncpy(info->para_info, jsoninfo, MAX_COMPONENT_INFO_LEN-1);
            info->para_info[MAX_COMPONENT_INFO_LEN-1] = '\0';
            fdb_kv_set_blob(&components_kv_db, key, fdb_blob_make(&blob, info, sizeof(component_info)));

            rt_uint8_t pos = atoi(key)%12;
            rt_uint8_t layer = atoi(key)/12;
            rt_mq_send(motor_mq, &pos, sizeof(pos));        //发送存储位置信息
            rt_mq_send(servo_mq, &layer, sizeof(layer));    //发送存储层数信息

            static const char * btns[] = {"finish", ""};
            lv_obj_t * finish_msgbox = lv_msgbox_create(NULL, "INFO", "click the button when finished.", btns, false);
            lv_obj_add_event_cb(finish_msgbox, event_finish_cb, LV_EVENT_VALUE_CHANGED, NULL);
            lv_obj_center(finish_msgbox);
            rt_kprintf("add new component  key: %s\n",key);
            break;
        }
    }
    rt_free(info);

    if(i >= MAX_COMPONENT_COUNT){
        // 器件盒满
        static const char * btns[] = {""};
        lv_obj_t * msgbox = lv_msgbox_create(NULL, "WARNING", "the box is full", btns, true);
        lv_obj_center(msgbox);
        rt_kprintf("the box is full, add failed\n");

        return -1;
    }
    return 0;
}

static rt_bool_t Is_match(const char *filter, const char *target) {
    if(target == NULL){
        return 0;
    }
    if (filter == NULL || filter[0] == '\0') {
        return 1; // 过滤器匹配任意值
    }
    return (strcmp(filter, target) == 0);
}

int components_find_kv(filter_info_t filter){
    struct fdb_kv_iterator iterator;
    fdb_kv_t cur_kv;
    struct fdb_blob blob;

    component_info_t info = (component_info_t)rt_malloc(sizeof(component_info));
    if(info == NULL){
        rt_kprintf("find kv info malloc failed\n");
       return -1;
    }

    fdb_kv_iterator_init(&components_kv_db, &iterator);
    while (fdb_kv_iterate(&components_kv_db, &iterator)) {
       cur_kv = &(iterator.curr_kv);
       if(cur_kv->value_len != sizeof(component_info)){
         //kv存储类型不符
         continue;
       }
       memset((void *)info, 0, sizeof(component_info));
       fdb_blob_read((fdb_db_t) &components_kv_db, fdb_kv_to_blob(cur_kv, fdb_blob_make(&blob, info, sizeof(component_info))));
       /*
        * balabala do what ever you like with blob...
        */
       rt_bool_t match = 0;
       cJSON *root = cJSON_Parse((const char*)info->para_info);     //字符信息解析出JSON数据
       if(root == NULL){
           rt_kprintf("cJSON parse is NULL\n");
           continue;
       }                                                            //得到对应属性信息
       cJSON *type =  cJSON_GetObjectItem(root, "type");            //类型，如电阻
       cJSON *value = cJSON_GetObjectItem(root, "val");             //值，如10k
       cJSON *package = cJSON_GetObjectItem(root, "pkg");           //封装，如0603
       cJSON *rated = cJSON_GetObjectItem(root, "rated val");
       cJSON *tol = cJSON_GetObjectItem(root, "accuracy");
       cJSON *other = cJSON_GetObjectItem(root, "other");

       if(type && value && rated && tol && other){                  //判断是否匹配
           match = Is_match(filter->type, type->valuestring)&&
                  Is_match(filter->val, value->valuestring)&&
                  Is_match(filter->package, package->valuestring)&&
                  Is_match(filter->ratedval, rated->valuestring)&&
                  Is_match(filter->tolerance, tol->valuestring)&&
                  Is_match(filter->otherinfo, other->valuestring);
           if(match){                             //找到器件
               add_component_to_list(cur_kv->name, info);   //将器件信息添加至屏幕列表
               rt_kprintf("find component success, the key is %s\n", cur_kv->name);
           }
       }
       cJSON_Delete(root);
    }
    rt_free(info);
    return 0;
}




