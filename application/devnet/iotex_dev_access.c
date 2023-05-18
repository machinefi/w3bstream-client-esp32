#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "iotex_dev_access.h"

#include "base64/base64.h"
#include "cJSON/cJSON.h"
#include "keccak256/keccak256.h"

#include "ProtoBuf/event.pb.h"
#include "ProtoBuf/pb_common.h"
#include "ProtoBuf/pb_decode.h"
#include "ProtoBuf/pb_encode.h"

#include "psa_layer/svc/crypto.h"

extern psa_key_id_t key_id;

iotex_dev_ctx_t *dev_ctx = NULL;

#if 0
static char str2Hex(char c)
{
    if (c >= '0' && c <= '9') {
        return (c - '0');
    }

    if (c >= 'a' && c <= 'z') {
        return (c - 'a' + 10);
    }

    if (c >= 'A' && c <= 'Z') {
        return (c -'A' + 10);
    }
    return c;
}

static int iotex_hexStr_2_Bin(char *str, char *bin) {
    int i,j;
    for(i = 0,j = 0; j < (strlen(str)>>1) ; i++,j++)
    {
        bin[j] = (str2Hex(str[i]) <<4);
        i++;
        bin[j] |= str2Hex(str[i]);
    }
    return j;
}
#endif

static void iotex_hex2str(uint8_t *input, uint16_t input_len, char *output)
{
    char *hexEncode = "0123456789ABCDEF";
    int i = 0, j = 0;

    for (i = 0; i < input_len; i++) {
        output[j++] = hexEncode[(input[i] >> 4) & 0xf];
        output[j++] = hexEncode[(input[i]) & 0xf];
    }
}

int iotex_dev_access_init(void)
{
	if (NULL == dev_ctx)
		dev_ctx = (iotex_dev_ctx_t *)malloc(sizeof(iotex_dev_ctx_t));

	if (NULL == dev_ctx)
		return IOTEX_DEV_ACCESS_ERR_ALLOCATE_FAIL;

	memset(dev_ctx, 0, sizeof(iotex_dev_ctx_t));
	memcpy(dev_ctx->mqtt_ctx.topic[0], IOTEX_MQTT_TOPIC_DEFAULT, strlen(IOTEX_MQTT_TOPIC_DEFAULT));

	dev_ctx->inited = 1;

    return IOTEX_DEV_ACCESS_ERR_SUCCESS;
}

int iotex_dev_access_set_mqtt_topic(const char *topic, int topic_len, int topic_location) {
    if( (NULL == dev_ctx) || (0 == dev_ctx->inited) )
        return IOTEX_DEV_ACCESS_ERR_NO_INIT;

    if( NULL == topic )
        return IOTEX_DEV_ACCESS_ERR_BAD_INPUT_PARAMETER;

    if( (topic_location >= IOTEX_MAX_TOPIC_NUM) || (topic_len > IOTEX_MAX_TOPIC_SIZE) )
        return IOTEX_DEV_ACCESS_ERR_BAD_INPUT_PARAMETER;

    memset(dev_ctx->mqtt_ctx.topic[topic_location], 0, IOTEX_MAX_TOPIC_SIZE);
    memcpy(dev_ctx->mqtt_ctx.topic[topic_location], topic, strlen(topic));

#ifdef IOTEX_DEBUG_ENABLE
    if( dev_ctx->debug_enable ) {
        printf("Success to set mqtt topic: \n");
        printf("topic : %s\n", dev_ctx->mqtt_ctx.topic[topic_location]);
    }
#endif
    return IOTEX_DEV_ACCESS_ERR_SUCCESS;
}

int iotex_dev_access_set_time_func(iotex_gettime get_time_func) {

    if( (NULL == dev_ctx) || (0 == dev_ctx->inited) ) {
        return IOTEX_DEV_ACCESS_ERR_NO_INIT;
    }

 	dev_ctx->get_time_func = get_time_func;

   	return IOTEX_DEV_ACCESS_ERR_SUCCESS;
}

