/**
 * @file onenet.c
 * @brief OneNET MQTT Protocol Implementation for STM32F103
 */

#include "stm32f10x.h"
#include "esp8266.h"
#include "onenet.h"
#include "mqttkit.h"
#include "base64.h"
#include "hmac_sha1.h"
#include "usart.h"
#include "delay.h"
#include "led.h"
#include <string.h>
#include <stdio.h>

#define OneNet_Product_ID         "182uSs4N9a"
#define OneNet_Access_KEY         "8V9zxR1hO/WVs8kXUx9dzz9D/XG3d0mhUdVnezFvI8E="
#define OneNet_Device_NAME        "addp_01"
#define OneNet_Service_Address    "mqtts.heclouds.com"

char devid[16];
char key[48];

extern unsigned char esp8266_buf[512];
extern u16 light;
extern volatile u8 led_mode;
extern volatile u8 manual_led_state;

static unsigned char OTA_UrlEncode(char *sign)
{
	char sign_t[40];
	unsigned char i = 0, j = 0;
	unsigned char sign_len = strlen(sign);
	
	if(sign == (void *)0 || sign_len < 28)
		return 1;
	
	for(; i < sign_len; i++)
	{
		sign_t[i] = sign[i];
		sign[i] = 0;
	}
	sign_t[i] = 0;
	
	for(i = 0, j = 0; i < sign_len; i++)
	{
		switch(sign_t[i])
		{
			case '+': strcat(sign + j, "%2B");j += 3; break;
			case ' ': strcat(sign + j, "%20");j += 3; break;
			case '/': strcat(sign + j, "%2F");j += 3; break;
			case '?': strcat(sign + j, "%3F");j += 3; break;
			case '%': strcat(sign + j, "%25");j += 3; break;
			case '#': strcat(sign + j, "%23");j += 3; break;
			case '&': strcat(sign + j, "%26");j += 3; break;
			case '=': strcat(sign + j, "%3D");j += 3; break;
			default: sign[j] = sign_t[i];j++; break;
		}
	}
	
	sign[j] = 0;
	return 0;
}

#define METHOD "sha1"
static unsigned char OneNET_Authorization(char *ver, char *res, unsigned int et, char *access_key, char *dev_name,
											char *authorization_buf, unsigned short authorization_buf_len, _Bool flag)
{
	size_t olen = 0;
	char sign_buf[64];
	char hmac_sha1_buf[64];
	char access_key_base64[64];
	char string_for_signature[72];

	if(ver == (void *)0 || res == (void *)0 || et < 1564562581 || access_key == (void *)0
		|| authorization_buf == (void *)0 || authorization_buf_len < 120)
		return 1;
	
	memset(access_key_base64, 0, sizeof(access_key_base64));
	BASE64_Decode((unsigned char *)access_key_base64, sizeof(access_key_base64), &olen, (unsigned char *)access_key, strlen(access_key));
	
	memset(string_for_signature, 0, sizeof(string_for_signature));
	if(flag)
		snprintf(string_for_signature, sizeof(string_for_signature), "%d\n%s\nproducts/%s\n%s", et, METHOD, res, ver);
	else
		snprintf(string_for_signature, sizeof(string_for_signature), "%d\n%s\nproducts/%s/devices/%s\n%s", et, METHOD, res, dev_name, ver);
	
	memset(hmac_sha1_buf, 0, sizeof(hmac_sha1_buf));
	hmac_sha1((unsigned char *)access_key_base64, strlen(access_key_base64),
				(unsigned char *)string_for_signature, strlen(string_for_signature),
				(unsigned char *)hmac_sha1_buf);
	
	olen = 0;
	memset(sign_buf, 0, sizeof(sign_buf));
	BASE64_Encode((unsigned char *)sign_buf, sizeof(sign_buf), &olen, (unsigned char *)hmac_sha1_buf, strlen(hmac_sha1_buf));
	OTA_UrlEncode(sign_buf);
	
	if(flag)
		snprintf(authorization_buf, authorization_buf_len, "version=%s&res=products%%2F%s&et=%d&method=%s&sign=%s", ver, res, et, METHOD, sign_buf);
	else
		snprintf(authorization_buf, authorization_buf_len, "version=%s&res=products%%2F%s%%2Fdevices%%2F%s&et=%d&method=%s&sign=%s", ver, res, dev_name, et, METHOD, sign_buf);
	
	return 0;
}

