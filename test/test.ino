#include <Arduino.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <DHT.h>
#include "MQ135.h"
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP32Servo.h>
#define DHTPIN 4      // Digital pin connected to the DHT sensor
#define DHTTYPE DHT11 // DHT 11
#define MQ_135 35     // Connected to pin 35
#define FLAME 14      // Connected to pin 14
#define GAS_PIN 34    // Connected to pin 34
#define coi 15        // Connected to pin 15
#define LED 5
#define RAIN_SENSOR 12
#define LIGHT 26
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "asia.pool.ntp.org", 7 * 60 * 60); // Điều chỉnh để sử dụng múi giờ của Việt Nam (UTC+7)

DHT dht(DHTPIN, DHTTYPE);
MQ135 mq135(MQ_135);
Servo servo;
#define FIREBASE_HOST "android-flutter-8e415-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "bZMt8bWQCQK7gfwxSwMI6YROwGVHOxHsoN07itAH"
#define WIFI_SSID "Kullhi23"
#define WIFI_PASSWORD "huyhuyhuy"

FirebaseData fbdo;

const unsigned long MQ135_INTERVAL = 5000; // Read MQ135 data every 5 seconds
const unsigned long DHT_INTERVAL = 2000;   // Read DHT data every 2 seconds

unsigned long previousMq135Time = 0;
unsigned long previousDhtTime = 0;

void sent_data_dht(int data_temp, int data_humidity)
{
    if (!isnan(data_temp) && !isnan(data_humidity))
    {
        if (!Firebase.setInt(fbdo, "/home/temp_data", data_temp) || !Firebase.setInt(fbdo, "/home/humidity_data", data_humidity))
        {
            Serial.println("Error sending DHT data to Firebase: " + fbdo.errorReason());
        }
    }
    else
    {
        Serial.println(F("Failed to read from DHT sensor!"));
    }
}
void sent_state_fire(bool state_fire)
{
    if (!Firebase.setBool(fbdo, "/home/state_fire", state_fire))
    {
        Serial.println(fbdo.errorReason());
    }
}
void sent_state_gas(bool state_gas)
{
    if (!Firebase.setBool(fbdo, "/home/state_gas", state_gas))
    {
        Serial.println(fbdo.errorReason());
    }
}
void sentDataMq135(int data_mq135)
{
    if (!Firebase.setInt(fbdo, "/home/mq135_data", data_mq135))
    {
        Serial.println("Error sending MQ135 data to Firebase: " + fbdo.errorReason());
    }
}
void sentDataRain(bool state)
{
    if (!Firebase.setInt(fbdo, "/home/state_covered", state))
    {
        Serial.println(fbdo.errorReason());
    }
}

void connectToWiFi()
{
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(500);
    }
    Serial.println("Connected to WiFi");
}

void setupFirebase()
{
    Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
}
int flame_sensor_read;
int gas_value;
int data_rain;
void setup()
{
    Serial.begin(115200);
    dht.begin();
    pinMode(MQ_135, INPUT);
    pinMode(FLAME, INPUT);
    pinMode(LIGHT, OUTPUT);
    pinMode(GAS_PIN, INPUT); // Gas sensor will be an input to the ESP32
    pinMode(RAIN_SENSOR, INPUT);
    connectToWiFi();
    setupFirebase();
    pinMode(coi, OUTPUT);
    pinMode(LED, OUTPUT);
    digitalWrite(coi, LOW);
    configTime(7 * 3600, 0, "asia.pool.ntp.org"); // Cấu hình múi giờ của Việt Nam (UTC+7)
    servo.attach(25);
    servo.write(0);
}
struct TimeInfo
{
    int hour;
    int minute;
};
TimeInfo currentTime;
bool state_curtain_sensor = fasle;
bool state_curtain_app = fasle;

void loop()
{
    control_light();
    warning();
    unsigned long currentMillis = millis();

    // Read data from the MQ135 sensor every 5 seconds
    if (currentMillis - previousMq135Time >= MQ135_INTERVAL)
    {
        int air_quality = mq135.getPPM();
        sentDataMq135(air_quality);
        previousMq135Time = currentMillis;
    }

    // Read data from the DHT sensor every 2 seconds
    if (currentMillis - previousDhtTime >= DHT_INTERVAL)
    {
        int temperature = dht.readTemperature();
        int humidity = dht.readHumidity();

        sent_data_dht(temperature, humidity);

        previousDhtTime = currentMillis;
    } // Read data from the Flame sensor every 2 seconds
}
void warning()
{
    bool state_fire = false;
    bool state_gas = false;
    flame_sensor_read = digitalRead(FLAME); // Đọc dữ liệu từ cảm biến lửa trên GPIO 14
    gas_value = analogRead(GAS_PIN);
    data_rain = digitalRead(RAIN_SENSOR);
    if (data_rain == 0)
    {
        servo.write(90);
        sentDataRain(true);
        state_curtain_sensor = true;
    }
    else
    {
        servo.write(0);
        sentDataRain(false);
        state_curtain_sensor = false;
    }
    //***************************************************
    if (Firebase.getBool(fbdo, "home/state_covered", &state_curtain_app))
    {
        if (state_curtain_app == true && state_curtain_sensor == false)
        {
            servo.write(90);
            state _curtain_sensor = true;
        }
        else if (state_curtain_app == false && state_curtain_sensor == true)
        {
            servo.write(0);
            state _curtain_sensor = false;
        }
    }
    //***************************************************
    Serial.print(gas_value);
    if (flame_sensor_read == 0)
    {
        state_fire = true;
        Serial.println("Lửa đang cháy");
    }

    if (gas_value > 2000)
    {
        state_gas = true;
        Serial.println("Khí gas vượt ngưỡng an toàn");
    }

    if (state_fire || state_gas)
    {
        digitalWrite(coi, HIGH);
        delay(5000);
        // digitalWrite(coi, LOW);
    }
    else
    {
        digitalWrite(coi, LOW);
    }

    sent_state_fire(state_fire);
    sent_state_gas(state_gas);
    control_led_on();
    control_led_off();
}

int hour_get = 0;   // Giá trị mặc định hoặc giá trị mong muốn
int minute_get = 0; // Giá trị mặc định hoặc giá trị mong muốn

// Hàm để cập nhật giờ và phút từ Firebase
bool updateTimeFromFirebase(String path)
{
    timeClient.update();
    currentTime.hour = timeClient.getHours();
    currentTime.minute = timeClient.getMinutes();

    // Lấy giá trị từ Firebase và gán vào hour_get và minute_get
    return Firebase.getInt(fbdo, path + "/h", &hour_get) && Firebase.getInt(fbdo, path + "/m", &minute_get);
}

// Hàm kiểm tra và điều khiển đèn
void controlLED(bool isTurnOn, String path, int pin)
{
    if (updateTimeFromFirebase(path))
    {
        if (currentTime.hour == hour_get && currentTime.minute == minute_get)
        {
            digitalWrite(pin, isTurnOn ? HIGH : LOW);
        }
    }
}

void control_led_on()
{
    controlLED(true, "/home/turn_on", LED);
}

void control_led_off()
{
    controlLED(false, "/home/turn_off", LED);
}
bool state_light = true;
void control_light()
{
    if (Firebase.getBool(fbdo, "/home/light", &state_light))
    {
        if (state_light == true)
        {
            digitalWrite(LIGHT, HIGH);
        }
        else
            digitalWrite(LIGHT, LOW);
    }
}