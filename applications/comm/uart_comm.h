#ifndef __UART_COMM_H__
#define __UART_COMM_H__

#include <rtthread.h>
#include <rtdevice.h>

#define UART_COMM_DEVICE_NAME    "uart5"
#define UART_COMM_BAUDRATE       115200
#define UART_COMM_BUF_SIZE       512
#define UART_COMM_FRAME_MAX_SIZE 400

#define FRAME_HEADER_BYTE1       0xAA
#define FRAME_HEADER_BYTE2       0x55

#define CMD_QUERY_ALL            0x01
#define CMD_QUERY_ONE            0x02
#define CMD_ADD_COMPONENT        0x03
#define CMD_DELETE_COMPONENT     0x04
#define CMD_SYNC_DATA            0x05
#define CMD_HEARTBEAT            0x06

#define RESP_OK                  0x00
#define RESP_ERROR               0x01
#define RESP_NOT_FOUND           0x02
#define RESP_INVALID_PARAM       0x03
#define RESP_NO_MEMORY           0x04

typedef struct {
    rt_uint8_t cmd;
    rt_uint16_t length;
    rt_uint8_t data[UART_COMM_FRAME_MAX_SIZE - 6];
    rt_uint16_t crc;
} __attribute__((packed)) uart_frame_t;

typedef struct {
    rt_uint8_t resp;
    rt_uint16_t length;
    rt_uint8_t data[UART_COMM_FRAME_MAX_SIZE - 6];
    rt_uint16_t crc;
} __attribute__((packed)) uart_response_t;

void uart_comm_init(void);

#endif /* __UART_COMM_H__ */