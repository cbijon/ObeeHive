# Beehive Monitoring with CubeCell

This repository contains an Arduino sketch for monitoring beehives using a CubeCell device from Heltec. The code collects various environmental data and hive-specific metrics, then transmits the data to The Things Network (TTN) via LoRaWAN communication.

## Features

- Collects data from various sensors, including temperature, humidity, pressure, light intensity, battery level, and weight from a load cell.
- Supports both Over The Air Activation (OTAA) and Activation By Personalization (ABP) LoRaWAN activation methods.
- Configurable LoRaWAN parameters, including device EUI, application EUI, and application key (for OTAA) or network session key and application session key (for ABP).
- LoRaWAN settings such as channels, region, device class, and more can be customized.
- Provides debug information through serial output for monitoring and debugging.
- Implements a sleep mode to conserve power between LoRaWAN transmission cycles.

## Hardware Requirements

To use this code, you will need the following hardware:

- Heltec CubeCell device
- 2x BME280 environmental sensors (indoor and outdoor)
- OneWire temperature sensors
- HX711 load cell
- Light sensor
- Appropriate power source (e.g., battery or external power supply)

## Installation and Usage

1. Clone or download this repository to your local development environment.

2. Install the required libraries by following the library installation instructions in the Arduino IDE or PlatformIO.

3. Open the Arduino sketch (`ObeeHive.ino.cpp`) in your preferred Arduino development environment.

4. Configure the LoRaWAN parameters, sensor settings, and other variables to suit your setup and preferences.

5. Upload the sketch to your CubeCell device.

6. Monitor the serial output for debugging and data readings.

7. Ensure your TTN application is properly set up to receive and handle the data transmitted by your CubeCell device.

8. Deploy the CubeCell device in your beehive for continuous monitoring.

## Contributing

Contributions to this project are welcome. Feel free to submit issues, feature requests, or pull requests to enhance the functionality or documentation.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## Contact

For questions or inquiries, please contact the project author:

- Name: Charles BIJON @neouf
- Email: bijon.charles@gmail.com

---

By neouf
