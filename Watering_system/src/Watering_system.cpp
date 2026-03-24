/* 
 * Project Smart Houseplant Watering System
 * Author: Jay Elder
 * Date: 03/19/2026
 */


#include "Particle.h"
#include "credentials.h"
#include "Adafruit_BME280.h"
#include "Adafruit_SSD1306.h"
#include "IotClassroom_CNM.h"
#include "Grove_Air_quality_Sensor.h"
#include <Adafruit_MQTT.h>
#include "Adafruit_MQTT/Adafruit_MQTT_SPARK.h"
#include "Adafruit_MQTT/Adafruit_MQTT.h"

TCPClient TheClient; 
Adafruit_MQTT_SPARK mqtt(&TheClient,AIO_SERVER,AIO_SERVERPORT,AIO_USERNAME,AIO_KEY); 

Adafruit_MQTT_Subscribe pumpFeed = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/Pump"); 
Adafruit_MQTT_Publish sensorFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Temperature, Moisture, Air quality");

SYSTEM_MODE(AUTOMATIC);


SYSTEM_THREAD(ENABLED);

const int OLED_RESET=-1;
Adafruit_SSD1306 display(OLED_RESET);
Adafruit_BME280 bme;
AirQualitySensor sensor(A5);

bool status;
bool timer;
float temp, press, humid, Fah, inHg;
float temptoFah(float measurement);
float presstoinHg(float measurement);
unsigned int msec=500;
unsigned int last, lastTime;
float subValue,pubValue;
void MQTT_connect();
bool MQTT_ping();

void setup() {
  
  Serial.begin(9600);
  waitFor(Serial.isConnected, 10000);

  WiFi.on();
  WiFi.connect();
  while(WiFi.connecting()) {
    Serial.printf(".");
  } 
  Serial.printf("\n\n");
  
  mqtt.subscribe(&pumpFeed);

  while (!Serial);

  Serial.printf("Waiting sensor to init...");
   delay(20000);

      if (sensor.init()) {
         Serial.printf("Sensor ready.");
     } else {
         Serial.printf("Sensor ERROR!");
     }

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  display.display();
  delay(2000);
  display.clearDisplay();

   status= bme.begin(0x76);
}


void loop() {
   temp = bme.readTemperature();
   press = bme.readPressure();
   humid = bme.readHumidity(); 
   Fah = temptoFah(temp);
   inHg = presstoinHg(press);
 
    int quality = sensor.slope();

     Serial.printf("Sensor value: ");
     Serial.printf("Sensor value: %d\n", sensor.getValue());

     if (quality == AirQualitySensor::FORCE_SIGNAL) {
         Serial.printf("High pollution! Force signal active.");
     } else if (quality == AirQualitySensor::HIGH_POLLUTION) {
         Serial.printf("High pollution!");
     } else if (quality == AirQualitySensor::LOW_POLLUTION) {
         Serial.printf("Low pollution!");
     } else if (quality == AirQualitySensor::FRESH_AIR) {
         Serial.printf("Fresh air.");
     }

     delay(1000);

    if (timer == true) {
    Serial.printf("%0.1f %0.2f %0.1f", Fah,inHg,humid);

    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0,0); 
    display.printf("%0.1f %0.1f %0.1f", Fah,inHg,humid);
    display.display();
    display.clearDisplay();
  }
}

 float temptoFah(float measurement){
    float result;
    result = (9/5.0)*measurement+32;
    return result;
}

    float presstoinHg(float measurement){
    float reading;
    reading = .0002*press;
    return reading;
}