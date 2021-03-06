#include "aws_iot_task.h"
#include "aws_iot_shadow_json.h"
#include "aws_iot_json_utils.h"
#include "aws_iot_shadow_key.h"
extern const uint8_t aws_root_ca_pem_start[] asm("_binary_aws_root_ca_pem_start");
extern const uint8_t aws_root_ca_pem_end[] asm("_binary_aws_root_ca_pem_end");
extern const uint8_t certificate_pem_crt_start[] asm("_binary_certificate_pem_crt_start");
extern const uint8_t certificate_pem_crt_end[] asm("_binary_certificate_pem_crt_end");
extern const uint8_t private_pem_key_start[] asm("_binary_private_pem_key_start");
extern const uint8_t private_pem_key_end[] asm("_binary_private_pem_key_end");

IoT_Error_t rc = FAILURE;
AWS_IoT_Client subscribe_client;
AWS_IoT_Client shadow_client;

bool switchOn = false;
char JsonDocumentBuffer[MAX_LENGTH_OF_UPDATE_JSON_BUFFER];
size_t sizeOfJsonDocumentBuffer = sizeof(JsonDocumentBuffer) / sizeof(JsonDocumentBuffer[0]);
jsonStruct_t led_status_json;

void ShadowUpdateStatusCallback(const char *pThingName, ShadowActions_t action, Shadow_Ack_Status_t status,
								const char *pReceivedJsonDocument, void *pContextData) {
	IOT_UNUSED(pThingName);
	IOT_UNUSED(action);
	IOT_UNUSED(pReceivedJsonDocument);
	IOT_UNUSED(pContextData);

	if(SHADOW_ACK_TIMEOUT == status) {
		IOT_INFO("Update Timeout--");
	} else if(SHADOW_ACK_REJECTED == status) {
		IOT_INFO("Update RejectedXX");
	} else if(SHADOW_ACK_ACCEPTED == status) {
		IOT_INFO("Update Accepted !!");
	}
}
void setLedOn(bool on){
    IOT_INFO("setLedOn: %d", on);
    if (on) {
	    gpio_set_level(GPIO_NUM_2, 0);
	} else {
	    gpio_set_level(GPIO_NUM_2, 1);
	}
}
jsmntok_t *findToken(const char *key, const char *jsonString, jsmntok_t *token) {
	jsmntok_t *result = token;
	int i;

	if(token->type != JSMN_OBJECT) {
		IOT_WARN("Token was not an object.");
		return NULL;
	}

	if(token->size == 0) {
		return NULL;
	}

	result = token + 1;

	for (i = 0; i < token->size; i++) {
		if (0 == jsoneq(jsonString, result, key)) {
			return result + 1;
		}

		int propertyEnd = (result + 1)->end;
		result += 2;
		while (result->start < propertyEnd)
			result++;
	}

	return NULL;
}
void ShadowGetStatusCallback(const char *pThingName, ShadowActions_t action, Shadow_Ack_Status_t status,
                            								const char *pReceivedJsonDocument, void *pContextData) {
    IOT_UNUSED(pThingName);
    IOT_UNUSED(action);
    IOT_UNUSED(pContextData);
    if(SHADOW_ACK_TIMEOUT == status) {
        IOT_INFO("Get Timeout--");
    } else if(SHADOW_ACK_REJECTED == status) {
        IOT_INFO("Get RejectedXX");
    } else if(SHADOW_ACK_ACCEPTED == status) {
        IOT_INFO("Get Accepted !!");
        IOT_INFO("Get Shadow: %s", pReceivedJsonDocument);
        int32_t tokenCount;
        if(!isJsonValidAndParse(pReceivedJsonDocument, NULL, &tokenCount)) {
            IOT_WARN("Received JSON is not valid");
            return;
        }
        IoT_Error_t ret_val = SUCCESS;
        jsmn_parser test_parser;
        jsmn_init(&test_parser);
        jsmntok_t jsonTokenStruct[MAX_JSON_TOKEN_EXPECTED];
        jsmn_parse(&test_parser, pReceivedJsonDocument, strlen(pReceivedJsonDocument), jsonTokenStruct, sizeof(jsonTokenStruct) / sizeof(jsonTokenStruct[0]));

        findToken("on", pReceivedJsonDocument, jsonTokenStruct);
        ret_val = parseBooleanValue(&switchOn, pReceivedJsonDocument, jsonTokenStruct);
        setLedOn(switchOn);
       }
    }


