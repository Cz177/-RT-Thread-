#include <HardwareSerial.h>
#include <ArduinoJson.h>

#define UART_TX_PIN    43
#define UART_RX_PIN    44
#define BAUDRATE       115200
#define BUF_SIZE       512

#define FRAME_HEADER_BYTE1  0xAA
#define FRAME_HEADER_BYTE2  0x55

#define CMD_QUERY_ALL        0x01
#define CMD_QUERY_ONE        0x02
#define CMD_ADD_COMPONENT    0x03
#define CMD_DELETE_COMPONENT 0x04
#define CMD_SYNC_DATA        0x05
#define CMD_HEARTBEAT        0x06

#define RESP_OK              0x00
#define RESP_ERROR           0x01
#define RESP_NOT_FOUND       0x02
#define RESP_INVALID_PARAM   0x03
#define RESP_NO_MEMORY       0x04

HardwareSerial SerialComm(2);

uint8_t recv_buf[BUF_SIZE];
uint16_t recv_index = 0;

uint16_t crc16(const uint8_t *data, uint16_t length)
{
    uint16_t crc = 0xFFFF;
    uint16_t i, j;
    
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

void send_frame(uint8_t cmd, const char *payload)
{
    uint16_t payload_len = strlen(payload);
    uint16_t frame_len = payload_len + 1;
    
    uint8_t send_buf[BUF_SIZE];
    send_buf[0] = FRAME_HEADER_BYTE1;
    send_buf[1] = FRAME_HEADER_BYTE2;
    send_buf[2] = (frame_len >> 8) & 0xFF;
    send_buf[3] = frame_len & 0xFF;
    send_buf[4] = cmd;
    
    if (payload_len > 0)
    {
        memcpy(&send_buf[5], payload, payload_len);
    }
    
    uint16_t crc = crc16(&send_buf[4], frame_len);
    send_buf[5 + payload_len] = (crc >> 8) & 0xFF;
    send_buf[5 + payload_len + 1] = crc & 0xFF;
    
    SerialComm.write(send_buf, frame_len + 6);
    SerialComm.flush();
}

void send_query_all()
{
    send_frame(CMD_QUERY_ALL, "");
}

void send_query_one(const char *key)
{
    StaticJsonDocument<256> doc;
    doc["key"] = key;
    
    char payload[256];
    serializeJson(doc, payload, sizeof(payload));
    
    send_frame(CMD_QUERY_ONE, payload);
}

void send_add_component(const char *info, uint32_t quantity)
{
    StaticJsonDocument<512> doc;
    doc["info"] = info;
    doc["quantity"] = quantity;
    
    char payload[512];
    serializeJson(doc, payload, sizeof(payload));
    
    send_frame(CMD_ADD_COMPONENT, payload);
}

void send_delete_component(const char *key)
{
    StaticJsonDocument<256> doc;
    doc["key"] = key;
    
    char payload[256];
    serializeJson(doc, payload, sizeof(payload));
    
    send_frame(CMD_DELETE_COMPONENT, payload);
}

void send_heartbeat()
{
    send_frame(CMD_HEARTBEAT, "");
}

void process_response()
{
    if (recv_index < 6)
        return;
    
    uint16_t frame_len = (recv_buf[2] << 8) | recv_buf[3];
    
    if (frame_len + 6 > BUF_SIZE || recv_index < frame_len + 6)
        return;
    
    uint16_t calc_crc = crc16(&recv_buf[4], frame_len);
    uint16_t recv_crc = (recv_buf[4 + frame_len] << 8) | recv_buf[4 + frame_len + 1];
    
    if (calc_crc != recv_crc)
    {
        Serial.println("CRC mismatch");
        recv_index = 0;
        return;
    }
    
    uint8_t resp_code = recv_buf[4];
    uint16_t payload_len = frame_len - 1;
    
    Serial.print("Response code: 0x");
    Serial.println(resp_code, HEX);
    
    if (payload_len > 0)
    {
        recv_buf[5 + payload_len] = '\0';
        Serial.println("Payload:");
        Serial.println((char *)&recv_buf[5]);
        
        if (resp_code == RESP_OK)
        {
            DynamicJsonDocument doc(4096);
            DeserializationError error = deserializeJson(doc, (char *)&recv_buf[5]);
            
            if (!error)
            {
                if (doc.is<JsonArray>())
                {
                    Serial.println("Component list:");
                    for (JsonObject item : doc.as<JsonArray>())
                    {
                        Serial.print("  Key: ");
                        Serial.println(item["key"].as<String>());
                        Serial.print("  Info: ");
                        Serial.println(item["info"].as<String>());
                        Serial.print("  Quantity: ");
                        Serial.println(item["quantity"].as<uint32_t>());
                    }
                }
                else if (doc.is<JsonObject>())
                {
                    Serial.println("Single component:");
                    Serial.print("  Key: ");
                    Serial.println(doc["key"].as<String>());
                    Serial.print("  Info: ");
                    Serial.println(doc["info"].as<String>());
                    Serial.print("  Quantity: ");
                    Serial.println(doc["quantity"].as<uint32_t>());
                }
            }
        }
    }
    
    recv_index = 0;
}

void setup()
{
    Serial.begin(115200);
    SerialComm.begin(BAUDRATE, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
    
    delay(1000);
    Serial.println("ESP32-S3 UART Communication Ready");
    
    send_heartbeat();
    delay(500);
    send_query_all();
}

void loop()
{
    if (SerialComm.available() > 0)
    {
        while (SerialComm.available() > 0 && recv_index < BUF_SIZE)
        {
            uint8_t ch = SerialComm.read();
            
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
                uint16_t frame_len = (recv_buf[2] << 8) | recv_buf[3];
                
                if (recv_index >= frame_len + 6)
                {
                    process_response();
                }
            }
        }
    }
    
    if (Serial.available() > 0)
    {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        
        if (cmd == "query_all")
        {
            send_query_all();
        }
        else if (cmd.startsWith("query "))
        {
            String key = cmd.substring(6);
            send_query_one(key.c_str());
        }
        else if (cmd == "heartbeat")
        {
            send_heartbeat();
        }
    }
    
    delay(10);
}