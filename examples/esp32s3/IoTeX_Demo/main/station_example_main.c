/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "esp_random.h"

#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "mqtt_client.h"
#include "wsiotsdk.h"

/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID      CONFIG_ESP_WIFI_SSID
#define EXAMPLE_ESP_WIFI_PASS      CONFIG_ESP_WIFI_PASSWORD
#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY

#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif


#define KEY_BITS  256
#define IOTEX_DEBUG_ENABLE
//#define IOTEX_DEBUG_ENABLE_EXT

#define IOTEX_SIGNKEY_USE_NONE			    0x00
#define IOTEX_SIGNKEY_USE_STATIC_DATA       0x01
#define IOTEX_SIGNKEY_USE_EEPROM            0x02
#define IOTEX_SIGNKEY_USE_PRNG              0x04

#define IOTEX_SIGNKEY_USE_MODE              IOTEX_SIGNKEY_USE_STATIC_DATA

#define IOTEX_SIGNKEY_ECC_MODE				PSA_ECC_FAMILY_SECP_K1

#if ( IOTEX_SIGNKEY_USE_MODE == IOTEX_SIGNKEY_USE_PRNG )

#define IOTEX_SEED_USER_DEFINE              69834

#endif

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "wifi station";

static int s_retry_num = 0;

psa_key_id_t key_id = 0;

esp_mqtt_client_handle_t mqtt_client = NULL;

static const uint8_t private_key[] = {0xa1, 0x73, 0x6f, 0xbf, 0x37, 0xa2, 0xfc, 0xb8, 0xfe, 0xe2, 0x02, 0xdb, 0x0c, 0x63, 0x91, 0xdf, 0xa4, 0x61, 0x86, 0x29, 0xb1, 0x86, 0xa6, 0x90, 0x65, 0x85, 0x2d, 0xfc, 0xd8, 0x8f, 0x58, 0x19};
uint8_t  exported[PSA_KEY_EXPORT_ECC_PUBLIC_KEY_MAX_SIZE(KEY_BITS)];
uint32_t exported_len;
/*
static void iotex_hex2str(uint8_t *input, uint16_t input_len, char *output)
{
    char *hexEncode = "0123456789ABCDEF";
    int i = 0, j = 0;

    for (i = 0; i < input_len; i++) {
        output[j++] = hexEncode[(input[i] >> 4) & 0xf];
        output[j++] = hexEncode[(input[i]) & 0xf];
    }
}
*/

static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (pasword len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
	     * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

/*
 * @brief Event handler registered to receive MQTT events
 *
 *  This function is called by the MQTT client event loop.
 *
 * @param handler_args user data registered to the event.
 * @param base Event base for the handler(always MQTT Base in this example).
 * @param event_id The id for the received event.
 * @param event_data The data for the event, esp_mqtt_event_handle_t.
 */
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
#if 0
        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/qos1", 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
        ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
#endif

        msg_id = esp_mqtt_client_subscribe(client, IOTEX_MQTT_SUB_TOPIC_DEFAULT, 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        iotex_dev_access_set_mqtt_status(IOTEX_MQTT_CONNECTED);

        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        iotex_dev_access_set_mqtt_status(IOTEX_MQTT_DISCONNECTED);
        break;

    case MQTT_EVENT_SUBSCRIBED:

    	ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
#if 0
        msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
#endif
        iotex_dev_access_set_mqtt_status(IOTEX_MQTT_SUB_COMPLATED);

        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);

//        iotex_dev_access_mqtt_input((uint8_t *)event->topic, (uint8_t *)event->data, (uint32_t)event->data_len);

        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

static void mqtt_app_start(void)
{
	const esp_mqtt_client_config_t mqtt_cfg = {

#if 0
		.broker.address.uri = "mqtts://a11homvea4zo8t-ats.iot.us-east-1.amazonaws.com:8883",
		.broker.verification.certificate = (const char *)server_cert_pem_start,
		.credentials = {
				.authentication = {
				        .certificate = (const char *)client_cert_pem_start,
				        .key = (const char *)client_key_pem_start,
				},
		}
#else
#if 0
		.broker.address.hostname  = "104.198.23.192",
		.broker.address.port      = 1883,
		.broker.address.transport = MQTT_TRANSPORT_OVER_TCP,
#else
		.broker.address.uri = "mqtt://devnet-staging.w3bstream.com:1883",
#endif
#endif
	};

    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);


    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);

