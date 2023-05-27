

# How to create a w3bstream studio project



This article will explain how to create projects, add devices, get token, mqtt topic and other information through w3bstream studio.

For more information about w3bstream, please refer to the following document:

[About W3bstream - W3bstream Docs](https://docs.w3bstream.com/introduction/readme)



## Login to w3bstream studio

[W3bstream Devnet](https://devnet-staging.w3bstream.com/)

##### After login：

![devnet_home](img\devnet_home.png)



## Create a new project

##### 1. Please click the Create a project now button to start creating a new project.

![devnet_new_prj](img\devnet_new_prj.png)

##### 2. Enter a project name in the Name text box.

**Note: The project name should be less than 16 characters.**

You can choose either **Hello World** or **Code Upload**. (Here we choose Hello World as a demo)

![devnet_new_prj_2](img\devnet_new_prj_2.png)

When the settings are complete, click the Submit button. We now have a project named "esp32_hello".

![devnet_home_prj](img\devnet_home_prj.png)



## Add a device and get the Token

![devnet_home_prj](img\devnet_home_prj.png)

##### 1. Click on the project name (esp32_hello) to enter this project.

![devnet_prj_adddev](img\devnet_prj_adddev.png)

##### 2. Please click Devices and Add Device in order.

![devnet_prj_adddev_2](img\devnet_prj_adddev_2.png)

##### 3. Enter the Publisher Key.

![devnet_prj_adddev_3](img\devnet_prj_adddev_3.png)

**Device has been added and Token is displayed. **



## Get the Topic of MQTT

![devnet_prj_mqtt_topic](img\devnet_prj_mqtt_topic.png)

**You can see the MQTT Publish Topic after clicking on Triggers on the left**



## View WASM log output

![devnet_log](img\devnet_log.png)