int iotex_dev_access_set_mqtt_func(iotex_mqtt_pub mqtt_pub, iotex_mqtt_sub mqtt_sub) {

    if( (NULL == dev_ctx) || (0 == dev_ctx->inited) ) {
        return IOTEX_DEV_ACCESS_ERR_NO_INIT;
    }

    dev_ctx->mqtt_ctx.mqtt_pub_func = mqtt_pub;
    dev_ctx->mqtt_ctx.mqtt_sub_func = mqtt_sub;

    return IOTEX_DEV_ACCESS_ERR_SUCCESS;
}

int iotex_dev_access_set_sign_func(iotex_sign_message sign_func) {

    if( (NULL == dev_ctx) || (0 == dev_ctx->inited) ) {
        return IOTEX_DEV_ACCESS_ERR_NO_INIT;
    }

    dev_ctx->crypto_ctx.sign_func = sign_func;

    return IOTEX_DEV_ACCESS_ERR_SUCCESS;
}

#ifdef IOTEX_DEBUG_ENABLE
int iotex_dev_access_set_verify_func(iotex_verify_message verify_func) {

    if( (NULL == dev_ctx) || (0 == dev_ctx->inited) ) {
        return IOTEX_DEV_ACCESS_ERR_NO_INIT;
    }

    dev_ctx->crypto_ctx.verify_func = verify_func;

    return IOTEX_DEV_ACCESS_ERR_SUCCESS;
}
#endif

static int iotex_dev_access_send_data(unsigned char *buf, unsigned int buflen) {

    int ret = 0;

    if( NULL == dev_ctx || 0 == dev_ctx->inited )
        return IOTEX_DEV_ACCESS_ERR_NO_INIT;

    if( (NULL == buf) || (0 == buflen))
        return IOTEX_DEV_ACCESS_ERR_BAD_INPUT_PARAMETER;

    if( NULL == dev_ctx->mqtt_ctx.mqtt_pub_func )
        return IOTEX_DEV_ACCESS_ERR_MQTT_PUB_FUNC_EMPTY;

    ret = dev_ctx->mqtt_ctx.mqtt_pub_func((unsigned char *)dev_ctx->mqtt_ctx.topic[0], buf, buflen, 0);
#ifdef IOTEX_DEBUG_ENABLE
    if( dev_ctx->debug_enable ) {
        printf("Sending  [%s]: %d\n", dev_ctx->mqtt_ctx.topic[0], ret);
    }
#endif

    return (ret ? IOTEX_DEV_ACCESS_ERR_SUCCESS : IOTEX_DEV_ACCESS_ERR_SEND_DATA_FAIL);
}

int iotex_dev_access_mqtt_input(uint8_t *topic, uint8_t *payload, uint32_t len) {

//    const cJSON *status   = NULL;
//    const cJSON *proposer = NULL;

    if( NULL == dev_ctx || 0 == dev_ctx->inited )
        return IOTEX_DEV_ACCESS_ERR_NO_INIT;

    if( topic == NULL || payload == NULL || len == 0)
        return IOTEX_DEV_ACCESS_ERR_BAD_INPUT_PARAMETER;

#if 0
    cJSON *monitor_json = cJSON_Parse((const char *)payload);
    if (monitor_json == NULL)
    {
#ifdef IOTEX_DEBUG_ENABLE
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            printf("Error before: %s\n", error_ptr);
        }
#endif

        return IOTEX_DEV_ACCESS_ERR_JSON_FAIL;
    }

    status = cJSON_GetObjectItemCaseSensitive(monitor_json, "status");
    if (!cJSON_IsNumber(status)) {
		status = 0;
		return IOTEX_DEV_ACCESS_ERR_JSON_FAIL;
    }

    dev_ctx->status = status->valueint;

    switch (dev_ctx->status)
    {
        case 0:
            /* code */
            break;
        case 1:

            proposer = cJSON_GetObjectItemCaseSensitive(monitor_json, "proposer");
            if (cJSON_IsString(proposer) && (proposer->valuestring != NULL)) {
                memcpy(dev_ctx->crypto_ctx.wallet_addr, proposer->valuestring, strlen(proposer->valuestring));
#ifdef IOTEX_DEBUG_ENABLE
                printf("Wallet Addr %s\n", dev_ctx->crypto_ctx.wallet_addr);
#endif
            }

            break;
        case 2:

            printf("Dev register success\n");

            break;
        default:
            break;
    }

    cJSON_Delete(monitor_json);
