// fingerprint_sensor.cpp

#include "fingerprint_sensor.h"

FingerprintSensor::FingerprintSensor(HardwareSerial &serial) : finger(&serial) {}

void FingerprintSensor::initialize()
{
    Serial.println("\n\nAdafruit finger detect test");
    finger.begin(57600);
    if (!finger.verifyPassword())
    {
        Serial.println("Did not find fingerprint sensor :(");
        while (1)
        {
            delay(1);
        }
    }
    finger.getTemplateCount();
    Serial.print("Sensor contains ");
    Serial.print(finger.templateCount);
    Serial.println(" templates");
    Serial.println("Waiting for valid finger...");
}

uint8_t FingerprintSensor::getFingerprintIDez()
{
    // Implement your existing getFingerprintIDez() logic here...
    // ...
}
