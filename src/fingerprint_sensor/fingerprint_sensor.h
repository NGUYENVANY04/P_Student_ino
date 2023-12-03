// fingerprint_sensor.h

#ifndef FINGERPRINT_SENSOR_H
#define FINGERPRINT_SENSOR_H

#include <Adafruit_Fingerprint.h>
#include <WiFi.h>

class FingerprintSensor
{
public:
    FingerprintSensor(HardwareSerial &serial);
    void initialize();
    uint8_t getFingerprintIDez();

private:
    Adafruit_Fingerprint finger;
};

#endif