#endif

    return IOTEX_DEV_ACCESS_ERR_SUCCESS;

}

int iotex_dev_access_data_upload_example_with_protobuf(char *buf, int buflen) {

	char sign_buf[64]  = {0};
	char sign_str[150] = {0};
	unsigned int  sign_len = 0;
	char *json_string = NULL;

	unsigned char *buffer = malloc(Event_size);

	pb_ostream_t ostream_event = {0};
	Event  pb_event  = Event_init_zero;
	cJSON *payload	= cJSON_CreateObject();

	if (NULL == buf || buflen == 0)
		return IOTEX_DEV_ACCESS_ERR_BAD_INPUT_PARAMETER;

	cJSON_AddStringToObject(payload, "user", buf);

 	psa_sign_message( key_id, PSA_ALG_ECDSA(PSA_ALG_SHA_256), (const uint8_t *)buf, buflen, (uint8_t *)sign_buf, 64, &sign_len);

 	iotex_hex2str((uint8_t *)sign_buf, sign_len, sign_str);
 	cJSON_AddStringToObject(payload, "sign", sign_str);

 	json_string = cJSON_PrintUnformatted(payload);

	pb_event.has_header = true;

	strcpy(pb_event.header.event_id, IOTEX_EVENT_ID_DEFAULT);
	strcpy(pb_event.header.pub_id,   IOTEX_PUB_ID_DEFAULT);
	strcpy(pb_event.header.event_type, IOTEX_EVENT_TYPE_DEFAULT);
	strcpy(pb_event.header.token, IOTEX_TOKEN_DEFAULT);
	pb_event.header.pub_time = IOTEX_PUB_TIME_TEST_DEFAULT;


	pb_event.payload.size = strlen(json_string);
	strcpy((char *)pb_event.payload.bytes, json_string);

	memset(buffer, 0, Event_size);
	ostream_event  = pb_ostream_from_buffer(buffer, Event_size);
	if (!pb_encode(&ostream_event, Event_fields, &pb_event)) {
		printf("pb encode [event] error in [%s]\n", PB_GET_ERROR(&ostream_event));
		goto exit;
	}

#ifdef IOTEX_DEBUG_ENABLE
	printf("Event Upload len %d\n", ostream_event.bytes_written);

	for (int i = 0; i < ostream_event.bytes_written; i++) {
		printf("%02x ", buffer[i]);
	}
	printf("\n");
#endif

	iotex_dev_access_send_data(buffer, ostream_event.bytes_written);

exit:
	if(json_string) {
		free(json_string);
		json_string = NULL;
	}

	if(buffer) {
		free(buffer);
		buffer = NULL;
	}

	if(payload) {
		cJSON_Delete(payload);
	}

	return IOTEX_DEV_ACCESS_ERR_SUCCESS;
}

