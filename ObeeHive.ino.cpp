/**

Using CubeCell from heltec with some probe to have a continue monitoring about your hive.
Data are send to a TTN

**/


#include <LoRaWan_APP.h>
#include <Arduino.h>
#include <HX711_ADC.h>
#include "cactus_io_BME280_I2C.h"
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
// Pression au niveau de la mer (1013.25) ( paris  + 6.75 )
#define SEALEVELPRESSURE_HPA (1006.5)
// Data wire is plugged into GPIO0 on the CubeCell
#define ONE_WIRE_BUS GPIO0
#define Vext GPIO6
#define LIGHTSENSORPIN ADC

/* OTAA para TODO : SETUP THERE YOUR TTN parameters*/
uint8_t devEui[] = { ...... };
uint8_t appEui[] = { ...... };

uint8_t appKey[] = { ................... };


/* ABP para*/
uint8_t nwkSKey[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t appSKey[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint32_t devAddr = (uint32_t)0x00000000;

bool ENABLE_SERIAL = true;        // enable serial debug output here if required
uint32_t appTxDutyCycle = 40000;  // the frequency of readings, in milliseconds(set 300s)

uint16_t userChannelsMask[6] = { 0x00FF, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000 };
LoRaMacRegion_t loraWanRegion = ACTIVE_REGION;
DeviceClass_t loraWanClass = LORAWAN_CLASS;
bool overTheAirActivation = LORAWAN_NETMODE;
bool loraWanAdr = LORAWAN_ADR;
bool keepNet = LORAWAN_NET_RESERVE;
bool isTxConfirmed = LORAWAN_UPLINKMODE;
uint8_t appPort = 2;
uint8_t confirmedNbTrials = 1;



//meteo + battery
int temperature, humidity, batteryVoltage, temperatureOut, humidityOut, batteryLevel;
long pressure, pressureOut;

int boxAltitude = 50;
 
// Create two BME280 instances
BME280_I2C bme280_out(0x77);  // I2C using address 0x77
BME280_I2C bme280_in(0x76);   // I2C using address 0x76


// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

//scale:
const int HX711_dout = GPIO2;  //mcu > HX711 dout pin
const int HX711_sck = GPIO1;   //mcu > HX711 sck pin

//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);

/**
 Setup scale for callibration
**/
float scalecell;
const int calVal_eepromAdress = 0;
unsigned long t = 0;
float calibrationValue = 22.79; /** You can find this value with the project **/ 
long TareOffset = 8140740;


int max_sample = 10;



void setup() {
  boardInitMcu();
  // Vext alimentation switch ON
  //init serial
  Serial.begin(115200);
  uint64_t chipID = getID();
  Serial.println("Booting device ObeeTwo probe");
  Serial.printf("ChipID:%04X%08X\r\n", (uint32_t)(chipID >> 32), (uint32_t)chipID); // 13A659832B21
  Serial.begin(115200);
  delay(10);
  Serial.println("Starting...");
  delay(1500);
  LoadCell.begin(128);
  long stabilisingtime = 5000;  // tare preciscion can be improved by adding a few seconds of stabilising time
  //LoadCell.start(stabilisingtime, 0);
  LoadCell.start(stabilisingtime, 1);
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Tare timeout, check MCU>HX711 wiring and pin designations");
  } else {
    long tareoffst = LoadCell.getTareOffset();
    Serial.print("tare is :");
    Serial.println(tareoffst);
    LoadCell.setCalFactor(calibrationValue);  // set calibration factor (float)
    long newtareoffset = tareoffst;
    LoadCell.setTareOffset(newtareoffset); // auto Tare
    //LoadCell.setTareOffset(TareOffset);
    Serial.println("Startup + tare is complete");
  }
  while (!LoadCell.update())
    ;
  Serial.print("Calibration factor: ");
  Serial.println(LoadCell.getCalFactor());
  Serial.print("HX711 measured conversion time ms: ");
  Serial.println(LoadCell.getConversionTime());
  Serial.print("HX711 measured sampling rate HZ: ");
  Serial.println(LoadCell.getSPS());
  Serial.print("HX711 measured settlingtime ms: ");
  Serial.println(LoadCell.getSettlingTime());
  Serial.println("Note that the settling time may increase significantly if you use delay() in your sketch!");
  if (LoadCell.getSPS() < 7) {
    Serial.println("!!Sampling rate is lower than specification, check MCU>HX711 wiring and pin designations");
  } else if (LoadCell.getSPS() > 100) {
    Serial.println("!!Sampling rate is higher than specification, check MCU>HX711 wiring and pin designations");
  }
  deviceState = DEVICE_STATE_INIT;
  LoRaWAN.ifskipjoin();
  pinMode(LIGHTSENSORPIN, INPUT);
}

void loop() {
  switch (deviceState) {
    case DEVICE_STATE_INIT:
      {
        printDevParam();
        LoRaWAN.init(loraWanClass, loraWanRegion);
        deviceState = DEVICE_STATE_JOIN;
        break;
      }
    case DEVICE_STATE_JOIN:
      {
        LoRaWAN.join();
        break;
      }
    case DEVICE_STATE_SEND:
      {
        prepareTxFrame(appPort);
        LoRaWAN.send();
        deviceState = DEVICE_STATE_CYCLE;
        break;
      }
    case DEVICE_STATE_CYCLE:
      {
        // Schedule next packet transmission
        txDutyCycleTime = appTxDutyCycle + randr(0, APP_TX_DUTYCYCLE_RND);
        LoRaWAN.cycle(txDutyCycleTime);
        deviceState = DEVICE_STATE_SLEEP;
        break;
      }
    case DEVICE_STATE_SLEEP:
      {
        LoRaWAN.sleep();
        break;
      }
    default:
      {
        deviceState = DEVICE_STATE_INIT;
        break;
      }
  }
}


static void prepareTxFrame(uint8_t port) {
  // This enables the output to power the sensor
  pinMode(Vext, OUTPUT);
  digitalWrite(Vext, LOW);
  //init meteo probe
  delay(500);
  sensors.begin();
  if (!bme280_in.begin()) {
    Serial.println("Device BME280 Indoor error!");
  } else {

    Serial.println("Device BME280 Indoor initialized");
  }
  if (!bme280_out.begin()) {
    Serial.println("Device BME280 Outdoor error!");
  } else {

    Serial.println("Device BME280 Outdoor initialized");
  }

  // This delay is required to allow the sensor time to init
  delay(1000);
  bme280_in.readSensor();
  temperature = bme280_in.getTemperature_C() * 100;
  humidity = bme280_in.getHumidity();
  pressure = bme280_in.getPressure_HP();
  delay(1000);
  bme280_out.readSensor();
  temperatureOut = bme280_out.getTemperature_C() * 100;
  humidityOut = bme280_out.getHumidity();
  pressureOut = bme280_out.getPressure_HP();
 

  Serial.print("Requesting temperatures...");
  //delay(1000);
  sensors.requestTemperatures();  // Send the command to get temperatures
  delay(1000);
  Serial.println("DONE");
  // After we got the temperatures, we can print them here.
  // We use the function ByIndex, and as an example get the temperature from the first sensor only.
  Serial.print("Temperature for the device 1 (index 0) is: ");
  Serial.println(sensors.getTempCByIndex(0));
  int tempMiel = sensors.getTempCByIndex(0) * 100;

  Wire.end();

  float lux = analogRead(LIGHTSENSORPIN);
  
  Serial.print("lumens :");
  Serial.println(lux);
  lux = 100 - ((lux / 4096) * 100);  //Get percent 
  Serial.println(analogReadmV(ADC));

  LoadCell.powerUp();
  delay(2500);
  //  reading
  long t = millis();
  long hx711_timeout = 15000;
  bool newdata = false;
  //prewarm samples ( this probe is shiting so much )
  while (millis() < t + 5000) {
    LoadCell.update();
  }
  t = millis();
  //so launch the last review to get data
  while ((millis() < t + hx711_timeout) and (newdata == false)) {
    if (LoadCell.update()) {
      Serial.println("scale data ok");
      newdata = true;
    }
  }
  delay(500);
  scalecell = LoadCell.getData();
  LoadCell.powerDown();
  
  digitalWrite(Vext, HIGH);
  delay(500);
    // preparing payloads
  
  batteryVoltage = getBatteryVoltage();
  uint8_t blvel = BoardGetBatteryLevel();
  float level = (float)((uint8_t)blvel);
  batteryLevel = 100.0 / 254.0 * level;
  Serial.print("level bat  :");
  Serial.println(blvel);

  appDataSize = 32;
  appData[0] = highByte(temperature);
  appData[1] = lowByte(temperature);

  appData[2] = highByte(humidity);
  appData[3] = lowByte(humidity);

  appData[4] = (byte)((pressure & 0xFF000000) >> 24);
  appData[5] = (byte)((pressure & 0x00FF0000) >> 16);
  appData[6] = (byte)((pressure & 0x0000FF00) >> 8);
  appData[7] = (byte)((pressure & 0X000000FF));

  appData[8] = highByte(batteryVoltage);
  appData[9] = lowByte(batteryVoltage);

  appData[10] = highByte(batteryLevel);
  appData[11] = lowByte(batteryLevel);

  appData[12] = highByte(boxAltitude);
  appData[13] = lowByte(boxAltitude);

  long l_cell = (long)((double)scalecell);
  appData[14] = (byte)((l_cell & 0xFF000000) >> 24);
  appData[15] = (byte)((l_cell & 0x00FF0000) >> 16);
  appData[16] = (byte)((l_cell & 0x0000FF00) >> 8);
  appData[17] = (byte)((l_cell & 0X000000FF));

  appData[18] = highByte(temperatureOut);
  appData[19] = lowByte(temperatureOut);

  appData[20] = highByte(humidityOut);
  appData[21] = lowByte(humidityOut);

  appData[22] = (byte)((pressureOut & 0xFF000000) >> 24);
  appData[23] = (byte)((pressureOut & 0x00FF0000) >> 16);
  appData[24] = (byte)((pressureOut & 0x0000FF00) >> 8);
  appData[25] = (byte)((pressureOut & 0X000000FF));

  appData[26] = highByte(tempMiel);
  appData[27] = lowByte(tempMiel);
  
  long l_lux = (long)((double)lux);
  //long l_lux = (long)((double)square_ratio);
  appData[28] = (byte)((l_lux & 0xFF000000) >> 24);
  appData[29] = (byte)((l_lux & 0x00FF0000) >> 16);
  appData[30] = (byte)((l_lux & 0x0000FF00) >> 8);
  appData[31] = (byte)((l_lux & 0X000000FF));

  if (ENABLE_SERIAL) {
    Serial.print("Temperature Miel : ");
    Serial.print(tempMiel / 100);
    Serial.print(" C, Temperature ruche: ");
    Serial.print(temperature / 100);
    Serial.print(" C, Humidity ruche: ");
    Serial.print(humidity);
    Serial.print(" %, Pressure ruche: ");
    Serial.print(pressure / 100);
    Serial.print(" HPA, Battery Voltage: ");
    Serial.print(batteryVoltage);
    Serial.print(" mV, Battery Level: ");
    Serial.print(batteryLevel);
    Serial.print(" %, Temperature Ext: ");
    Serial.print(temperatureOut / 100);
    Serial.print(" C, Humidity Ext: ");
    Serial.print(humidityOut);
    Serial.print(" %, Pressure Ext: ");
    Serial.print(pressureOut / 100);
    Serial.print(" HPA, Load_cell output val: ");
    Serial.print(scalecell);
    Serial.print(" grammes");
    Serial.print(" Luxmeter is: ");
    Serial.print(lux);
    Serial.println(" %");
  }
}