_Bool OneNet_DevLink(void)
{
    MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};
    unsigned char *dataPtr;
    char authorization_buf[160];
    _Bool status = 1;

    OneNET_Authorization("2018-10-31", OneNet_Product_ID, 1956499200, OneNet_Access_KEY, OneNet_Device_NAME,
                        authorization_buf, sizeof(authorization_buf), 0);

    UsartPrintf(USART_DEBUG, "OneNET_DevLink\r\nNAME: %s, Product_ID: %s\r\n", 
                OneNet_Device_NAME, OneNet_Product_ID);

    if(MQTT_PacketConnect(OneNet_Product_ID, authorization_buf, OneNet_Device_NAME, 
                        256, 1, MQTT_QOS_LEVEL0, NULL, NULL, 0, &mqttPacket) == 0)
    {
        ESP8266_SendData(mqttPacket._data, mqttPacket._len);
        dataPtr = ESP8266_GetIPD(250);
        if(dataPtr != NULL)
        {
            if(MQTT_UnPacketRecv(dataPtr) == MQTT_PKT_CONNACK)
            {
                switch(MQTT_UnPacketConnectAck(dataPtr))
                {
                    case 0: UsartPrintf(USART_DEBUG, "Connected to OneNET successfully\r\n"); status = 0; break;
                    case 1: UsartPrintf(USART_DEBUG, "Connection failed: Protocol error\r\n"); break;
                    case 2: UsartPrintf(USART_DEBUG, "Connection failed: Invalid client ID\r\n"); break;
                    case 3: UsartPrintf(USART_DEBUG, "Connection failed: Server unavailable\r\n"); break;
                    case 4: UsartPrintf(USART_DEBUG, "Connection failed: Invalid username/password\r\n"); break;
                    case 5: UsartPrintf(USART_DEBUG, "Connection failed: Unauthorized\r\n"); break;
                    default: UsartPrintf(USART_DEBUG, "Connection failed: Unknown reason\r\n"); break;
                }
            }
        }
        MQTT_DeleteBuffer(&mqttPacket);
    }
    else
        UsartPrintf(USART_DEBUG, "MQTT_PacketConnect Failed\r\n");
    
    return status;
}

unsigned char OneNet_FillBuf(char *buf)
{
    char text[64];
    memset(text, 0, sizeof(text));
    strcpy(buf, "{\"method\":\"thing.property.post\",\"id\":\"1\",\"params\":{");
    memset(text, 0, sizeof(text));
    sprintf(text, "\"light\":{\"value\":%d}", light);
    strcat(buf, text);
    strcat(buf, "},\"version\":\"1.0\"}");
    UsartPrintf(USART_DEBUG, "Send JSON: %s\r\n", buf);
    return strlen(buf);
}

void OneNet_SendData(void)
{
    MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};
    char buf[256];
    short body_len = 0, i = 0;

    UsartPrintf(USART_DEBUG, "OneNet_SendData-MQTT\r\n");
    memset(buf, 0, sizeof(buf));
    body_len = OneNet_FillBuf(buf);

    if(body_len)
    {
        if(MQTT_PacketSaveData(OneNet_Product_ID, OneNet_Device_NAME, body_len, NULL, &mqttPacket) == 0)
        {
            for(; i < body_len; i++)
                mqttPacket._data[mqttPacket._len++] = buf[i];
            ESP8266_SendData(mqttPacket._data, mqttPacket._len);
            UsartPrintf(USART_DEBUG, "Sent %d Bytes\r\n", mqttPacket._len);
            MQTT_DeleteBuffer(&mqttPacket);
        }
        else
            UsartPrintf(USART_DEBUG, "Failed to create MQTT packet\r\n");
    }
}

void OneNET_Publish(const char *topic, const char *msg)
{
    MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};
	UsartPrintf(USART_DEBUG, "Publish Topic: %s, Msg: %s\r\n", topic, msg);
	if(MQTT_PacketPublish(MQTT_PUBLISH_ID, topic, msg, strlen(msg), MQTT_QOS_LEVEL0, 0, 1, &mqtt_packet) == 0)
	{
		ESP8266_SendData(mqtt_packet._data, mqtt_packet._len);
		MQTT_DeleteBuffer(&mqtt_packet);
	}
}

void OneNET_PushLightToFan(void)
{
    static u8 last_buzzer_cmd = 0;
    char topic[64];
    char msg[64];
    u8 should_buzzer_on = (light >= 3000) ? 1 : 0;
    
    if(should_buzzer_on != last_buzzer_cmd)
    {
        last_buzzer_cmd = should_buzzer_on;
        snprintf(topic, sizeof(topic), "$sys/%s/addp_03/thing/property/set", OneNet_Product_ID);
        snprintf(msg, sizeof(msg), "{\"buzzer_switch\":{\"value\":%s}}", should_buzzer_on ? "true" : "false");
        OneNET_Publish(topic, msg);
    }
}