int iotex_dev_access_data_upload_example(void) {

	char base64_buf[500] = {0};
	char time_str[20] = {0};
	int  base64_buf_len  = 0;
	char *json_string = NULL;
	int gyroscope[] = {-26, 9, 11};
	int accelerometer[] = {2658, 3835, 3326};

	time_t now = IOTEX_PUB_TIME_TEST_DEFAULT;

    if ( NULL == dev_ctx || 0 == dev_ctx->inited ) {
        return IOTEX_DEV_ACCESS_ERR_NO_INIT;
    }

	if (dev_ctx->mqtt_ctx.status < IOTEX_MQTT_CONNECTED )
		return IOTEX_DEV_ACCESS_ERR_LINK_NOT_ESTABLISH;

	if ( dev_ctx->get_time_func )
		now = dev_ctx->get_time_func();

	cJSON *w3b_data = cJSON_CreateObject();
	cJSON *message	= cJSON_CreateObject();
	cJSON *payload	= cJSON_CreateObject();
	cJSON *cjson_gyr = cJSON_CreateIntArray(gyroscope, 3);
	cJSON *cjson_acc = cJSON_CreateIntArray(accelerometer, 3);

	cJSON *header	= cJSON_CreateObject();
	cJSON_AddStringToObject(header, "event_type", IOTEX_EVENT_TYPE_DEFAULT);
	cJSON_AddStringToObject(header, "pub_id",     IOTEX_PUB_ID_DEFAULT);
	cJSON_AddStringToObject(header, "token",      IOTEX_TOKEN_DEFAULT);
	cJSON_AddNumberToObject(header, "pub_time",   now);
 	cJSON_AddItemToObject(w3b_data, "header", header);

 	cJSON_AddNumberToObject(message, "snr", 243);
 	cJSON_AddNumberToObject(message, "vbat", 5.5);
 	cJSON_AddNumberToObject(message, "latitude", 79.4);
 	cJSON_AddNumberToObject(message, "longitude", -142.6);
 	cJSON_AddNumberToObject(message, "gasResistance", 6137);
 	cJSON_AddNumberToObject(message, "temperature", 40.2);
 	cJSON_AddNumberToObject(message, "pressure", 689.3);
 	cJSON_AddNumberToObject(message, "humidity", 10.2);
 	cJSON_AddNumberToObject(message, "light", 20.4);

    cJSON_AddItemToObject(message, "gyroscope", cjson_gyr);
    cJSON_AddItemToObject(message, "accelerometer", cjson_acc);

    itoa((int)now, time_str, 10);
 	cJSON_AddStringToObject(message, "timestamp", time_str);
 	cJSON_AddStringToObject(message, "random", "0db1fafe1848fe60");

 	cJSON_AddItemToObject(payload, "message", message);
 	json_string = cJSON_Print(payload);
#ifdef IOTEX_DEBUG_ENABLE
 	if( dev_ctx->debug_enable ) {
 		printf("json payload[%d] : %s\n", strlen(json_string), json_string);
 	}
#endif

 	base64_encode(json_string, strlen(json_string), base64_buf, &base64_buf_len);
#ifdef IOTEX_DEBUG_ENABLE
 	if( dev_ctx->debug_enable ) {
 		printf("base64 payload[%d] : %s\n", strlen(base64_buf), base64_buf);
 	}
#endif

 	cJSON_AddStringToObject(w3b_data, "payload", base64_buf);

 	json_string = cJSON_Print(w3b_data);
#ifdef IOTEX_DEBUG_ENABLE
 	if( dev_ctx->debug_enable ) {
 		printf("json data[%d] : %s\n", strlen(json_string), json_string);
 	}
#endif

 	iotex_dev_access_send_data((unsigned char *)json_string, strlen(json_string));

 	cJSON_Delete(w3b_data);

 	return IOTEX_DEV_ACCESS_ERR_SUCCESS;
}

