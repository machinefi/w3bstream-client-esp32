| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- |

# W3bstream Devnet Example

This example shows how to use the `Web3 component for ESP` of the WSIoTSDK for connecting to W3bstream Devnet.



## How to use example

### Configure the project

Open the project configuration menu (`idf.py menuconfig`).

In the `Example Configuration` menu:

* Set the Wi-Fi configuration.
    * Set `WiFi SSID`.
    
    * Set `WiFi Password`.
    
      

In the `iotex_dev_access_config.h` file (`..\include\application\devnet\`) :

- Set the Devnet configuration.

  - Set `IOTEX_TOKEN_DEFAULT`.

  - Set `IOTEX_MQTT_TOPIC_DEFAULT`.	  



Optional: If you need, change the other options according to your requirements.



### Set the registration functions

- Function Prototypes:

```c
typedef time_t (*iotex_gettime)(void);
typedef int (*iotex_mqtt_pub)(unsigned char *topic, unsigned char *buf, unsigned int buflen, int qos);
typedef int (*iotex_mqtt_sub)(unsigned char *topic);
```

​		**iotex_gettime**

​		This function will return the number of seconds that have elapsed since January 1, 1970 A.D. UTC time was calculated from 0:00:0 seconds to the current moment (i.e., January 1, 1970 00:00:00 GMT to the current moment in seconds).

------

​		*example:*

```c
time_t iotex_time_set_func(void) {
    return time(NULL);
}
```

------

​		**iotex_mqtt_pub**

​		Client to send a publish message to the broker.

| Parameters | Note                                                         |
| ---------- | ------------------------------------------------------------ |
| topic      | topic string                                                 |
| buf        | payload string (set to NULL, sending empty payload message)  |
| buflen     | data length, if set to 0, length is calculated from payload string |
| qos        | QoS of publish message                                       |

------

​		*example：*

```c
int iotex_mqtt_pubscription(unsigned char *topic, unsigned char *buf, unsigned int buflen, int qos) {
	return esp_mqtt_client_publish(mqtt_client, (const char *)topic, (const char *)buf, buflen, 1, 0);
}
```

------

​		

​		**iotex_mqtt_sub**

​		Subscribe the client to defined topic.

| Parameters | Note         |
| ---------- | ------------ |
| topic      | topic string |

------

​		*example:*

```c
int iotex_mqtt_subscription(unsigned char *topic) {
    return esp_mqtt_client_subscribe(mqtt_client, (const char *)topic, 0);
}
```

------



- Register function prototype：

```c
int iotex_wsiotsdk_init(iotex_gettime get_time_func, iotex_mqtt_pub mqtt_pub, iotex_mqtt_sub mqtt_sub);
```

------

​		*example:*

```c
void app_main(void) {

	// ......

	iotex_wsiotsdk_init(iotex_time_set_func, iotex_mqtt_pubscription, iotex_mqtt_subscription);

	// ......

}
```

------



### **User Data Upload**

- Function Prototype:

```c
int iotex_dev_access_data_upload_with_userdata(void *buf, size_t buflen, enum UserData_Type type);
```

​		User data upload to devnet.

| Parameters | Notes                |
| ---------- | -------------------- |
| buf        | Point to user data.  |
| buflen     | Length of user data. |
| type       | User data type.      |

```c
enum UserData_Type {
    IOTEX_USER_DATA_TYPE_JSON,
	IOTEX_USER_DATA_TYPE_PB,
	IOTEX_USER_DATA_TYPE_RAW
};
```



------

​		*example:*

​		Three examples are provided to show how to  report user data in `Json`, `ProtoBuf` and `Rawdata` formats .

```c
void iotex_devnet_upload_data_example_raw(void);

void iotex_devnet_upload_data_example_json(void);

void iotex_devnet_upload_data_example_pb(void);
```

------



### Build and Flash

Build the project and flash it to the board, then run the monitor tool to view the serial output:

Run `idf.py -p PORT flash monitor` to build, flash and monitor the project.

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for all the steps to configure and use the ESP-IDF to build projects.

* [ESP-IDF Getting Started Guide on ESP32](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html)
* [ESP-IDF Getting Started Guide on ESP32-S2](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s2/get-started/index.html)
* [ESP-IDF Getting Started Guide on ESP32-C3](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c3/get-started/index.html)

## Example Output
Note that the output, in particular the order of the output, may vary depending on the environment.

Console output if station connects to AP successfully:
```
I (589) wifi station: ESP_WIFI_MODE_STA
I (599) wifi: wifi driver task: 3ffc08b4, prio:23, stack:3584, core=0
I (599) system_api: Base MAC address is not set, read default base MAC address from BLK0 of EFUSE
I (599) system_api: Base MAC address is not set, read default base MAC address from BLK0 of EFUSE
I (629) wifi: wifi firmware version: 2d94f02
I (629) wifi: config NVS flash: enabled
I (629) wifi: config nano formating: disabled
I (629) wifi: Init dynamic tx buffer num: 32
I (629) wifi: Init data frame dynamic rx buffer num: 32
I (639) wifi: Init management frame dynamic rx buffer num: 32
I (639) wifi: Init management short buffer num: 32
I (649) wifi: Init static rx buffer size: 1600
I (649) wifi: Init static rx buffer num: 10
I (659) wifi: Init dynamic rx buffer num: 32
I (759) phy: phy_version: 4180, cb3948e, Sep 12 2019, 16:39:13, 0, 0
I (769) wifi: mode : sta (30:ae:a4:d9:bc:c4)
I (769) wifi station: wifi_init_sta finished.
I (889) wifi: new:<6,0>, old:<1,0>, ap:<255,255>, sta:<6,0>, prof:1
I (889) wifi: state: init -> auth (b0)
I (899) wifi: state: auth -> assoc (0)
I (909) wifi: state: assoc -> run (10)
I (939) wifi: connected with #!/bin/test, aid = 1, channel 6, BW20, bssid = ac:9e:17:7e:31:40
I (939) wifi: security type: 3, phy: bgn, rssi: -68
I (949) wifi: pm start, type: 1

I (1029) wifi: AP's beacon interval = 102400 us, DTIM period = 3
I (2089) esp_netif_handlers: sta ip: 192.168.77.89, mask: 255.255.255.0, gw: 192.168.77.1
I (2089) wifi station: got ip:192.168.77.89
I (2089) wifi station: connected to ap SSID:myssid password:mypassword
```

Console output if the station failed to connect to AP:
```
I (589) wifi station: ESP_WIFI_MODE_STA
I (599) wifi: wifi driver task: 3ffc08b4, prio:23, stack:3584, core=0
I (599) system_api: Base MAC address is not set, read default base MAC address from BLK0 of EFUSE
I (599) system_api: Base MAC address is not set, read default base MAC address from BLK0 of EFUSE
I (629) wifi: wifi firmware version: 2d94f02
I (629) wifi: config NVS flash: enabled
I (629) wifi: config nano formating: disabled
I (629) wifi: Init dynamic tx buffer num: 32
I (629) wifi: Init data frame dynamic rx buffer num: 32
I (639) wifi: Init management frame dynamic rx buffer num: 32
I (639) wifi: Init management short buffer num: 32
I (649) wifi: Init static rx buffer size: 1600
I (649) wifi: Init static rx buffer num: 10
I (659) wifi: Init dynamic rx buffer num: 32
I (759) phy: phy_version: 4180, cb3948e, Sep 12 2019, 16:39:13, 0, 0
I (759) wifi: mode : sta (30:ae:a4:d9:bc:c4)
I (769) wifi station: wifi_init_sta finished.
I (889) wifi: new:<6,0>, old:<1,0>, ap:<255,255>, sta:<6,0>, prof:1
I (889) wifi: state: init -> auth (b0)
I (1889) wifi: state: auth -> init (200)
I (1889) wifi: new:<6,0>, old:<6,0>, ap:<255,255>, sta:<6,0>, prof:1
I (1889) wifi station: retry to connect to the AP
I (1899) wifi station: connect to the AP fail
I (3949) wifi station: retry to connect to the AP
I (3949) wifi station: connect to the AP fail
I (4069) wifi: new:<6,0>, old:<6,0>, ap:<255,255>, sta:<6,0>, prof:1
I (4069) wifi: state: init -> auth (b0)
I (5069) wifi: state: auth -> init (200)
I (5069) wifi: new:<6,0>, old:<6,0>, ap:<255,255>, sta:<6,0>, prof:1
I (5069) wifi station: retry to connect to the AP
I (5069) wifi station: connect to the AP fail
I (7129) wifi station: retry to connect to the AP
I (7129) wifi station: connect to the AP fail
I (7249) wifi: new:<6,0>, old:<6,0>, ap:<255,255>, sta:<6,0>, prof:1
I (7249) wifi: state: init -> auth (b0)
I (8249) wifi: state: auth -> init (200)
I (8249) wifi: new:<6,0>, old:<6,0>, ap:<255,255>, sta:<6,0>, prof:1
I (8249) wifi station: retry to connect to the AP
I (8249) wifi station: connect to the AP fail
I (10299) wifi station: connect to the AP fail
I (10299) wifi station: Failed to connect to SSID:myssid, password:mypassword
```

## Notes



## Troubleshooting

For any technical queries, please open an [issue](https://github.com/espressif/esp-idf/issues) on GitHub. We will get back to you soon.