/*
 * If 'NTP over DHCP' is enabled, we set dynamic pool address
 * as a 'secondary' server. It will act as a fallback server in case that address
 * provided via NTP over DHCP is not accessible
 */
#if LWIP_DHCP_GET_NTP_SRV && SNTP_MAX_SERVERS > 1
    sntp_setservername(1, "pool.ntp.org");

#if LWIP_IPV6 && SNTP_MAX_SERVERS > 2          // statically assigned IPv6 address is also possible
    ip_addr_t ip6;
    if (ipaddr_aton("2a01:3f7::1", &ip6)) {    // ipv6 ntp source "ntp.netnod.se"
        sntp_setserver(2, &ip6);
    }
#endif  /* LWIP_IPV6 */

#else   /* LWIP_DHCP_GET_NTP_SRV && (SNTP_MAX_SERVERS > 1) */
    // otherwise, use DNS address from a pool
    sntp_setservername(0, CONFIG_SNTP_TIME_SERVER);

    sntp_setservername(1, "time.windows.com");     // set the secondary NTP server (will be used only if SNTP_MAX_SERVERS > 1)
#endif

    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
#endif
    sntp_init();

    ESP_LOGI(TAG, "List of configured NTP servers:");

    for (uint8_t i = 0; i < SNTP_MAX_SERVERS; ++i){
        if (sntp_getservername(i)){
            ESP_LOGI(TAG, "server %d: %s", i, sntp_getservername(i));
        } else {
            // we have either IPv4 or IPv6 address, let's print it
            char buff[INET6_ADDRSTRLEN];
            ip_addr_t const *ip = sntp_getserver(i);
            if (ipaddr_ntoa_r(ip, buff, INET6_ADDRSTRLEN) != NULL)
                ESP_LOGI(TAG, "server %d: %s", i, buff);
        }
    }
}

static void obtain_time(void)
{
    /**
     * NTP server address could be aquired via DHCP,
     * see following menuconfig options:
     * 'LWIP_DHCP_GET_NTP_SRV' - enable STNP over DHCP
     * 'LWIP_SNTP_DEBUG' - enable debugging messages
     *
     * NOTE: This call should be made BEFORE esp aquires IP address from DHCP,
     * otherwise NTP option would be rejected by default.
     */
#if LWIP_DHCP_GET_NTP_SRV
    sntp_servermode_dhcp(1);      // accept NTP offers from DHCP server, if any
#endif

    initialize_sntp();

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 15;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    time(&now);
    localtime_r(&now, &timeinfo);
}

time_t iotex_time_set_func(void)
{
    return time(NULL);
}

int iotex_mqtt_pubscription(unsigned char *topic, unsigned char *buf, unsigned int buflen, int qos)
{
	return esp_mqtt_client_publish(mqtt_client, (const char *)topic, (const char *)buf, buflen, 1, 0);
}

int iotex_mqtt_subscription(unsigned char *topic)
{
    return esp_mqtt_client_subscribe(mqtt_client, (const char *)topic, 1);
}

#if IOTEX_USE_SIGN_FUNC_EXT
int iotex_sign_message_func(const uint8_t * input, size_t input_length, uint8_t * signature, size_t * signature_length )
{
    return psa_sign_message( key_id, PSA_ALG_ECDSA(PSA_ALG_SHA_256), input, input_length, signature, 64, signature_length);
}
#endif