void iot_subscribe_callback_handler(AWS_IoT_Client *pClient, char *topicName, uint16_t topicNameLen,
									IoT_Publish_Message_Params *params, void *pData) {
	IOT_UNUSED(pData);
	IOT_UNUSED(pClient);
	IOT_INFO("Subscribe callback");
	IOT_INFO("%.*s\t%.*s", topicNameLen, topicName, (int) params->payloadLen, (char *) params->payload);

    switchOn = !switchOn;
	setLedOn(switchOn);
}

void disconnectCallbackHandler(AWS_IoT_Client *pClient, void *data) {
	IOT_WARN("MQTT Disconnect");
	IoT_Error_t rc = FAILURE;

	if(NULL == pClient) {
		return;
	}

	IOT_UNUSED(data);

	if(aws_iot_is_autoreconnect_enabled(pClient)) {
		IOT_INFO("Auto Reconnect is enabled, Reconnecting attempt will start now");
	} else {
		IOT_WARN("Auto Reconnect not enabled. Starting manual reconnect...");
		rc = aws_iot_mqtt_attempt_reconnect(pClient);
		if(NETWORK_RECONNECTED == rc) {
			IOT_WARN("Manual Reconnect Successful");
		} else {
			IOT_WARN("Manual Reconnect Failed - %d", rc);
		}
	}
}

void init_shadow(){

	ShadowInitParameters_t sp = ShadowInitParametersDefault;
	sp.pHost = AWS_IOT_MQTT_HOST;
	sp.port = AWS_IOT_MQTT_PORT;
	sp.pClientCRT = (const char *)certificate_pem_crt_start;
	sp.pClientKey = (const char *)private_pem_key_start;
	sp.pRootCA = (const char *)aws_root_ca_pem_start;
	sp.enableAutoReconnect = false;
	sp.disconnectHandler = NULL;

	IOT_INFO("Shadow Init");
	rc = aws_iot_shadow_init(&shadow_client, &sp);
	if(SUCCESS != rc) {
		IOT_ERROR("Shadow Connection Error");
		return ;
	}

	ShadowConnectParameters_t scp = ShadowConnectParametersDefault;
	scp.pMyThingName = AWS_IOT_MY_THING_NAME;
	scp.pMqttClientId = AWS_IOT_SHADOW_CLIENT_ID;
	scp.mqttClientIdLen = (uint16_t) strlen(AWS_IOT_SHADOW_CLIENT_ID);

	IOT_INFO("Shadow Connect");
	rc = aws_iot_shadow_connect(&shadow_client, &scp);
	if(SUCCESS != rc) {
		IOT_ERROR("Shadow Connection Error");
		return ;
	}
	rc = aws_iot_shadow_set_autoreconnect_status(&shadow_client, true);
	if(SUCCESS != rc) {
		IOT_ERROR("Unable to set Auto Reconnect to true - %d", rc);
		return;
	}
	rc = aws_iot_shadow_get(&shadow_client, AWS_IOT_MY_THING_NAME, ShadowGetStatusCallback, NULL, 4, true);
}

