/**
 * @file onenet.c
 * @brief OneNET MQTT Protocol Implementation for STM32F103
 * 
 * This file provides functions to connect to OneNET IoT platform,
 * upload sensor data, and handle downlink commands via MQTT protocol.
 */

/* STM32 HAL header */
#include "stm32f10x.h"

/* Peripheral drivers */
#include "esp8266.h"

/* Protocol headers */
#include "onenet.h"
#include "mqttkit.h"

/* Algorithm headers */
#include "base64.h"
#include "hmac_sha1.h"

/* Hardware drivers */
#include "usart.h"
#include "delay.h"
#include "led.h"

/* C standard libraries */
#include <string.h>
#include <stdio.h>

/* OneNET Configuration Parameters */
#define OneNet_Product_ID         "182uSs4N9a"    /* OneNET Product ID */
#define OneNet_Access_KEY         "8V9zxR1hO/WVs8kXUx9dzz9D/XG3d0mhUdVnezFvI8E="  /* OneNET Access Key */
#define OneNet_Device_NAME        "addp_02"       /* OneNET Device Name/ID */
#define OneNet_Service_Address    "mqtts.heclouds.com"  /* OneNET MQTT Server Address */
#define SERVER_NAME_LED           "aaa"           /* LED service name for OneNET thing model */

/* Global variables */
char devid[16];   /* Device ID buffer */
char key[48];     /* Key buffer */

/* External sensor data variables */
extern unsigned char esp8266_buf[512];  /* ESP8266 receive buffer */
extern u8 human;                        /* Human detection status (0=No human, 1=Human detected) */

/*
************************************************************
*	Function:	OTA_UrlEncode
*
*	Description:	URL encode for sign string
*
*	Input:		sign string to encode
*
*	Output:		0-Success, others-Failure
*
*	Note:		+			%2B
*				space		%20
*				/			%2F
*				?			%3F
*				%			%25
*				#			%23
*				&			%26
*				=			%3D
************************************************************
*/
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
			case '+':
				strcat(sign + j, "%2B");j += 3;
			break;
			
			case ' ':
				strcat(sign + j, "%20");j += 3;
			break;
			
			case '/':
				strcat(sign + j, "%2F");j += 3;
			break;
			
			case '?':
				strcat(sign + j, "%3F");j += 3;
			break;
			
			case '%':
				strcat(sign + j, "%25");j += 3;
			break;
			
			case '#':
				strcat(sign + j, "%23");j += 3;
			break;
			
			case '&':
				strcat(sign + j, "%26");j += 3;
			break;
			
			case '=':
				strcat(sign + j, "%3D");j += 3;
			break;
			
			default:
				sign[j] = sign_t[i];j++;
			break;
		}
	}
	
	sign[j] = 0;
	
	return 0;

}

