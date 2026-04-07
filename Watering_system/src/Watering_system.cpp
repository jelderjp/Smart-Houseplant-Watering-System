/* 
 * Project Smart Houseplant Watering System
 * Author: Jay Elder
 * Date: 03/19/2026
 */


#include "Particle.h"
#include "credentials.h"
#include "Adafruit_BME280.h"
#include "Adafruit_SSD1306.h"
#include "Adafruit_GFX.h"
#include "IotClassroom_CNM.h"
#include "Grove_Air_quality_Sensor.h"
#include <Adafruit_MQTT.h>
#include "Adafruit_MQTT/Adafruit_MQTT_SPARK.h"
#include "Adafruit_MQTT/Adafruit_MQTT.h"

TCPClient TheClient; 
Adafruit_MQTT_SPARK mqtt(&TheClient,AIO_SERVER,AIO_SERVERPORT,AIO_USERNAME,AIO_KEY); 

Adafruit_MQTT_Subscribe pumpFeed = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/Pump"); 
Adafruit_MQTT_Publish sensorFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Temperature, Moisture, Humidity, Air_quality");
Adafruit_MQTT_Publish pubAQ = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Air_Quality");  
Adafruit_MQTT_Publish pubTemp = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Temperature"); 
Adafruit_MQTT_Publish pubMoist = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Moisture"); 
Adafruit_MQTT_Publish pubHumid = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Humidity");
Adafruit_MQTT_Publish pubPump = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Pump");

SYSTEM_MODE(AUTOMATIC);

const int OLED_RESET=-1;
Adafruit_SSD1306 display(OLED_RESET);
Adafruit_BME280 bme;
AirQualitySensor airSensor(A5);

const int soilSensor = A0;
const int pump = D9;
bool status;
bool bmeStatus = false;
bool pumpRunning = false;
float tempC = 0.0;
float tempF = 0.0;
float pressurePa = 0.0;
float pressureInHg = 0.0;
float humid = 0.0;
int soilRead = 0;
int airQuality = 0;
bool pumpCommand;
float tempToFah(float measurement);
float pressureToInHg(float measurementPa);
void MQTT_connect();
bool MQTT_ping();
unsigned long lastDisplayMs = 0;
unsigned long lastReadMs = 0;
unsigned long lastPublishMs = 0;
unsigned long pumpUntilMs = 0;


void setup() {
  
  Serial.begin(9600);
    waitFor(Serial.isConnected, 10000);
    
    pinMode(pump, OUTPUT);
    digitalWrite(pump, LOW);

    pinMode(soilSensor, INPUT);

    mqtt.subscribe(&pumpFeed);
  
    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.display();
    
    bmeStatus = bme.begin(0x76);
    if (bmeStatus) {
        Serial.printf("BME280 ready.");
    } else {
        Serial.printf("BME280 not found.");
    }

    Serial.printf("Initializing air quality sensor...");
    delay(2000);
    if (airSensor.init()) {
        Serial.printf("Air quality sensor ready.");
    } else {
        Serial.printf("Air quality sensor ERROR.");
    }
 }


void loop() {
    MQTT_connect();
    MQTT_ping();

    Adafruit_MQTT_Subscribe *subscription;
    while ((subscription = mqtt.readSubscription())) {
        if (subscription == &pumpFeed) {
            pumpCommand = atof((char *)pumpFeed.lastread);
            Serial.printf("Pump feed value: %f\n", pumpCommand);

            if (pumpCommand > 0.5) {
                 pumpUntilMs = millis() + 500; pubPump.publish(0.0);  // reset feed so it doesn't re-trigger}
            if (pumpCommand > 0.5 && !pumpRunning) {
                    pumpUntilMs = millis() + 500;
                    pumpRunning = true;
    }
        if (pumpCommand <= 0.5) {
            pumpRunning = false;
            }
        }
     }

    if (millis() < pumpUntilMs) {
        digitalWrite(pump, HIGH);
    } else {
        digitalWrite(pump, LOW);
    }
    }
}
    if (millis() - lastReadMs >= 5000) {
        lastReadMs = millis();

        if (bmeStatus) {
            tempC = bme.readTemperature();
            pressurePa = bme.readPressure();
            humid = bme.readHumidity();

            tempF = tempToFah(tempC);
            pressureInHg = pressureToInHg(pressurePa);
        }

        soilRead = analogRead(soilSensor);
        airQuality = airSensor.slope();

        Serial.printf("TempF: %.1f  Press: %.2f inHg  Humid: %.1f  Soil: %d  AQ raw: %d\n",
                      tempF, pressureInHg, humid, soilRead, airSensor.getValue());

        if (airQuality == AirQualitySensor::FORCE_SIGNAL) {
            Serial.printf("Air quality: High pollution! Force signal active.");
        } else if (airQuality == AirQualitySensor::HIGH_POLLUTION) {
            Serial.printf("Air quality: High pollution.");
        } else if (airQuality == AirQualitySensor::LOW_POLLUTION) {
            Serial.printf("Air quality: Low pollution.");
        } else if (airQuality == AirQualitySensor::FRESH_AIR) {
            Serial.printf("Air quality: Fresh air.");
        }
    }

    if (millis() - lastDisplayMs >= 10000) {
        lastDisplayMs = millis();

        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.printf("Temp: %.1f F\n", tempF);
        display.printf("Pres: %.2f inHg\n", pressureInHg);
        display.printf("Hum : %.1f %%\n", humid);
        display.printf("Soil: %d\n", soilRead);
        display.printf("Air : %d\n", airQuality);
        display.display();
    }

    if (millis() - lastPublishMs >= 10000) {
        lastPublishMs = millis();

        if (mqtt.connected()) {
            pubTemp.publish(tempF);
            pubMoist.publish(soilRead);
            pubHumid.publish(humid);
            pubAQ.publish(airQuality);

            Serial.printf("Published sensor data to Adafruit IO.");
            }
       }
    
    float tempToFah(float measurement) {
    return (9.0 / 5.0) * measurement + 32.0;
}

    float pressureToInHg(float measurementPa) {
    return measurementPa * 0.0002953;
}

    void MQTT_connect() {
    if (mqtt.connected()) {
        return;
    }

    Serial.printf("Connecting to MQTT... ");

    int8_t ret;
    while ((ret = mqtt.connect()) != 0) {
        Serial.printf("MQTT connect failed: %s\n", mqtt.connectErrorString(ret));
        Serial.printf("Retrying MQTT connection in 5 seconds...");
        mqtt.disconnect();
        delay(5000);
    }

    Serial.printf("MQTT connected.");
}

    bool MQTT_ping() {
    static unsigned long lastPing = 0;

    if (millis() - lastPing > 120000) {
        lastPing = millis();
        if (!mqtt.ping()) {
            mqtt.disconnect();
            return false;
        }
    }
    return true;
    }
