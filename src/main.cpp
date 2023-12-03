#include <time.h> // Thư viện time.h để lấy thời gian thực tế
#include <Adafruit_Fingerprint.h>
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <FirebaseESP32.h>
#include <ESP32Servo.h>
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "asia.pool.ntp.org", 7 * 60 * 60); // Điều chỉnh để sử dụng múi giờ của Việt Nam (UTC+7)
Servo servo;
#define MODEM_RX 16
#define MODEM_TX 17
#define LED 5

#define mySerial Serial2 // Dùng cho ESP32
#define FIREBASE_HOST "p-care-e73a4-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "2i7E1m5dyVcBg5z0JIZPgxSERFC3JC8h1TIVvc8N"
#define WIFI_SSID "Phong May 1"
#define WIFI_PASSWORD "phongmay1"

FirebaseData fbdo;
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
uint8_t getFingerprintIDez();
void setup()
{
  servo.attach(15);
  servo.write(0);
  pinMode(LED, OUTPUT);
  Serial.begin(9600);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  Serial.println("Connected to WiFi");

  configTime(7 * 3600, 0, "asia.pool.ntp.org"); // Cấu hình múi giờ của Việt Nam (UTC+7)

  Serial.println("\n\nAdafruit finger detect test");

  // set the data rate for the sensor serial port
  finger.begin(57600);

  if (finger.verifyPassword())
  {
    Serial.println("Found fingerprint sensor!");
  }
  else
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

void loop()
{
  getFingerprintIDez();
  delay(50);
}

uint8_t getFingerprintIDez()
{
  timeClient.update();
  String Time = timeClient.getFormattedTime();

  uint8_t p = finger.getImage(); // Dòng này lấy dữ liệu hình ảnh từ cảm biến vân tay.

  switch (p)
  {
  case FINGERPRINT_OK:
    Serial.println("Image taken");
    break;
  case FINGERPRINT_NOFINGER:
    Serial.println("No finger detected");
    Serial.println("đèn tắt");

    digitalWrite(LED, LOW);

    return p;
  case FINGERPRINT_PACKETRECIEVEERR:
    Serial.println("Communication error");
    return p;
  case FINGERPRINT_IMAGEFAIL:
    Serial.println("Imaging error");
    return p;
  default:
    Serial.println("Unknown error");
    return p;
  }

  // OK success!
  p = finger.image2Tz(); // Dòng này chuyển đổi dữ liệu hình ảnh thành mẫu vân tay để so sánh.
  switch (p)
  {
  case FINGERPRINT_OK:
    Serial.println("Image converted");
    break;
  case FINGERPRINT_IMAGEMESS:
    Serial.println("Image too messy");
    return p;
  case FINGERPRINT_PACKETRECIEVEERR:
    Serial.println("Communication error");
    return p;
  case FINGERPRINT_FEATUREFAIL:
    Serial.println("Could not find fingerprint features");
    return p;
  case FINGERPRINT_INVALIDIMAGE:
    Serial.println("Could not find fingerprint features");
    return p;
  default:
    Serial.println("Unknown error");
    return p;
  }

  // OK converted!
  p = finger.fingerFastSearch(); // Dòng này tìm kiếm nhanh vân tay trong cơ sở dữ liệu đã được tạo trước đó và xác định xem có sự khớp hay không.
  if (p == FINGERPRINT_OK)
  {
    Serial.println("Found a print match!");
  }
  else if (p == FINGERPRINT_PACKETRECIEVEERR)
  {
    Serial.println("Communication error");
    return p;
  }
  else if (p == FINGERPRINT_NOTFOUND)
  {
    Serial.println("Did not find a match");
    return p;
  }
  else
  {
    Serial.println("Unknown error");
    return p;
  }

  // found a match!
  Serial.print("Found ID #");
  Serial.print(finger.fingerID);
  Serial.print(" with confidence of ");
  Serial.println(finger.confidence);

  // Lấy thời gian thực tế
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return 0;
  }

  // Format ngày tháng năm thành chuỗi YYYY-MM-DD
  char Day[20];
  strftime(Day, sizeof(Day), "%Y-%m-%d", &timeinfo);

  // Cập nhật giá trị lên Firebase
  if (Firebase.get(fbdo, "/BYT/11A1/students/student" + String(finger.fingerID)))
  {
    digitalWrite(LED, HIGH);
    servo.write(90);
    Serial.println("đèn sáng");
    delay(10000);

    servo.write(0);
    if (!Firebase.setStringAsync(fbdo, "/BYT/11A1/students/student" + String(finger.fingerID) + "/Time", Time) || !Firebase.setStringAsync(fbdo, "/BYT/11A1/students/student" + String(finger.fingerID) + "/Day", Day))
    {
      Serial.println(fbdo.errorReason());
    }
  }
  else if (Firebase.get(fbdo, "/BYT/11A2/students/student" + String(finger.fingerID)))
  {
    digitalWrite(LED, HIGH);
    servo.write(90);
    Serial.println("đèn sáng");
    delay(10000);
    servo.write(0);
    if (!Firebase.setStringAsync(fbdo, "/BYT/11A2/students/student" + String(finger.fingerID) + "/Time", Time) || !Firebase.setStringAsync(fbdo, "/BYT/11A2/students/student" + String(finger.fingerID) + "/Day", Day))
    {
      Serial.println(fbdo.errorReason());
    }
  }
  else
  {
    Serial.println("Không có học sinh này ");
    digitalWrite(LED, HIGH);
  }

  return finger.fingerID;
}