/*
************************************************************
*	Function:	OTA_Authorization
*
*	Description:	Generate Authorization token
*
*	Input:		ver-version, res-product ID
*				et-expire time (UTC)
*				access_key-access key
*				dev_name-device name
*				authorization_buf-token buffer
*				authorization_buf_len-buffer length (bytes)
*
*	Output:		0-Success, others-Failure
*
*	Note:		Currently supports sha1
************************************************************
*/
#define METHOD		"sha1"
static unsigned char OneNET_Authorization(char *ver, char *res, unsigned int et, char *access_key, char *dev_name,
											char *authorization_buf, unsigned short authorization_buf_len, _Bool flag)
{
	
	size_t olen = 0;
	
	char sign_buf[64];								//Sign buffer (Base64 encoded + URL encoded)
	char hmac_sha1_buf[64];							//HMAC SHA1 buffer
	char access_key_base64[64];						//Decoded access key (Base64)
	char string_for_signature[72];					//String for signature generation

//----------------------------------------------------Parameter check--------------------------------------------------------------------
	if(ver == (void *)0 || res == (void *)0 || et < 1564562581 || access_key == (void *)0
		|| authorization_buf == (void *)0 || authorization_buf_len < 120)
		return 1;
	
//----------------------------------------------------Decode access_key from Base64----------------------------------------------------
	memset(access_key_base64, 0, sizeof(access_key_base64));
	BASE64_Decode((unsigned char *)access_key_base64, sizeof(access_key_base64), &olen, (unsigned char *)access_key, strlen(access_key));
	//UsartPrintf(USART_DEBUG, "access_key_base64: %s\r\n", access_key_base64);
	
//----------------------------------------------------Build string_for_signature-----------------------------------------------------
	memset(string_for_signature, 0, sizeof(string_for_signature));
	if(flag)
		snprintf(string_for_signature, sizeof(string_for_signature), "%d\n%s\nproducts/%s\n%s", et, METHOD, res, ver);
	else
		snprintf(string_for_signature, sizeof(string_for_signature), "%d\n%s\nproducts/%s/devices/%s\n%s", et, METHOD, res, dev_name, ver);
	//UsartPrintf(USART_DEBUG, "string_for_signature: %s\r\n", string_for_signature);
	
//----------------------------------------------------HMAC SHA1-------------------------------------------------------------------------
	memset(hmac_sha1_buf, 0, sizeof(hmac_sha1_buf));
	
	hmac_sha1((unsigned char *)access_key_base64, strlen(access_key_base64),
				(unsigned char *)string_for_signature, strlen(string_for_signature),
				(unsigned char *)hmac_sha1_buf);
	
	//UsartPrintf(USART_DEBUG, "hmac_sha1_buf: %s\r\n", hmac_sha1_buf);
	
//----------------------------------------------------Encode signature with Base64------------------------------------------------------
	olen = 0;
	memset(sign_buf, 0, sizeof(sign_buf));
	BASE64_Encode((unsigned char *)sign_buf, sizeof(sign_buf), &olen, (unsigned char *)hmac_sha1_buf, strlen(hmac_sha1_buf));

//----------------------------------------------------URL encode Base64 signature---------------------------------------------------
	OTA_UrlEncode(sign_buf);
	//UsartPrintf(USART_DEBUG, "sign_buf: %s\r\n", sign_buf);
	
//----------------------------------------------------Build Token--------------------------------------------------------------------
	if(flag)
		snprintf(authorization_buf, authorization_buf_len, "version=%s&res=products%%2F%s&et=%d&method=%s&sign=%s", ver, res, et, METHOD, sign_buf);
	else
		snprintf(authorization_buf, authorization_buf_len, "version=%s&res=products%%2F%s%%2Fdevices%%2F%s&et=%d&method=%s&sign=%s", ver, res, dev_name, et, METHOD, sign_buf);
	//UsartPrintf(USART_DEBUG, "Token: %s\r\n", authorization_buf);
	
	return 0;

}

/**
 * @brief Connect device to OneNET platform
 * 
 * This function establishes MQTT connection to OneNET IoT platform
 * using token-based authentication.
 * 
 * @return 0 - Connected successfully, 1 - Connection failed
 */
_Bool OneNet_DevLink(void)
{
    MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};  /* MQTT packet structure */
    unsigned char *dataPtr;                               /* Pointer for received data */
    char authorization_buf[160];                          /* Authorization token buffer */
    _Bool status = 1;                                     /* Connection status */

    /* Generate authorization token */
    OneNET_Authorization("2018-10-31", OneNet_Product_ID, 1956499200, OneNet_Access_KEY, OneNet_Device_NAME,
                        authorization_buf, sizeof(authorization_buf), 0);

    /* Debug output */
    UsartPrintf(USART_DEBUG, "OneNET_DevLink\r\nNAME: %s, Product_ID: %s\r\n", 
                OneNet_Device_NAME, OneNet_Product_ID);

    /* Create and send MQTT CONNECT packet */
    if(MQTT_PacketConnect(OneNet_Product_ID, authorization_buf, OneNet_Device_NAME, 
                        256, 1, MQTT_QOS_LEVEL0, NULL, NULL, 0, &mqttPacket) == 0)
    {
        ESP8266_SendData(mqttPacket._data, mqttPacket._len);  /* Send to platform */
        
        dataPtr = ESP8266_GetIPD(250);                         /* Wait for platform response */
        if(dataPtr != NULL)
        {
            if(MQTT_UnPacketRecv(dataPtr) == MQTT_PKT_CONNACK)
            {
                switch(MQTT_UnPacketConnectAck(dataPtr))
                {
                    case 0:
                        UsartPrintf(USART_DEBUG, "Connected to OneNET successfully\r\n");
                        status = 0;
                        break;
                    
                    case 1:
                        UsartPrintf(USART_DEBUG, "Connection failed: Protocol error\r\n");
                        break;
                    case 2:
                        UsartPrintf(USART_DEBUG, "Connection failed: Invalid client ID\r\n");
                        break;
                    case 3:
                        UsartPrintf(USART_DEBUG, "Connection failed: Server unavailable\r\n");
                        break;
                    case 4:
                        UsartPrintf(USART_DEBUG, "Connection failed: Invalid username/password\r\n");
                        break;
                    case 5:
                        UsartPrintf(USART_DEBUG, "Connection failed: Unauthorized (check token)\r\n");
                        break;
                    
                    default:
                        UsartPrintf(USART_DEBUG, "Connection failed: Unknown reason\r\n");
                        break;
                }
            }
        }
        
        MQTT_DeleteBuffer(&mqttPacket);  /* Free packet buffer */
    }
    else
        UsartPrintf(USART_DEBUG, "MQTT_PacketConnect Failed\r\n");
    
    return status;
}