int iotex_dev_access_data_with_sign_upload_example(unsigned char *pubkey, int pubkey_len) {

	char base64_buf[500] = {0};
//	char time_str[20] = {0};
	int  base64_buf_len  = 0;
	char *json_string = NULL;

	char sign_buf[64]  = {0};
	char sign_str[150] = {0};
	unsigned int  sign_len = 0;

	char pubkey_str[150] = {0};

	time_t now = IOTEX_PUB_TIME_TEST_DEFAULT;

    if ( NULL == dev_ctx || 0 == dev_ctx->inited ) {
        return IOTEX_DEV_ACCESS_ERR_NO_INIT;
    }

	if (dev_ctx->mqtt_ctx.status < IOTEX_MQTT_CONNECTED )
		return IOTEX_DEV_ACCESS_ERR_LINK_NOT_ESTABLISH;

	if ( dev_ctx->get_time_func )
		now = dev_ctx->get_time_func();

#if IOTEX_USE_SIGN_FUNC_EXT
	if ( !dev_ctx->crypto_ctx.sign_func )
		return IOTEX_DEV_ACCESS_ERR_SIGN_FUNC_EMPTY;
#endif

	cJSON *w3b_data = cJSON_CreateObject();
	cJSON *sensor	= cJSON_CreateObject();
//	cJSON *sign  	= cJSON_CreateObject();
	cJSON *payload	= cJSON_CreateObject();


	cJSON *header	= cJSON_CreateObject();
	cJSON_AddStringToObject(header, "event_type", IOTEX_EVENT_TYPE_DEFAULT);
	cJSON_AddStringToObject(header, "pub_id",     IOTEX_PUB_ID_DEFAULT);
	cJSON_AddStringToObject(header, "token",      IOTEX_TOKEN_DEFAULT);
	cJSON_AddNumberToObject(header, "pub_time",   now);
 	cJSON_AddItemToObject(w3b_data, "header", header);

 	// sensor data
 	cJSON_AddNumberToObject(sensor, "snr", 243);
 	cJSON_AddNumberToObject(sensor, "vbat", 5.5);

 	cJSON_AddItemToObject(payload, "sensor", sensor);

 	// sign data
#if IOTEX_USE_SIGN_FUNC_EXT
 	dev_ctx->crypto_ctx.sign_func((const uint8_t *)"iotex_sample", strlen("iotex_sample"), (uint8_t *)sign_buf, &sign_len);
#else
 	psa_sign_message( key_id, PSA_ALG_ECDSA(PSA_ALG_SHA_256), (const uint8_t *)"iotex_sample", strlen("iotex_sample"), (uint8_t *)sign_buf, 64, &sign_len);
#endif

 	iotex_hex2str((uint8_t *)sign_buf, sign_len, sign_str);
 	cJSON_AddStringToObject(payload, "sign", sign_str);

 	iotex_hex2str((uint8_t *)pubkey, pubkey_len, pubkey_str);
 	cJSON_AddStringToObject(payload, "pubkey", pubkey_str);

 	json_string = cJSON_PrintUnformatted(payload);
#ifdef IOTEX_DEBUG_ENABLE
 	if( dev_ctx->debug_enable ) {
 		printf("json payload[%d] : %s\n", strlen(json_string), json_string);
 	}
#endif

 	base64_encode(json_string, strlen(json_string), base64_buf, &base64_buf_len);
#ifdef IOTEX_DEBUG_ENABLE
 	if( dev_ctx->debug_enable ) {
 		printf("base64 payload[%d] : %s\n", strlen(base64_buf), base64_buf);
 	}
#endif

 	cJSON_AddStringToObject(w3b_data, "payload", base64_buf);

 	json_string = cJSON_PrintUnformatted(w3b_data);
#ifdef IOTEX_DEBUG_ENABLE
 	if( dev_ctx->debug_enable ) {
 		printf("json data[%d] : %s\n", strlen(json_string), json_string);
 	}
#endif

 	iotex_dev_access_send_data((unsigned char *)json_string, strlen(json_string));

 	cJSON_Delete(w3b_data);

 	return IOTEX_DEV_ACCESS_ERR_SUCCESS;
}