void iotex_generate_signkey(unsigned char *exported_key, unsigned int *key_len)
{
    psa_status_t status;
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

    uint8_t exported[PSA_KEY_EXPORT_ECC_PUBLIC_KEY_MAX_SIZE(KEY_BITS)];
	size_t exported_length = 0;

    printf("Generate a key pair...\n");

	psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_VERIFY_HASH | PSA_KEY_USAGE_EXPORT);
	psa_set_key_algorithm(&attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
	psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(IOTEX_SIGNKEY_ECC_MODE));
	psa_set_key_bits(&attributes, KEY_BITS);

	status = psa_generate_key(&attributes, &key_id);
	if (status != PSA_SUCCESS) {
		printf("Failed to generate key %d\n", status);
        return;
	}

#ifdef IOTEX_DEBUG_ENABLE
	printf("Success to generate a key pair: key id : %x\n", key_id);
#endif

    status = psa_export_key(key_id, exported_key, 32, &exported_length);
	if (status != PSA_SUCCESS) {
		printf("Failed to export pair key %d\n", status);
        return;
	}

#ifdef IOTEX_DEBUG_ENABLE
	printf("Exported a pair key len %d\n", exported_length);
#endif

    *key_len = exported_length;

#ifdef IOTEX_DEBUG_ENABLE
    for (int i = 0; i < exported_length; i++)
    {
        printf("%02x ", exported_key[i]);
    }
    printf("\n");
#endif

    status = psa_export_public_key(key_id, exported, sizeof(exported), &exported_length);
	if (status != PSA_SUCCESS) {
		printf("Failed to export public key %d\n", status);
        return;
	}
#ifdef IOTEX_DEBUG_ENABLE
	printf("Exported a public key len %d\n", exported_length);

	for (int i = 0; i < exported_length; i++)
	{
        printf("%02x ", exported[i]);
	}
	printf("\n");
#endif
}



void iotex_import_key_example(void)
{
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;
    psa_status_t status;
    unsigned char prikey[32] = {0};
#if (IOTEX_SIGNKEY_USE_MODE == IOTEX_SIGNKEY_USE_EEPROM)
    unsigned int  prikey_len = 0;
#endif

    char dev_address[100] = {0};

    uint8_t key_mode = 0;

#if (IOTEX_SIGNKEY_USE_MODE == IOTEX_SIGNKEY_USE_EEPROM)

    EEPROM.begin(IOTEX_SIGNKEY_IN_EEPROM_BUF_SIZE);

    if( iotex_check_signkey_valid_in_eeprom() ) {

#ifdef IOTEX_DEBUG_ENABLE
        Serial.printf("SignKey is valid in eeprom\n");
#endif
        iotex_read_signkey_from_eeprom(prikey, &prikey_len);

#ifdef IOTEX_DEBUG_ENABLE
        Serial.print("Read Key [");
        Serial.print(prikey_len);
        Serial.print("]: ");
        for (int i = 0; i < prikey_len; i++) {
            Serial.printf("%02x ", prikey[i]);
        }
        Serial.println();
#endif

        key_mode = 1;

    } else {

#ifdef IOTEX_DEBUG_ENABLE
        Serial.printf("SignKey is invalid in eeprom\n");
#endif
        iotex_generate_signkey(prikey, &prikey_len);

#ifdef IOTEX_DEBUG_ENABLE
        Serial.print("Generate Key [");
        Serial.print(prikey_len);
        Serial.print("]: ");
        for (int i = 0; i < prikey_len; i++) {
            Serial.printf("%02x ", prikey[i]);
        }
        Serial.println();
#endif
        iotex_write_signkey_to_eeprom(prikey, prikey_len);

        key_mode = 0;
   }

#endif

#if (IOTEX_SIGNKEY_USE_MODE == IOTEX_SIGNKEY_USE_STATIC_DATA)

    memcpy(prikey, private_key, sizeof(prikey));
    key_mode = 1;

#endif

#if (IOTEX_SIGNKEY_USE_MODE == IOTEX_SIGNKEY_USE_PRNG)

    default_SetSeed(IOTEX_SEED_USER_DEFINE);
    iotex_generate_signkey(prikey, &prikey_len);

    key_mode = 0;
#endif

#if 0
    iotex_generate_signkey(prikey, &prikey_len);

    printf("Generate Key [%d]:\n", prikey_len);
    for (int i = 0; i < prikey_len; i++) {
        printf("%02x ", prikey[i]);
    }
    printf("\n");

    key_mode = 0;
#endif

    if( key_mode ) {

        /* Set key attributes */
        psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_VERIFY_HASH);
        psa_set_key_algorithm(&attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
        psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(IOTEX_SIGNKEY_ECC_MODE));
        psa_set_key_bits(&attributes, 256);

        /* Import the key */
        status = psa_import_key(&attributes, prikey, 32, &key_id);
        if (status != PSA_SUCCESS) {
    #ifdef IOTEX_DEBUG_ENABLE
            printf("Failed to import pri key err %d\n", status);
    #endif
            key_id = 0;

            return;
        }
    #ifdef IOTEX_DEBUG_ENABLE
        else
            printf("Success to import pri key keyid %x\n", key_id);
    #endif

    }

    status = psa_export_public_key(key_id, exported, sizeof(exported), (size_t *)&exported_len);
	if (status != PSA_SUCCESS) {
#ifdef IOTEX_DEBUG_ENABLE
		printf("Failed to export public key %d\n", status);
#endif
        return;
	}