/**
 * @brief Fill the buffer with sensor data in OneNET property post JSON format
 * @param buf Buffer to store the JSON data
 * @return Length of the filled buffer
 */
unsigned char OneNet_FillBuf(char *buf)
{
    char text[64];

    memset(text, 0, sizeof(text));

    /* JSON format: {"method":"thing.property.post","id":"xxx","params":{...},"version":"1.0"} */
    strcpy(buf, "{\"method\":\"thing.property.post\",\"id\":\"1\",\"params\":{");

    /* Add human sensor data - OneNET property: Pir (bool type) */
    memset(text, 0, sizeof(text));
    sprintf(text, "\"Pir\":{\"value\":%s}", human ? "false" : "true");
    strcat(buf, text);

    strcat(buf, "},\"version\":\"1.0\"}");

    /* Debug: Print the JSON data */
    UsartPrintf(USART_DEBUG, "Send JSON: %s\r\n", buf);

    return strlen(buf);
}


/**
 * @brief Send sensor data to OneNET platform
 * 
 * This function packages sensor data into MQTT publish packet
 * and sends it to OneNET platform via the topic:
 * $sys/{product_id}/{device_name}/thing/property/post
 */
void OneNet_SendData(void)
{
    MQTT_PACKET_STRUCTURE mqttPacket = {NULL, 0, 0, 0};  /* MQTT packet structure */
    char buf[256];                                        /* Data buffer for JSON payload */
    short body_len = 0, i = 0;                            /* Length variables */

    UsartPrintf(USART_DEBUG, "OneNet_SendData-MQTT\r\n");

    memset(buf, 0, sizeof(buf));

    /* Fill buffer with sensor data in JSON format */
    body_len = OneNet_FillBuf(buf);

    if(body_len)
    {
        /* Create MQTT publish packet */
        if(MQTT_PacketSaveData(OneNet_Product_ID, OneNet_Device_NAME, body_len, NULL, &mqttPacket) == 0)
        {
            /* Copy JSON data to MQTT packet payload */
            for(; i < body_len; i++)
                mqttPacket._data[mqttPacket._len++] = buf[i];
		
            /* Send data to OneNET platform */
            ESP8266_SendData(mqttPacket._data, mqttPacket._len);
            UsartPrintf(USART_DEBUG, "Sent %d Bytes\r\n", mqttPacket._len);
            
            /* Free MQTT packet buffer */
            MQTT_DeleteBuffer(&mqttPacket);
        }
        else
            UsartPrintf(USART_DEBUG, "Failed to create MQTT packet\r\n");
    }
}

/**
 * @brief Publish a message to a specific MQTT topic
 * 
 * @param topic MQTT topic to publish to
 * @param msg Message content to publish
 */
void OneNET_Publish(const char *topic, const char *msg)
{
    MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};  /* MQTT packet structure */
	
	UsartPrintf(USART_DEBUG, "Publish Topic: %s, Msg: %s\r\n", topic, msg);
	
	if(MQTT_PacketPublish(MQTT_PUBLISH_ID, topic, msg, strlen(msg), MQTT_QOS_LEVEL0, 0, 1, &mqtt_packet) == 0)
	{
		ESP8266_SendData(mqtt_packet._data, mqtt_packet._len);					//Send to platform
		
		MQTT_DeleteBuffer(&mqtt_packet);										//Delete
	}

}

