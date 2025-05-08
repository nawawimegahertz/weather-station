# Smart Weather Station with ESP32

This project implements a smart weather station using an ESP32 microcontroller. It measures various environmental parameters such as temperature, humidity, wind speed, and soil moisture, displays the data on an LCD screen, and uploads the readings to the ThingSpeak IoT platform.

## Features

* **Temperature and Humidity Measurement:** Utilizes the DHT22 sensor to accurately measure ambient temperature and humidity.
* **Wind Speed Measurement:** Employs an anemometer to detect wind speed by counting rotations.
* **Soil Moisture Measurement:** Measures the moisture level in the soil using a capacitive soil moisture sensor.
* **Real-time Display:** Shows the current temperature, humidity, wind speed, and soil moisture on a 16x2 LCD display with I2C interface. The display alternates between environmental data and wind/soil data every 2 seconds.
* **ThingSpeak Integration:** Periodically uploads the sensor readings to a designated ThingSpeak channel for remote monitoring and data logging.
* **Multitasking:** Leverages the ESP32's dual-core capabilities using FreeRTOS tasks to handle sensor readings and display updates concurrently, ensuring smooth operation.
* **WiFi Connectivity:** Connects to a local WiFi network to enable data transmission to ThingSpeak.

## Hardware Components

* ESP32 Development Board
* DHT22 Temperature and Humidity Sensor
* Anemometer (Wind Speed Sensor)
* Capacitive Soil Moisture Sensor
* 16x2 LCD Display with I2C Module (Address 0x27)
* Jumper Wires

## Pin Configuration

| Component             | ESP32 Pin |
| :-------------------- | :-------- |
| Wind Sensor Output    | 15        |
| DHT22 Data Pin        | 4         |
| Soil Moisture Sensor  | 32        |
| LCD SDA               | SDA (typically 21) |
| LCD SCL               | SCL (typically 22) |

**Note:** The I2C pins (SDA and SCL) might vary depending on your ESP32 board. Please refer to your board's documentation.

## Software Libraries

The following Arduino libraries are required for this project:

* **DHT sensor library by Adafruit:** For interfacing with the DHT22 sensor. You can install it via the Arduino Library Manager by searching for "DHT sensor library by Adafruit".
* **Adafruit Unified Sensor:** This is a dependency for the DHT sensor library. It will likely be installed automatically when you install the DHT library.
* **LiquidCrystal I2C:** For controlling the I2C LCD display. You can install it via the Arduino Library Manager by searching for "LiquidCrystal I2C" by Frank de Brabander.
* **ThingSpeak:** For communicating with the ThingSpeak IoT platform. You can install it via the Arduino Library Manager by searching for "ThingSpeak".
* **WiFi:** This library is built-in to the ESP32 Arduino core and does not need separate installation.

## Setup Instructions

1.  **Install Libraries:** Open your Arduino IDE and go to `Sketch` > `Include Library` > `Manage Libraries...`. Search for and install the libraries mentioned above.
2.  **Connect Hardware:** Connect the sensors and the LCD to your ESP32 according to the pin configuration described in the "Hardware Components" section. Ensure all connections are secure.
3.  **Configure WiFi Credentials:** Open the Arduino sketch and modify the following lines with your WiFi network's SSID and password:

    ```arduino
    const char* ssid = "Your WiFi SSID";
    const char* password = "Your WiFi Password";
    ```

4.  **Configure ThingSpeak Channel:**
    * Create a new channel on [ThingSpeak](https://thingspeak.com/).
    * Note down your Channel ID and Write API Key.
    * Update the following lines in the Arduino sketch with your ThingSpeak Channel ID and Write API Key:

    ```arduino
    unsigned long myChannelNumber = Your_Channel_ID;
    const char* myWriteAPIKey = "Your_Write_API_Key";
    ```

5.  **Upload Code:** Select the appropriate ESP32 board and port in the Arduino IDE and upload the code to your ESP32.
6.  **Monitor Output:** Open the Serial Monitor in the Arduino IDE (Tools > Serial Monitor) to observe the connection status, sensor readings, and ThingSpeak upload status.
7.  **View Data on ThingSpeak:** Once the ESP32 connects to WiFi and starts sending data, you should be able to see the sensor readings updated on your ThingSpeak channel.

## Code Explanation

* **Includes:** Includes necessary libraries for DHT sensor, LCD, Arduino functionalities, WiFi, and ThingSpeak.
* **Pin Configuration:** Defines the GPIO pins used for connecting the sensors and the LCD.
* **WiFi Credentials:** Stores the SSID and password for your WiFi network.
* **ThingSpeak Configuration:** Holds your ThingSpeak Channel ID and Write API Key.
* **Sensor & LCD Objects:** Creates objects for the DHT sensor and the I2C LCD.
* **Shared Variables:** Declares variables to store sensor readings and status flags. `volatile` and `portMUX_TYPE` are used for safe access from different FreeRTOS tasks.
* **Task Handles:** Declares handles for the FreeRTOS tasks.
* **Timing Variables:** Defines the interval for uploading data to ThingSpeak.
* **Interrupt Handler (`countRotation`):** This function is called every time the wind sensor's output falls (indicating a rotation). It increments the `rotationCount`. It's marked `IRAM_ATTR` to ensure it's placed in IRAM for faster execution and `portENTER_CRITICAL_ISR` and `portEXIT_CRITICAL_ISR` for thread-safe access to the shared variable.
* **Sensor Task (`sensorReadingTask`):** This task runs on Core 0 and is responsible for:
    * Calculating wind speed based on the number of rotations within a second.
    * Reading temperature and humidity from the DHT22 sensor every 2 seconds.
    * Reading soil moisture level and mapping the analog value to a percentage.
* **Display Task (`displayUpdateTask`):** This task runs on Core 1 and handles updating the LCD display. It alternates between showing temperature/humidity and wind speed/soil moisture every 2 seconds. It uses `portENTER_CRITICAL` and `portEXIT_CRITICAL` for thread-safe access to the shared sensor data.
* **ThingSpeak Upload (`uploadToThingSpeak`):** This function reads the latest sensor values (with thread-safe access) and uploads them to the specified ThingSpeak channel.
* **WiFi Connection (`connectToWiFi`):** This function handles connecting the ESP32 to the configured WiFi network. It also reconnects if the connection is lost.
* **Setup (`setup`):** Initializes the serial communication, connects to WiFi, initializes ThingSpeak, configures the wind sensor pin as an interrupt, initializes the DHT sensor, and creates the `sensorReadingTask` and `displayUpdateTask` on different cores.
* **Loop (`loop`):** Checks for WiFi connection and periodically calls the `uploadToThingSpeak` function. A small delay is added to prevent watchdog timer issues.

## Further Improvements

* **Rain Gauge Integration:** Add a rain gauge sensor to measure rainfall.
* **Solar Radiation Sensor:** Incorporate a solar radiation sensor to measure sunlight intensity.
* **Data Logging to SD Card:** Implement data logging to an SD card for offline data storage.
* **More Sophisticated Display:** Use a graphical LCD or OLED display to show more information or create charts.
* **Web Interface:** Develop a web interface to view real-time and historical data.
* **Power Management:** Implement power-saving techniques for battery-powered operation.
* **Error Handling:** Add more robust error handling for sensor readings and network connectivity.# weather-station
