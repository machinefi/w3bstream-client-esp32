#include "wsiotsdk.h"

int iotex_wsiotsdk_init(iotex_gettime get_time_func, iotex_mqtt_pub mqtt_pub, iotex_mqtt_sub mqtt_sub) {

    psa_crypto_init();
    iotex_dev_access_init();

    iotex_dev_access_set_time_func(get_time_func);
    iotex_dev_access_set_mqtt_func(mqtt_pub, mqtt_sub);
#if IOTEX_USE_SIGN_FUNC_EXT
    iotex_dev_access_set_sign_func(iotex_sign_message_func);
#endif

	return 0;

}