void OneNET_Subscribe(void)
{
	MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};
	char topic_buf[64];
	const char *topic = topic_buf;
	snprintf(topic_buf, sizeof(topic_buf), "$sys/%s/%s/thing/property/set", OneNet_Product_ID, OneNet_Device_NAME);
	UsartPrintf(USART_DEBUG, "Subscribe Topic: %s\r\n", topic_buf);
	if(MQTT_PacketSubscribe(MQTT_SUBSCRIBE_ID, MQTT_QOS_LEVEL0, &topic, 1, &mqtt_packet) == 0)
	{
		ESP8266_SendData(mqtt_packet._data, mqtt_packet._len);
		MQTT_DeleteBuffer(&mqtt_packet);
	}
}

void OneNet_RevPro(unsigned char *cmd)
{
	char *req_payload = NULL;
	char *cmdid_topic = NULL;
	unsigned short topic_len = 0;
	unsigned short req_len = 0;
	unsigned char qos = 0;
	static unsigned short pkt_id = 0;
	unsigned char type = 0;
	short result = 0;

	type = MQTT_UnPacketRecv(cmd);
	switch(type)
	{
		case MQTT_PKT_PUBLISH:
			result = MQTT_UnPacketPublish(cmd, &cmdid_topic, &topic_len, &req_payload, &req_len, &qos, &pkt_id);
			if(result == 0)
			{
				const char* led_start = strstr(req_payload, "\"led\":");

				UsartPrintf(USART_DEBUG, "topic: %s, payload: %s\r\n", cmdid_topic, req_payload);

				if (led_start != NULL) {
					const char* val_ptr;
					int led_on = 0;
					
					/* Try nested format: {"value":true/false} */
					val_ptr = strstr(led_start, "\"value\":");
					if (val_ptr != NULL) {
						val_ptr += 8;
						while (*val_ptr == ' ' || *val_ptr == '\t') val_ptr++;
						if (strncmp(val_ptr, "true", 4) == 0) led_on = 1;
					} else {
						/* Direct format: true/false after "led": */
						val_ptr = led_start + 6;
						while (*val_ptr == ' ' || *val_ptr == '\t') val_ptr++;
						if (strncmp(val_ptr, "true", 4) == 0) led_on = 1;
					}
					
					if (led_on) {
						UsartPrintf(USART_DEBUG, "led=true - Manual mode, PB1 Green LED ON\r\n");
						led_mode = 1;
						manual_led_state = 1;
						LED_Green_On();
					} else {
						UsartPrintf(USART_DEBUG, "led=false - Switch to AUTO mode (light sensor)\r\n");
						led_mode = 0;
						manual_led_state = 0;
						LED_Green_Off();
					}
				}

				{
					char topic_buf[80];
					char reply[128];
					const char* id_start = strstr(req_payload, "\"id\":\"");
					char req_id[40];
					if (id_start != NULL) {
						int i;
						id_start += 6; // skip "id":"
						for (i = 0; id_start[i] != '"' && id_start[i] != 0 && i < 36; i++) {
							req_id[i] = id_start[i];
						}
						req_id[i] = 0;

						snprintf(topic_buf, sizeof(topic_buf), "$sys/%s/%s/thing/property/set_reply",
								OneNet_Product_ID, OneNet_Device_NAME);
						snprintf(reply, sizeof(reply), "{\"id\":\"%s\",\"code\":0,\"msg\":\"success\"}", req_id);
						UsartPrintf(USART_DEBUG, "Reply Topic: %s, Payload: %s\r\n", topic_buf, reply);
						OneNET_Publish(topic_buf, reply);
					} else {
						UsartPrintf(USART_DEBUG, "No id found in property set message, cannot reply\r\n");
					}
				}
			}
			break;
		case MQTT_PKT_PUBACK:
			if(MQTT_UnPacketPublishAck(cmd) == 0)
				UsartPrintf(USART_DEBUG, "Tips: MQTT Publish Send OK\r\n");
		break;
		case MQTT_PKT_SUBACK:
			if(MQTT_UnPacketSubscribe(cmd) == 0)
				UsartPrintf(USART_DEBUG, "Tips: MQTT Subscribe OK\r\n");
			else
				UsartPrintf(USART_DEBUG, "Tips: MQTT Subscribe Err\r\n");
		break;
		default:
			result = -1;
		break;
	}
	
	ESP8266_Clear();
	if(result == -1) return;
	if(type == MQTT_PKT_CMD || type == MQTT_PKT_PUBLISH)
	{
	MQTT_FreeBuffer(cmdid_topic);
	MQTT_FreeBuffer(req_payload);
  }
}