void init_mqtt()
{
	IoT_Client_Init_Params mqttInitParams = iotClientInitParamsDefault;
	mqttInitParams.enableAutoReconnect = false; // We enable this later below
	mqttInitParams.pHostURL = AWS_IOT_MQTT_HOST;
	mqttInitParams.port = AWS_IOT_MQTT_PORT;
    mqttInitParams.pRootCALocation = (const char *)aws_root_ca_pem_start;
    mqttInitParams.pDeviceCertLocation = (const char *)certificate_pem_crt_start;
    mqttInitParams.pDevicePrivateKeyLocation = (const char *)private_pem_key_start;

	mqttInitParams.mqttCommandTimeout_ms = 20000;
	mqttInitParams.tlsHandshakeTimeout_ms = 5000;
	mqttInitParams.isSSLHostnameVerify = true;
	mqttInitParams.disconnectHandler = disconnectCallbackHandler;
	mqttInitParams.disconnectHandlerData = NULL;

	rc = aws_iot_mqtt_init(&subscribe_client, &mqttInitParams);
	if(SUCCESS != rc) {
		IOT_ERROR("aws_iot_mqtt_init returned error : %d ", rc);
		return ;
	}

	IoT_Client_Connect_Params connectParams = iotClientConnectParamsDefault;
	connectParams.keepAliveIntervalInSec = 600;
    connectParams.isCleanSession = true;
    connectParams.MQTTVersion = MQTT_3_1_1;
    connectParams.pClientID = AWS_IOT_MQTT_CLIENT_ID;
    connectParams.clientIDLen = (uint16_t) strlen(AWS_IOT_MQTT_CLIENT_ID);
    connectParams.isWillMsgPresent = false;

    IOT_INFO("Connecting...");
    rc = aws_iot_mqtt_connect(&subscribe_client, &connectParams);
    if(SUCCESS != rc) {
        IOT_ERROR("Error(%d) connecting to %s:%d", rc, mqttInitParams.pHostURL, mqttInitParams.port);
        return ;
    }
    rc = aws_iot_mqtt_autoreconnect_set_status(&subscribe_client, true);
    if(SUCCESS != rc) {
        IOT_ERROR("Unable to set Auto Reconnect to true - %d", rc);
        return ;
    }
}

void init_led()
{

    gpio_pad_select_gpio(GPIO_NUM_2);
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_2, 0);
}
void subscribe_to_button_switch_topic()
{
    IOT_INFO("Subscribing...");
	rc = aws_iot_mqtt_subscribe(&subscribe_client, "esp32Button/switch", 18, QOS0, iot_subscribe_callback_handler, NULL);
	if(SUCCESS != rc) {
		IOT_ERROR("Error subscribing : %d ", rc);
		return ;
	}
}
void aws_iot_task(void *param) {
    init_led();
    init_mqtt();
    subscribe_to_button_switch_topic();
    init_shadow();
	while(NETWORK_ATTEMPTING_RECONNECT == rc || NETWORK_RECONNECTED == rc || SUCCESS == rc) {
		rc = aws_iot_mqtt_yield(&subscribe_client, 100);
		rc = aws_iot_shadow_yield(&shadow_client, 100);
		if(NETWORK_ATTEMPTING_RECONNECT == rc) {
			continue;
		}
		rc = aws_iot_shadow_init_json_document(JsonDocumentBuffer, sizeOfJsonDocumentBuffer);
        if(SUCCESS == rc) {
            led_status_json.cb = NULL;
            led_status_json.pKey = "on";
            led_status_json.pData = &switchOn;
            led_status_json.type = SHADOW_JSON_BOOL;
            rc = aws_iot_shadow_add_reported(JsonDocumentBuffer, sizeOfJsonDocumentBuffer, 1, &led_status_json);
            if(SUCCESS == rc) {
                rc = aws_iot_finalize_json_document(JsonDocumentBuffer, sizeOfJsonDocumentBuffer);
                if(SUCCESS == rc) {
                    IOT_INFO("Update Shadow: %s", JsonDocumentBuffer);
                    rc = aws_iot_shadow_update(&shadow_client, AWS_IOT_MY_THING_NAME, JsonDocumentBuffer,
                                               ShadowUpdateStatusCallback, NULL, 4, true);
                }
            }
        }
		IOT_INFO("-->sleep");
		sleep(3);
	}
    IOT_INFO("Err: %d",rc);
	return ;
}