#ifdef IOTEX_DEBUG_ENABLE
	printf("Exported a public key len %d\n", exported_len);
	for (int i = 0; i < exported_len; i++)
	{
        printf("%02x ", exported[i]);
	}
	printf("\n");
#endif

	iotex_dev_access_generate_dev_addr((const unsigned char *)exported, dev_address);
    printf("Dev_addr : %s\n", dev_address);

#ifdef IOTEX_DEBUG_ENABLE_EXT
    unsigned char inbuf[] = "hello devnet";
	unsigned char buf[65] = {0};
    unsigned int  sinlen   = 0;

    status = psa_sign_message( key_id, PSA_ALG_ECDSA(PSA_ALG_SHA_256), inbuf, strlen((const char *)inbuf), (unsigned char *)buf, 65, &sinlen);
    if (status != PSA_SUCCESS) {
		printf("Failed to sign message %d\n", status);
	} else {
        printf("Success to sign message %d\n", sinlen);
    }

#ifdef IOTEX_DEBUG_ENABLE
	printf("Exported a sign len %d\n", sinlen);
	for (int i = 0; i < sinlen; i++)
	{
        printf("%02x ", buf[i]);
	}
	printf("\n");
#endif

    status = psa_verify_message( key_id, PSA_ALG_ECDSA(PSA_ALG_SHA_256), inbuf, strlen((const char *)inbuf), (unsigned char *)buf, sinlen);
    if (status != PSA_SUCCESS) {
		printf("Failed to verify message %d\n", status);
	} else  {
        printf("Success to verify message\n");
    }
#endif

}
extern void default_SetSeed(unsigned int seed);
void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();

    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    // Is time set? If not, tm_year will be (1970 - 1900).
    if (timeinfo.tm_year < (2016 - 1900)) {
        ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        obtain_time();
        // update 'now' variable with current time
        time(&now);
    }

    char strftime_buf[64];

#if 0
    // Set timezone to Eastern Standard Time and print local time
    setenv("TZ", "EST5EDT,M3.2.0/2,M11.1.0", 1);
    tzset();
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in New York is: %s", strftime_buf);
#else
    // Set timezone to China Standard Time
    setenv("TZ", "CST-8", 1);
    tzset();
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in Shanghai is: %s", strftime_buf);
#endif

    iotex_wsiotsdk_init(iotex_time_set_func, iotex_mqtt_pubscription, iotex_mqtt_subscription);

    default_SetSeed(esp_random());
    iotex_import_key_example();

    ESP_LOGI(TAG, "ESP_MQTT_START");
    mqtt_app_start();

    while(1) {

    	vTaskDelay(5000 / portTICK_PERIOD_MS);

    	iotex_dev_access_data_upload_example_with_protobuf("hello devnet", strlen("hello devnet"));

    }
}














