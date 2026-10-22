// =====================================
// BLYNK CONFIG
// =====================================
#define BLYNK_TEMPLATE_ID "TMPL6f0e3c1b7d8e4f9a9"
#define BLYNK_TEMPLATE_NAME "Smart Trashbin"
#define BLYNK_AUTH_TOKEN "YOUR_BLYNK_AUTH_TOKEN"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// =====================================
// LIBRARY UTAMA
// =====================================
#include <ESP32Servo.h>
#include <HTTPClient.h>

#define TRIG_TANGAN 5
#define ECHO_TANGAN 18
#define TRIG_PENUH  4
#define ECHO_PENUH  2
#define BUZZER_PIN  14
#define SERVO_PIN   13
#define LED_MERAH   25
#define LED_HIJAU   26

// WiFi
char ssid[] = "YOUR_WIFI_SSID";
char pass[] = "YOUR_WIFI_PASSWORD";

// Telegram
String BOT_TOKEN = "YOUR_TELEGRAM_BOT_TOKEN";
String CHAT_ID   = "YOUR_TELEGRAM_CHAT_ID";

Servo servo;

bool sudahKirimPesanPenuh = false;
bool sudahKirimPesanKosong = false;
bool servoSedangGerak = false;

// =====================================
// BLYNK VIRTUAL PINS
// =====================================
#define VP_STATUS   V0
#define VP_JARAK_PENUH V1
#define VP_JARAK_TANGAN V2

// =====================================
// BACA JARAK
// =====================================
float bacaJarak(int trig, int echo) {
  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  long duration = pulseIn(echo, HIGH, 30000);
  if (duration == 0) return 999;
  return duration * 0.034 / 2;
}

// =====================================
// TELEGRAM
// =====================================
void kirimTelegram(String pesan) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "https://api.telegram.org/bot" + BOT_TOKEN +
                 "/sendMessage?chat_id=" + CHAT_ID + "&text=" + pesan;
    http.begin(url);
    http.GET();
    http.end();
  }
}

// =====================================
// UPDATE BLYNK
// =====================================
void updateBlynk(float jarakPenuh, float jarakTangan, bool penuh) {
  Blynk.virtualWrite(VP_JARAK_PENUH, (int)jarakPenuh);
  Blynk.virtualWrite(VP_JARAK_TANGAN, (int)jarakTangan);
  // Blynk.virtualWrite(VP_JARAK_TANGAN, (int)penuh);

  Serial.print("Jarak Penuh : ");
  Serial.println(jarakPenuh);

  Serial.print("Jarak Tangan : ");
  Serial.println(jarakTangan);
  
  Serial.print("Penuh? : ");
  Serial.println(penuh);

  if (penuh) {
    Blynk.virtualWrite(VP_STATUS, "PENUH");
  } else {
    Blynk.virtualWrite(VP_STATUS, "TIDAK PENUH");
  }
}

// =====================================
// SETUP
// =====================================
void setup() {
  Serial.begin(115200);

  pinMode(TRIG_TANGAN, OUTPUT);
  pinMode(ECHO_TANGAN, INPUT);
  pinMode(TRIG_PENUH, OUTPUT);
  pinMode(ECHO_PENUH, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(LED_HIJAU, OUTPUT);

  servo.attach(SERVO_PIN);
  servo.write(0);

  // =========================================================
  // WI-FI + BLYNK CONNECT
  // =========================================================
  Serial.println("Menghubungkan ke Blynk...");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  Serial.println("Blynk Connected!");

  Serial.println("=== Smart Trash Bin Aktif ===");
}

// =====================================
// LOOP UTAMA
// =====================================
void loop() {
  Blynk.run();  // <--- WAJIB UNTUK BLYNK

  float jarakTangan = bacaJarak(TRIG_TANGAN, ECHO_TANGAN);

  // Sensor penuh dimatikan jika servo sedang bergerak
  float jarakPenuh = servoSedangGerak ? 999 : bacaJarak(TRIG_PENUH, ECHO_PENUH);

  Serial.print("Tangan: "); Serial.print(jarakTangan);
  Serial.print(" | Penuh: "); Serial.println(jarakPenuh);

  bool penuh = (jarakPenuh <= 10 && jarakPenuh > 0);

  // =====================================
  // KIRIM DATA MONITORING KE BLYNK
  // =====================================
  updateBlynk(jarakPenuh, jarakTangan, penuh);

  // =====================================
  // KONDISI PENUH
  // =====================================
  if (penuh) {
    digitalWrite(LED_MERAH, HIGH);
    digitalWrite(LED_HIJAU, LOW);

    tone(BUZZER_PIN, 2000);
    delay(200);
    noTone(BUZZER_PIN);
    delay(200);

    servo.write(0);

    if (!sudahKirimPesanPenuh) {
      kirimTelegram("🚮 Tong sampah penuh! Segera dikosongkan.");
      sudahKirimPesanPenuh = true;
      sudahKirimPesanKosong = false;
    }
  }

  // =====================================
  // KONDISI TIDAK PENUH
  // =====================================
  else {
    digitalWrite(LED_HIJAU, HIGH);
    digitalWrite(LED_MERAH, LOW);
    noTone(BUZZER_PIN);

    if (!sudahKirimPesanKosong) {
      kirimTelegram("✅ Tong sampah siap digunakan kembali.");
      sudahKirimPesanKosong = true;
      sudahKirimPesanPenuh = false;
    }

    // Buka tutup jika tangan dekat
    if (jarakTangan <= 15 && jarakTangan > 0) {
      servoSedangGerak = true;
      servo.write(90);
      delay(2000);
      servo.write(0);
      delay(600);
      servoSedangGerak = false;
    }
  }

  delay(100);
}