//==========================================================
//	Function:	OneNET_Subscribe
//
//	Description:	Subscribe to MQTT topic
//
//	Input:		None
//
//	Output:		None
//
//	Note:		
//==========================================================
void OneNET_Subscribe(void)
{
	
	MQTT_PACKET_STRUCTURE mqtt_packet = {NULL, 0, 0, 0};						//Packet structure
	
	char topic_buf[56];
	const char *topic = topic_buf;
	snprintf(topic_buf, sizeof(topic_buf), "$sys/%s/%s/thing/service/%s/invoke", OneNet_Product_ID, OneNet_Device_NAME, SERVER_NAME_LED);
	
	UsartPrintf(USART_DEBUG, "Subscribe Topic: %s\r\n", topic_buf);
	
	if(MQTT_PacketSubscribe(MQTT_SUBSCRIBE_ID, MQTT_QOS_LEVEL0, &topic, 1, &mqtt_packet) == 0)
	{
		ESP8266_SendData(mqtt_packet._data, mqtt_packet._len);					//Send to platform
		
		MQTT_DeleteBuffer(&mqtt_packet);										//Delete
	}

}

//==========================================================
//	Function:	OneNet_RevPro
//
//	Description:	Process data from platform
//
//	Input:		dataPtr - data received from platform
//
//	Output:		None
//
//	Note:		
//==========================================================
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
		case MQTT_PKT_PUBLISH:																//Receive Publish message
		
			result = MQTT_UnPacketPublish(cmd, &cmdid_topic, &topic_len, &req_payload, &req_len, &qos, &pkt_id);
			if(result == 0)
			{
				char *data_ptr = NULL;
				const char* ctl_led_start = strstr(req_payload, "\"ctl_led\":");			
				const char* ctl_led_value_start = ctl_led_start + strlen("\"ctl_led\":");
				
				UsartPrintf(USART_DEBUG, "topic: %s, topic_len: %d, payload: %s, payload_len: %d\r\n",
																	cmdid_topic, topic_len, req_payload, req_len);

				if (ctl_led_start == NULL) {
					UsartPrintf(USART_DEBUG,"No 'ctl_led' value found\n");
					return;
				}
				// Handle "ctl_led" field value

				if (strncmp(ctl_led_value_start, "true", 4) == 0) {
					UsartPrintf(USART_DEBUG,"ctl_led is true\n");
					
				} else {
					UsartPrintf(USART_DEBUG,"ctl_led is false\n");
					
				}
				

				data_ptr = strstr(cmdid_topic, "request/");									//find cmdid
				if(data_ptr)
				{
					char topic_buf[80], cmdid[40];
					
					data_ptr = strchr(data_ptr, '/');
					data_ptr++;
					
					memcpy(cmdid, data_ptr, 36);											//copy cmdid
					cmdid[36] = 0;
					
					snprintf(topic_buf, sizeof(topic_buf), "$sys/%s/%s/thing/service/%s/invoke_reply",
															OneNet_Product_ID, OneNet_Device_NAME, SERVER_NAME_LED);

					OneNET_Publish(topic_buf, "ojbk");										//send response
				}
			}
			break;
		case MQTT_PKT_PUBACK:														//Publish message acknowledged by platform
		
			if(MQTT_UnPacketPublishAck(cmd) == 0)
				UsartPrintf(USART_DEBUG, "Tips:	MQTT Publish Send OK\r\n");
			
		break;
		
		case MQTT_PKT_SUBACK:																//Subscribe message acknowledged
		
			if(MQTT_UnPacketSubscribe(cmd) == 0)
				UsartPrintf(USART_DEBUG, "Tips:	MQTT Subscribe OK\r\n");
			else
				UsartPrintf(USART_DEBUG, "Tips:	MQTT Subscribe Err\r\n");
		
		break;
		
		default:
			result = -1;
		break;
	}
	
	ESP8266_Clear();									//Clear buffer
	
	if(result == -1)
		return;
	

	if(type == MQTT_PKT_CMD || type == MQTT_PKT_PUBLISH)
	{
		MQTT_FreeBuffer(cmdid_topic);
		MQTT_FreeBuffer(req_payload);
	}

}
