#include <stdio.h>
#include "wsiotsdk.h"

psa_key_id_t g_signkey = 1;
static uint8_t exported[PSA_KEY_EXPORT_ECC_PUBLIC_KEY_MAX_SIZE(256) + 1];
static uint8_t exported_dev_addr[65];

static void iotex_export_public_key(void) {

    psa_status_t status;
    size_t exported_length = 0;
    psa_key_attributes_t attributes = PSA_KEY_ATTRIBUTES_INIT;

    exported[0] = 0x4;
    status = psa_export_public_key( g_signkey, (uint8_t *)exported + 1, sizeof(exported) - 1, &exported_length );
    if( status != PSA_SUCCESS ) {

        printf("Generate a pair key...\n");

        psa_set_key_usage_flags(&attributes, PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_VERIFY_HASH | PSA_KEY_USAGE_EXPORT);
	    psa_set_key_algorithm(&attributes, PSA_ALG_ECDSA(PSA_ALG_SHA_256));
	    psa_set_key_type(&attributes, PSA_KEY_TYPE_ECC_KEY_PAIR(PSA_ECC_FAMILY_SECP_K1));
        psa_set_key_lifetime(&attributes, PSA_KEY_LIFETIME_PERSISTENT);
	    psa_set_key_bits(&attributes, 256); 
        psa_set_key_id(&attributes, 1);
        
        status = psa_generate_key(&attributes, &g_signkey);
        if( status == PSA_SUCCESS ) {
            printf("Success to generate a pairkey keyid %d\n", g_signkey);
        } else {
            printf("Failed to generate a pairkey %d\n", status);
        }

        psa_export_public_key( g_signkey, (uint8_t *)exported + 1, sizeof(exported) - 1, &exported_length );
    }

    printf("export key :\n");
    for (int i = 0; i < exported_length + 1; i++)
        printf("%.2x ", exported[i]);
    printf("\n");

    iotex_dev_access_generate_dev_addr(exported, exported_dev_addr);
    printf("Wallet Addr : %s\n", exported_dev_addr);
}

uint8_t * iotex_wsiotsdk_init(iotex_gettime get_time_func, iotex_mqtt_pub mqtt_pub, iotex_mqtt_sub mqtt_sub) {


    psa_crypto_init();
    iotex_dev_access_init();

    iotex_dev_access_set_time_func(get_time_func);
    iotex_dev_access_set_mqtt_func(mqtt_pub, mqtt_sub);

    iotex_export_public_key();

    InitLowsCalc();

#ifdef IOTEX_SIGN_VERIFY_TEST
    psa_status_t status;
    unsigned char inbuf[] = "iotex_ecdsa_test_only";
	unsigned char buf[65] = {0};
    unsigned int  sinlen   = 0;

    status = psa_sign_message( g_signkey, PSA_ALG_ECDSA(PSA_ALG_SHA_256), inbuf, strlen((const char *)inbuf) + 1, (unsigned char *)buf, 65, &sinlen);
    if (status != PSA_SUCCESS) {
		printf("Failed to sign message %d\n", status);
	} else {
        printf("Success to sign message %d\n", sinlen);
    }

    status = psa_verify_message( g_signkey, PSA_ALG_ECDSA(PSA_ALG_SHA_256), inbuf, strlen((const char *)inbuf) + 1, (unsigned char *)buf, sinlen);
    if (status != PSA_SUCCESS) {
		printf("Failed to verify message %d\n", status);
	} else  {
        printf("Success to verify message\n");
    }
#endif

	return (uint8_t *)exported;
}

uint8_t * iotex_wsiotsdk_get_public_key(void) {

    return (uint8_t *)exported;
}