int iotex_dev_access_data_with_pubky_upload_example(unsigned char *pubkey, int pubkey_len) {

	char base64_buf[500] = {0};
	int  base64_buf_len  = 0;
	char *json_string = NULL;

	char pubkey_str[150] = {0};

	time_t now = IOTEX_PUB_TIME_TEST_DEFAULT;

    if ( NULL == dev_ctx || 0 == dev_ctx->inited ) {
        return IOTEX_DEV_ACCESS_ERR_NO_INIT;
    }

	if (dev_ctx->mqtt_ctx.status < IOTEX_MQTT_CONNECTED )
		return IOTEX_DEV_ACCESS_ERR_LINK_NOT_ESTABLISH;

	if ( dev_ctx->get_time_func )
		now = dev_ctx->get_time_func();

	if ( !dev_ctx->crypto_ctx.sign_func )
		return IOTEX_DEV_ACCESS_ERR_SIGN_FUNC_EMPTY;

	cJSON *w3b_data = cJSON_CreateObject();
	cJSON *payload	= cJSON_CreateObject();


	cJSON *header	= cJSON_CreateObject();
	cJSON_AddStringToObject(header, "event_type", IOTEX_EVENT_TYPE_PUBKEY);
	cJSON_AddStringToObject(header, "pub_id",     IOTEX_PUB_ID_DEFAULT);
	cJSON_AddStringToObject(header, "token",      IOTEX_TOKEN_DEFAULT);
	cJSON_AddNumberToObject(header, "pub_time",   now);
 	cJSON_AddItemToObject(w3b_data, "header", header);

 	iotex_hex2str((uint8_t *)pubkey, pubkey_len, pubkey_str);
 	cJSON_AddStringToObject(payload, "pubkey", pubkey_str);

 	json_string = cJSON_PrintUnformatted(payload);
//#ifdef IOTEX_DEBUG_ENABLE
// 	if( dev_ctx->debug_enable ) {
 		printf("json payload[%d] : %s\n", strlen(json_string), json_string);
// 	}
//#endif

 	base64_encode(json_string, strlen(json_string), base64_buf, &base64_buf_len);
#ifdef IOTEX_DEBUG_ENABLE
 	if( dev_ctx->debug_enable ) {
 		printf("base64 payload[%d] : %s\n", strlen(base64_buf), base64_buf);
 	}
#endif

 	cJSON_AddStringToObject(w3b_data, "payload", base64_buf);

 	json_string = cJSON_PrintUnformatted(w3b_data);
#ifdef IOTEX_DEBUG_ENABLE
 	if( dev_ctx->debug_enable ) {
 		printf("json data[%d] : %s\n", strlen(json_string), json_string);
 	}
#endif

 	iotex_dev_access_send_data((unsigned char *)json_string, strlen(json_string));

 	cJSON_Delete(w3b_data);

 	return IOTEX_DEV_ACCESS_ERR_SUCCESS;
}

void iotex_dev_access_loop(void) {

	// TODO: Loop code here
}

int iotex_dev_access_set_mqtt_status(enum IOTEX_MQTT_STATUS status) {

	if( NULL == dev_ctx || 0 == dev_ctx->inited ) {
		return IOTEX_DEV_ACCESS_ERR_NO_INIT;
	}

	dev_ctx->mqtt_ctx.status = status;
#ifdef IOTEX_DEBUG_ENABLE
#ifndef IOTEX_DEBUG_ENABLE_FORCE
	if( dev_ctx->debug_enable ) {
#endif
		printf("mqtt status set %d\n", dev_ctx->mqtt_ctx.status);
#ifndef IOTEX_DEBUG_ENABLE_FORCE
	}
#endif
#endif

	return IOTEX_DEV_ACCESS_ERR_SUCCESS;
}

int iotex_dev_access_debug_enable(int enable) {

    if( NULL == dev_ctx || 0 == dev_ctx->inited )
        return IOTEX_DEV_ACCESS_ERR_NO_INIT;

    dev_ctx->debug_enable = enable;

    return IOTEX_DEV_ACCESS_ERR_SUCCESS;
}

int iotex_dev_access_generate_dev_addr(const unsigned char* public_key, char *dev_address)
{
	// The device address is the hex string representation of the last 20 bytes of the keccak256 hash of the public key.

	uint8_t hash[32] = {0};
	keccak256_getHash(public_key + 1, 64, hash);

    dev_address[0] = '0';
    dev_address[1] = 'x';

	for (int i=0; i<20; i++)
	{
		char buf[3] = {0};
		sprintf(buf, "%02x", hash[32 - 20 + i]);

        memcpy(dev_address + 2 + i * 2, buf, 2);

	}

	return IOTEX_DEV_ACCESS_ERR_SUCCESS;
}


