#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "iotex_dev_access.h"

#include "psa/crypto.h"

extern psa_key_id_t key_id;

iotex_dev_ctx_t *dev_ctx = NULL;

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

void iotex_dev_access_loop(void) {

	// TODO: Loop code here
}

int iotex_dev_access_data_upload_with_userdate(void *buf, size_t buf_len, enum UserData_Type type) {

	char sign_buf[64]  = {0};
	unsigned int  sign_len = 0;
	char *message = NULL;
	size_t message_len = 0;

	unsigned char *buffer = malloc(Upload_size);
	if (NULL == buffer)
		return IOTEX_DEV_ACCESS_ERR_ALLOCATE_FAIL;

	if (NULL == buf || 0 == buf_len)
		return IOTEX_DEV_ACCESS_ERR_BAD_INPUT_PARAMETER;

	pb_ostream_t ostream_upload = {0};
	Upload upload = Upload_init_zero;

	upload.has_header = true;
	strcpy(upload.header.event_id, IOTEX_EVENT_ID_DEFAULT);
	strcpy(upload.header.pub_id,   IOTEX_PUB_ID_DEFAULT);
	strcpy(upload.header.event_type, IOTEX_EVENT_TYPE_DEFAULT);
	strcpy(upload.header.token, IOTEX_TOKEN_DEFAULT);
	upload.header.pub_time = IOTEX_PUB_TIME_TEST_DEFAULT;

 	upload.payload.type = type;

 	switch (type) {

 		case IOTEX_USER_DATA_TYPE_JSON:

 			message = cJSON_PrintUnformatted((const cJSON *)buf);
 			message_len = strlen(message);

 			upload.payload.user.size = message_len;
 			memcpy(upload.payload.user.bytes, message, message_len);

 			break;
 		case IOTEX_USER_DATA_TYPE_PB:

 			upload.payload.user.size = buf_len;
 			memcpy(upload.payload.user.bytes, buf, buf_len);

 			message = (char *)buf;
 			message_len = buf_len;

 			break;
 		case IOTEX_USER_DATA_TYPE_RAW:

 			upload.payload.user.size = buf_len;
			memcpy(upload.payload.user.bytes, buf, buf_len);

 			message = (char *)buf;
 			message_len = buf_len;

 			break;
 		default:
 			return IOTEX_DEV_ACCESS_ERR_BAD_INPUT_PARAMETER;
 	}

 	psa_sign_message( key_id, PSA_ALG_ECDSA(PSA_ALG_SHA_256), (const uint8_t *)message, message_len, (uint8_t *)sign_buf, 64, &sign_len);

 	upload.has_payload = true;
 	upload.payload.sign.size = sign_len;
 	memcpy(upload.payload.sign.bytes, sign_buf, sign_len);


	memset(buffer, 0, Upload_size);
	ostream_upload  = pb_ostream_from_buffer(buffer, Upload_size);
	if (!pb_encode(&ostream_upload, Upload_fields, &upload)) {
		printf("pb encode [event] error in [%s]\n", PB_GET_ERROR(&ostream_upload));
		goto exit;
	}

#ifdef IOTEX_DEBUG_ENABLE
	printf("Event Upload len %d\n", ostream_upload.bytes_written);

	for (int i = 0; i < ostream_upload.bytes_written; i++) {
		printf("%02x ", buffer[i]);
	}
	printf("\n");
#endif

	iotex_dev_access_send_data(buffer, ostream_upload.bytes_written);

exit:
	if(buffer) {
		free(buffer);
		buffer = NULL;
	}

	if (IOTEX_USER_DATA_TYPE_JSON == type &&  message) {
		free(message);
		message = NULL;
	}

	return IOTEX_DEV_ACCESS_ERR_SUCCESS;
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


