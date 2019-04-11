# ojousima.esp32_blegw.c
Gateway to listen to Bluetooth broadcasts and relay them via MQTT.
This project is a proposal to implement [Ruuvi Dongle](https://f.ruuvi.com/t/new-project-ruuvi-dongle/3201). 

# Usage
1. Download the repository and components
```
git clone $REPOSITORY_ADDRESS
git submodule update --init --recursive
```

2. Connect the esp32 dev-kit to your PC.  

3. Go into the project folder, open terminal in the folder path and run the following command: 

```	
make menuconfig
```	

4. Select the ‘Project Configuration’ option and set the GPIO pin numbers for WiFi status LED and WiFi reset configuration GPIOs.

5. Exit and save the configuration.

6. Run the following command: 

```	
make erase_flash flash monitor
```

This will erase the flash and burn the firmware. You can see the output from the esp32 on this terminal.

7. When the code is burnt the LED on the dev-kit will start blinking and the esp32 will be showing the following output, which means it is in APSTA mode:

```
I (0) cpu_start: App cpu up.
I (620) heap_init: Initializing. RAM available for dynamic allocation:
I (627) heap_init: At 3FFAFF10 len 000000F0 (0 KiB): DRAM
I (633) heap_init: At 3FFD1730 len 0000E8D0 (58 KiB): DRAM
I (639) heap_init: At 3FFE0440 len 00003BC0 (14 KiB): D/IRAM
I (646) heap_init: At 3FFE4350 len 0001BCB0 (111 KiB): D/IRAM
I (652) heap_init: At 400960B8 len 00009F48 (39 KiB): IRAM
I (658) cpu_start: Pro cpu start user code
I (5) cpu_start: Starting scheduler on PRO CPU.
I (0) cpu_start: Starting scheduler on APP CPU.
I (7) main: App started
I (7) gpio: GPIO[2]| InputEn: 0| OutputEn: 1| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:0 
I (17) gpio: GPIO[0]| InputEn: 1| OutputEn: 0| OpenDrain: 0| Pullup: 0| Pulldown: 0| Intr:3 
I (47) BTDM_INIT: BT controller compile version [0e46ff6]

I (47) system_api: Base MAC address is not set, read default base MAC address from BLK0 of EFUSE
I (47) main: free heap: 117240
I (57) system_api: Base MAC address is not set, read default base MAC address from BLK0 of EFUSE
I (67) system_api: Base MAC address is not set, read default base MAC address from BLK0 of EFUSE
I (1137) phy: phy_version: 4000, b6198fa, Sep  3 2018, 15:11:06, 0, 0
I (1387) http_server: HTTP Server listening on 80/tcp
I (1387) dns_server: DNS Server listening on 53/udp
I (5077) main: free heap: 73276
I (10077) main: free heap: 73276
I (15077) main: free heap: 73276
```

8. Now it’s time to connect to esp32 access point. The password is `esp32pwd`.

9. When esp32 is connected to your PC go to the browser and enter `192.168.1.1`, wait for some time as esp32 is loading APs and then select the desired Access Point (AP) to which esp32 will connect and come into STA only mode. 

You will not be able to access the esp32 as an AP again unless you press the wifi reset button (selected GPIO in `make menuconfig`). The WiFi status LED will start blinking which means that the esp32 is not yet connected to WiFi. When the esp32 is connected to the desired AP the LED will get stable.

When board boots with WiFi configured, printout is like below. 
```
I (47) system_api: Base MAC address is not set, read default base MAC address from BLK0 of EFUSE
I (47) main: free heap: 117240
I (57) system_api: Base MAC address is not set, read default base MAC address from BLK0 of EFUSE
I (67) system_api: Base MAC address is not set, read default base MAC address from BLK0 of EFUSE
I (1137) phy: phy_version: 4000, b6198fa, Sep  3 2018, 15:11:06, 0, 0
I (1387) http_server: HTTP Server listening on 80/tcp
I (1387) dns_server: DNS Server listening on 53/udp
I (5047) event: sta ip: 192.168.10.128, mask: 255.255.255.0, gw: 192.168.10.1
I (5047) main: Connected to WiFi, connecting to MQTT
I (5047) MQTT: Connecting MQTT client
I (5047) system_api: Base MAC address is not set, read default base MAC address from BLK0 of EFUSE
I (5057) time: Synchronizing SNTP
I (5067) time: Waiting for system time to be set... (1/10)
I (5077) main: free heap: 59244
I (5307) MQTT_CLIENT: Sending MQTT CONNECT message, type: 1, id: 0000
I (5667) MQTT: MQTT client connected
I (5667) main: Connected to MQTT, scanning for BLE devices
I (5877) main: Device detected: d8:a0:1d:65:7c:9c/RuuviTag/e2:ae:91:cb:0c:b0/RAW:1552656616:03371556BF2CFFC10002040D0BB3
I (5907) main: Device detected: d8:a0:1d:65:7c:9c/Eddystone/f1:5c:df:51:b9:81/RSSI:1552656616:-86
I (6067) time: SNTP Synchronized
I (6177) main: Device detected: d8:a0:1d:65:7c:9c/RuuviTag/c3:a7:3b:33:d4:c9/RAW:1552656617:FAF74A9313EF81FC35BC386B5758684EBAC3A7
I (6417) main: Device detected: d8:a0:1d:65:7c:9c/Eddystone/f1:5c:df:51:b9:81/RSSI:1552656617:-80
I (6917) main: Device detected: d8:a0:1d:65:7c:9c/Eddystone/f1:5c:df:51:b9:81/RSSI:1552656617:-80
I (7327) main: Device detected: d8:a0:1d:65:7c:9c/RuuviTag/f5:4f:92:06:77:95/RAW:1552656618:033E1536BF82005703EFFF620BEF
I (7527) main: Device detected: d8:a0:1d:65:7c:9c/RuuviTag/fc:48:06:67:9b:7e/RAW:1552656618:03371563BF360078FF24FBF80BA1
I (7927) main: Device detected: d8:a0:1d:65:7c:9c/Eddystone/f1:5c:df:51:b9:81/RSSI:1552656618:-81
I (7937) main: Device detected: d8:a0:1d:65:7c:9c/Eddystone/bc:62:dc:4a:34:12/RSSI:1552656618:-89
```

Data will be sent to a fixed MQTT server playground.ruuvi.com. Please note that there is no access control and
the server is for development only and may be taken offline at any point. You can configure the server address
in main.c

10. When you press the GPIO wifi reset button the esp32 will go into APSTA mode and you can again repeat the point 7.

11. If you press the dev-kit reset button then esp32 will reload the previously saved AP settings from NVSRAM and connect to it. So LED will again get stable when esp32 is connected to WiFi AP.

# Project roadmap
## First boot

 * ~~Dongle must be connectible over LAN~~
 * ~~Dongle may act as a WiFi access point for connection~~
 * ~~Dongle must provide a HTTP page which allows configuration to be set~~
 * The page shall have Ruuvi’s MQTT broker and QoS 0 as default [0.5.0]
 * ~~The page must have field for WIFi configuration~~

## Operation
 * ~~The dongle must fetch current real time from NTP server~~
 * ~~The dongle must scan for BLE advertisements from Ruuvi Devices~~
 * ~~Advertisements may be identified via sequence FF9904~~
 * The dongle must store new data to RAM buffer if previous data point is older than minimum update interval
 * ~~The dongle must transmit data buffer to MQTT broker with topic “{GW_MAC_ADDRESS}/RuuviTag/{TAG_MAC_ADDRESS}/{DATA}~~
 * ~~The data shall be in format {TIMESTAMP}:{PAYLOAD}~~
 * ~~Status of dongle shall be indicated via LEDs~~
 * Dongle shall enter into client mode after MQTT connection has been established [0.4.0]
 * Dongle shall enter into access point mode after MQTT connection has been lost [0.4.0]

 Timestamp is a string representing Unix Epoch (number of milliseconds since Jan 1. 1970 00.00).
 Payload is value of field in string represenstation, such as -90 for RSSI or `03371556BF2CFFC10002040D0BB3`for Ruuvi data format 3.

## Factory reset
 * ~~The dongle must return to first boot state if user triggers factory reset via button.~~

## Firmware update
 * ~~The firmware must be updateable without special equipment.~~
 * Update may require wired USB connection, or it may be implemented over the internet. [0.6.0]
 * Update must be triggered only after user action requiring physical access to device, i.e. button press. [0.6.0]

## Licenses
 BSD-3 for the main project, submodules under folder `components` have their own licenses. 
 Main project is based on MIT-Licensed (Ruuvi-GW)[https://gitlab.com/rhr407/ruuvi].