#include "uart_comm.h"
#include "database.h"
#include "cJSON.h"

#define DBG_TAG "uart_comm"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

static rt_device_t uart_dev = RT_NULL;
static rt_thread_t uart_recv_thread = RT_NULL;
static rt_uint8_t recv_buf[UART_COMM_BUF_SIZE];
static rt_uint8_t recv_index = 0;
static rt_mutex_t recv_mutex = RT_NULL;

static rt_uint16_t crc16(const rt_uint8_t *data, rt_uint16_t length)
{
    rt_uint16_t crc = 0xFFFF;
    rt_uint16_t i, j;
    
    for (i = 0; i < length; i++)
    {
        crc ^= data[i];
        for (j = 0; j < 8; j++)
        {
            if (crc & 0x0001)
            {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    return crc;
}

static rt_err_t uart_recv_callback(rt_device_t dev, rt_size_t size)
{
    rt_uint8_t ch;
    
    if (size == 0)
        return RT_EOK;
    
    rt_mutex_take(recv_mutex, RT_WAITING_FOREVER);
    
    while (rt_device_read(dev, -1, &ch, 1) == 1)
    {
        if (recv_index >= UART_COMM_BUF_SIZE)
        {
            recv_index = 0;
        }
        
        if (recv_index == 0 && ch != FRAME_HEADER_BYTE1)
            continue;
        
        if (recv_index == 1 && ch != FRAME_HEADER_BYTE2)
        {
            recv_index = (ch == FRAME_HEADER_BYTE1) ? 1 : 0;
            continue;
        }
        
        recv_buf[recv_index++] = ch;
        
        if (recv_index >= 6)
        {
            rt_uint16_t frame_len = (recv_buf[2] << 8) | recv_buf[3];
            if (frame_len + 6 <= UART_COMM_BUF_SIZE && recv_index >= frame_len + 6)
            {
                rt_uint16_t calc_crc = crc16(&recv_buf[4], frame_len);
                rt_uint16_t recv_crc = (recv_buf[4 + frame_len] << 8) | recv_buf[4 + frame_len + 1];
                
                if (calc_crc == recv_crc)
                {
                    rt_uint8_t cmd = recv_buf[4];
                    rt_uint8_t *payload = &recv_buf[5];
                    rt_uint16_t payload_len = frame_len - 1;
                    
                    LOG_D("Received command: 0x%02X, payload len: %d", cmd, payload_len);
                    
                    if (cmd == CMD_QUERY_ALL)
                    {
                        fdb_kv_iterator_t iter;
                        fdb_kvdb_iterator_init(&components_kv_db, &iter);
                        
                        cJSON *root = cJSON_CreateArray();
                        if (root == NULL)
                        {
                            LOG_E("Failed to create JSON array");
                            recv_index = 0;
                            rt_mutex_release(recv_mutex);
                            return RT_EOK;
                        }
                        
                        while (fdb_kv_iterator_next(&iter) == FDB_OK)
                        {
                            cJSON *item = cJSON_CreateObject();
                            cJSON_AddStringToObject(item, "key", fdb_kv_iterator_get_key(&iter));
                            
                            struct fdb_blob blob;
                            component_info info;
                            fdb_blob_make(&blob, &info, sizeof(component_info));
                            
                            if (fdb_kv_get_blob(&components_kv_db, fdb_kv_iterator_get_key(&iter), &blob) == FDB_OK)
                            {
                                cJSON_AddStringToObject(item, "info", info.para_info);
                                cJSON_AddNumberToObject(item, "quantity", info.quantity);
                            }
                            
                            cJSON_AddItemToArray(root, item);
                        }
                        
                        char *json_str = cJSON_PrintUnformatted(root);
                        if (json_str != NULL)
                        {
                            rt_uint16_t json_len = rt_strlen(json_str);
                            rt_uint8_t send_buf[UART_COMM_FRAME_MAX_SIZE];
                            
                            send_buf[0] = FRAME_HEADER_BYTE1;
                            send_buf[1] = FRAME_HEADER_BYTE2;
                            send_buf[2] = (json_len + 2) >> 8;
                            send_buf[3] = (json_len + 2) & 0xFF;
                            send_buf[4] = RESP_OK;
                            rt_memcpy(&send_buf[5], json_str, json_len);
                            
                            rt_uint16_t crc = crc16(&send_buf[4], json_len + 1);
                            send_buf[5 + json_len] = crc >> 8;
                            send_buf[5 + json_len + 1] = crc & 0xFF;
                            
                            rt_device_write(uart_dev, 0, send_buf, json_len + 7);
                            rt_free(json_str);
                        }
                        
                        cJSON_Delete(root);
                        fdb_kvdb_iterator_close(&iter);
                    }
                    else if (cmd == CMD_QUERY_ONE)
                    {
                        payload[payload_len] = '\0';
                        
                        cJSON *root = cJSON_Parse((char *)payload);
                        if (root == NULL)
                        {
                            LOG_E("Failed to parse JSON");
                            recv_index = 0;
                            rt_mutex_release(recv_mutex);
                            return RT_EOK;
                        }
                        
                        const char *key = cJSON_GetObjectItem(root, "key")->valuestring;
                        if (key != NULL)
                        {
                            struct fdb_blob blob;
                            component_info info;
                            fdb_blob_make(&blob, &info, sizeof(component_info));
                            
                            if (fdb_kv_get_blob(&components_kv_db, key, &blob) == FDB_OK)
                            {
                                cJSON *resp = cJSON_CreateObject();
                                cJSON_AddStringToObject(resp, "key", key);
                                cJSON_AddStringToObject(resp, "info", info.para_info);
                                cJSON_AddNumberToObject(resp, "quantity", info.quantity);
                                
                                char *json_str = cJSON_PrintUnformatted(resp);
                                if (json_str != NULL)
                                {
                                    rt_uint16_t json_len = rt_strlen(json_str);
                                    rt_uint8_t send_buf[UART_COMM_FRAME_MAX_SIZE];
                                    
                                    send_buf[0] = FRAME_HEADER_BYTE1;
                                    send_buf[1] = FRAME_HEADER_BYTE2;
                                    send_buf[2] = (json_len + 2) >> 8;
                                    send_buf[3] = (json_len + 2) & 0xFF;
                                    send_buf[4] = RESP_OK;
                                    rt_memcpy(&send_buf[5], json_str, json_len);
                                    
                                    rt_uint16_t crc = crc16(&send_buf[4], json_len + 1);
                                    send_buf[5 + json_len] = crc >> 8;
                                    send_buf[5 + json_len + 1] = crc & 0xFF;
                                    
                                    rt_device_write(uart_dev, 0, send_buf, json_len + 7);
                                    rt_free(json_str);
                                }
                                cJSON_Delete(resp);
                            }
                            else
                            {
                                rt_uint8_t send_buf[10];
                                send_buf[0] = FRAME_HEADER_BYTE1;
                                send_buf[1] = FRAME_HEADER_BYTE2;
                                send_buf[2] = 0x00;
                                send_buf[3] = 0x02;
                                send_buf[4] = RESP_NOT_FOUND;
                                send_buf[5] = 0x00;
                                rt_uint16_t crc = crc16(&send_buf[4], 1);
                                send_buf[6] = crc >> 8;
                                send_buf[7] = crc & 0xFF;
                                rt_device_write(uart_dev, 0, send_buf, 8);
                            }
                        }
                        cJSON_Delete(root);
                    }
                    else if (cmd == CMD_ADD_COMPONENT)
                    {
                        payload[payload_len] = '\0';
                        
                        cJSON *root = cJSON_Parse((char *)payload);
                        if (root == NULL)
                        {
                            LOG_E("Failed to parse JSON");
                            recv_index = 0;
                            rt_mutex_release(recv_mutex);
                            return RT_EOK;
                        }
                        
                        const char *info_str = cJSON_GetObjectItem(root, "info")->valuestring;
                        rt_uint32_t quantity = cJSON_GetObjectItem(root, "quantity")->valueint;
                        
                        if (info_str != NULL)
                        {
                            int ret = components_add_kv((char *)info_str, quantity);
                            
                            rt_uint8_t send_buf[10];
                            send_buf[0] = FRAME_HEADER_BYTE1;
                            send_buf[1] = FRAME_HEADER_BYTE2;
                            send_buf[2] = 0x00;
                            send_buf[3] = 0x02;
                            send_buf[4] = (ret == RT_EOK) ? RESP_OK : RESP_ERROR;
                            send_buf[5] = 0x00;
                            rt_uint16_t crc = crc16(&send_buf[4], 1);
                            send_buf[6] = crc >> 8;
                            send_buf[7] = crc & 0xFF;
                            rt_device_write(uart_dev, 0, send_buf, 8);
                        }
                        cJSON_Delete(root);
                    }
                    else if (cmd == CMD_DELETE_COMPONENT)
                    {
                        payload[payload_len] = '\0';
                        
                        cJSON *root = cJSON_Parse((char *)payload);
                        if (root == NULL)
                        {
                            LOG_E("Failed to parse JSON");
                            recv_index = 0;
                            rt_mutex_release(recv_mutex);
                            return RT_EOK;
                        }
                        
                        const char *key = cJSON_GetObjectItem(root, "key")->valuestring;
                        
                        if (key != NULL)
                        {
                            fdb_err_t ret = fdb_kv_del(&components_kv_db, key);
                            
                            rt_uint8_t send_buf[10];
                            send_buf[0] = FRAME_HEADER_BYTE1;
                            send_buf[1] = FRAME_HEADER_BYTE2;
                            send_buf[2] = 0x00;
                            send_buf[3] = 0x02;
                            send_buf[4] = (ret == FDB_OK) ? RESP_OK : RESP_NOT_FOUND;
                            send_buf[5] = 0x00;
                            rt_uint16_t crc = crc16(&send_buf[4], 1);
                            send_buf[6] = crc >> 8;
                            send_buf[7] = crc & 0xFF;
                            rt_device_write(uart_dev, 0, send_buf, 8);
                        }
                        cJSON_Delete(root);
                    }
                    else if (cmd == CMD_HEARTBEAT)
                    {
                        rt_uint8_t send_buf[10];
                        send_buf[0] = FRAME_HEADER_BYTE1;
                        send_buf[1] = FRAME_HEADER_BYTE2;
                        send_buf[2] = 0x00;
                        send_buf[3] = 0x02;
                        send_buf[4] = RESP_OK;
                        send_buf[5] = 0x00;
                        rt_uint16_t crc = crc16(&send_buf[4], 1);
                        send_buf[6] = crc >> 8;
                        send_buf[7] = crc & 0xFF;
                        rt_device_write(uart_dev, 0, send_buf, 8);
                    }
                    
                    recv_index = 0;
                }
                else
                {
                    LOG_E("CRC mismatch: calc=0x%04X, recv=0x%04X", calc_crc, recv_crc);
                    recv_index = 0;
                }
            }
        }
    }
    
    rt_mutex_release(recv_mutex);
    return RT_EOK;
}

static void uart_recv_thread_entry(void *parameter)
{
    LOG_I("UART5 communication thread started");
    
    while (1)
    {
        rt_thread_mdelay(100);
    }
}

void uart_comm_init(void)
{
    uart_dev = rt_device_find(UART_COMM_DEVICE_NAME);
    if (uart_dev == RT_NULL)
    {
        LOG_E("Cannot find UART device: %s", UART_COMM_DEVICE_NAME);
        return;
    }
    
    if (rt_device_open(uart_dev, RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX) != RT_EOK)
    {
        LOG_E("Failed to open UART device: %s", UART_COMM_DEVICE_NAME);
        return;
    }
    
    struct serial_configure config = RT_SERIAL_CONFIG_DEFAULT;
    config.baud_rate = BAUD_RATE_115200;
    config.data_bits = DATA_BITS_8;
    config.stop_bits = STOP_BITS_1;
    config.bufsz = UART_COMM_BUF_SIZE;
    rt_device_control(uart_dev, RT_DEVICE_CTRL_CONFIG, &config);
    
    recv_mutex = rt_mutex_create("uart_recv_mtx", RT_IPC_FLAG_PRIO);
    if (recv_mutex == RT_NULL)
    {
        LOG_E("Failed to create mutex");
        rt_device_close(uart_dev);
        return;
    }
    
    rt_device_set_rx_indicate(uart_dev, uart_recv_callback);
    
    uart_recv_thread = rt_thread_create("uart_recv", uart_recv_thread_entry, RT_NULL,
                                         1024, 12, 10);
    if (uart_recv_thread != RT_NULL)
    {
        rt_thread_startup(uart_recv_thread);
        LOG_I("UART5 communication initialized successfully");
    }
    else
    {
        LOG_E("Failed to create UART recv thread");
        rt_mutex_delete(recv_mutex);
        rt_device_close(uart_dev);
    }
}
INIT_APP_EXPORT(uart_comm_init);