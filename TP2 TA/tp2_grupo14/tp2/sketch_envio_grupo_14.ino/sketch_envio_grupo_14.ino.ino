/*
 * ============================================================
 *  TP N°2 – IoT
 *  Sketch 1: Envío de datos a ThingSpeak + OTA Web
 * ============================================================
 *  Hardware:
 *    - ESP32
 *    - Display OLED SH1106 128x64 I2C (SDA=21, SCL=22)
 *    - DHT22 en GPIO 33
 *    - Pulsador en GPIO 19 (pull-up interno)
 * ============================================================ */

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ThingSpeak.h>
#include <Ticker.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <DHT.h>
#include "secrets.h"

// ---------- WiFi ----------
const char* ssid     = SECRET_SSID;
const char* password = SECRET_PASS;

// ---------- ThingSpeak ----------
unsigned long CHANNEL_ID    = SECRET_CH_ID;
const char*   WRITE_API_KEY = SECRET_WRITE_APIKEY;

// ---------- OTA ----------
WebServer   server(80);
const char* host = "esp32";
#include "OTA.h"

// ---------- Hardware ----------
#define DHTPIN        33
#define DHTTYPE       DHT22
#define BUTTON_PIN    19
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDR     0x3C

DHT              dht(DHTPIN, DHTTYPE);
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
WiFiClient       wifiClient;
Ticker           enviarDatos;
volatile bool    enviarFlag = false;

// ---------- Estados ----------
const char* ESTADOS[]   = {"No disponible", "En reparacion", "Habilitado", "Fuera de servicio"};
const int   NUM_ESTADOS = 4;
int         estadoActual = 0;

// ---------- Debounce ----------
bool     lastRawState   = HIGH;
bool     debouncedState = HIGH;
uint32_t lastDebounceMs = 0;
const uint16_t DEBOUNCE_DELAY = 50;

uint32_t startMillis = 0;

// ============================================================
void checkButton();
void realizarEnvio();

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  display.begin(OLED_ADDR, false);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println("TP2 IoT");
  display.println("Iniciando...");
  display.display();

  dht.begin();

  WiFi.begin(ssid, password);
  Serial.print("Conectando a WiFi");
  display.println("Conectando WiFi...");
  display.display();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi OK – IP: " + WiFi.localIP().toString());
  display.println("WiFi OK");
  display.println(WiFi.localIP().toString());
  display.display();
  delay(1500);

  iniciaOTA();
  ThingSpeak.begin(wifiClient);

  display.clearDisplay();
  display.setCursor(0, 0);  display.println("-- OTA Listo --");
  display.setCursor(0, 14); display.println("Abrir en browser:");
  display.setCursor(0, 26); display.println("http://");
  display.println(WiFi.localIP().toString());
  display.setCursor(0, 50); display.println("user:admin  pass:admin");
  display.display();
  delay(5000);

  startMillis = millis();
  enviarDatos.attach(16, []() { enviarFlag = true; });

  Serial.println("[OTA] http://" + WiFi.localIP().toString());
  Serial.println("[OTA] Usuario: admin | Contrasena: admin");
  Serial.println("=== SKETCH 1 LISTO ===");
}

// ============================================================
void loop() {
  server.handleClient();
  if (enviarFlag) {
    enviarFlag = false;
    realizarEnvio();
  }
  checkButton();
  delay(1);
}

// ============================================================
void checkButton() {
  bool reading = digitalRead(BUTTON_PIN);
  if (reading != lastRawState) {
    lastDebounceMs = millis();
    lastRawState   = reading;
  }
  if ((millis() - lastDebounceMs) > DEBOUNCE_DELAY) {
    if (reading == LOW && debouncedState == HIGH) {
      debouncedState = LOW;
      estadoActual = (estadoActual + 1) % NUM_ESTADOS;
      Serial.printf("[BOTON] Estado: %s\n", ESTADOS[estadoActual]);
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0, 0);  display.println("--- Estado ---");
      display.setCursor(0, 20); display.println(ESTADOS[estadoActual]);
      display.display();
    } else if (reading == HIGH) {
      debouncedState = HIGH;
    }
  }
}

// ============================================================
void realizarEnvio() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Sin conexion, omitiendo envio.");
    return;
  }

  float temperatura = dht.readTemperature();
  float humedad     = dht.readHumidity();

  if (isnan(temperatura)) temperatura = 20.0 + random(0, 100) / 10.0;
  if (isnan(humedad))     humedad     = 40.0 + random(0, 400) / 10.0;

  int  aleatorio = random(100, 501);
  long tiempoSeg = (millis() - startMillis) / 1000;

  ThingSpeak.setField(1, aleatorio);
  ThingSpeak.setField(2, temperatura);
  ThingSpeak.setField(3, humedad);
  ThingSpeak.setField(4, (int)tiempoSeg);
  ThingSpeak.setStatus(ESTADOS[estadoActual]);

  int httpCode = ThingSpeak.writeFields(CHANNEL_ID, WRITE_API_KEY);

  Serial.println("------------------------------------");
  Serial.printf("[TS] Aleatorio  : %d\n",      aleatorio);
  Serial.printf("[TS] Temperatura: %.1f C\n",  temperatura);
  Serial.printf("[TS] Humedad    : %.1f %%\n", humedad);
  Serial.printf("[TS] Tiempo     : %ld s\n",   tiempoSeg);
  Serial.printf("[TS] Estado     : %s\n",      ESTADOS[estadoActual]);
  if (httpCode == 200) Serial.println("[TS] OK");
  else Serial.printf("[TS] ERROR HTTP %d\n", httpCode);
  Serial.println("------------------------------------");

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);  display.printf("Temp : %.1f C",   temperatura);
  display.setCursor(0, 12); display.printf("Hum  : %.1f %%",  humedad);
  display.setCursor(0, 24); display.printf("Rand : %d",       aleatorio);
  display.setCursor(0, 36); display.printf("t    : %ld s",    tiempoSeg);
  display.setCursor(0, 50); display.printf("St: %s",          ESTADOS[estadoActual]);
  display.display();
}