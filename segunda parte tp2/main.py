#include <WiFi.h>
#include <ThingSpeak.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------------- WIFI ----------------
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// ---------------- THINGSPEAK ----------------
unsigned long channelID = 3365192;
const char* readAPIKey = "YG5NS6WNVP60V4M6";

WiFiClient client;

// ---------------- OLED ----------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define I2C_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ---------------- BOTON ----------------
#define BTN 19

// ---------------- CONTROL ----------------
int pantalla = 0;
unsigned long ultimaLectura = 0;
const unsigned long intervalo = 5000;

// ---------------- VARIABLES ----------------
float campo1 = 0, temp = 0, hum = 0;
long uptime = 0;
String estado = "Sin datos";

// ---------------- SETUP ----------------
void setup() {
  Serial.begin(115200);

  Wire.begin(21, 22);
  delay(100);

  pinMode(BTN, INPUT_PULLUP);

  // ---- INICIALIZAR OLED ----
  if (!display.begin(SSD1306_SWITCHCAPVCC, I2C_ADDRESS)) {
    Serial.println("Error OLED");
    while (true);
  }

  // ---- MENSAJE INICIAL (CLAVE) ----
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0, 20);
  display.println("INICIANDO");
  display.display();

  delay(1500);

  // ---- WIFI (SIN BLOQUEO INFINITO) ----
  Serial.print("Conectando WiFi...");
  WiFi.begin(ssid, password);

  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 20) {
    delay(500);
    Serial.print(".");
    intentos++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi OK");
  } else {
    Serial.println("\nWiFi FALLÓ");
  }

  ThingSpeak.begin(client);

  // Mostrar primera pantalla
  mostrarPantalla();
}

// ---------------- LOOP ----------------
void loop() {

  // ---- BOTON ----
  if (digitalRead(BTN) == LOW) {
    pantalla = (pantalla + 1) % 3;
    mostrarPantalla();
    delay(300);
  }

  // ---- RECONEXION WIFI ----
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, password);
  }

  // ---- LECTURA ----
  if (millis() - ultimaLectura > intervalo) {

    if (WiFi.status() == WL_CONNECTED) {

      int httpCode = ThingSpeak.readMultipleFields(channelID, readAPIKey);

      if (httpCode == 200) {
        campo1 = ThingSpeak.getFieldAsFloat(1);
        temp   = ThingSpeak.getFieldAsFloat(2);
        hum    = ThingSpeak.getFieldAsFloat(3);
        uptime = ThingSpeak.getFieldAsLong(4);
        estado = ThingSpeak.getStatus();
      } else {
        estado = "Error TS";
      }

    } else {
      estado = "Sin WiFi";
    }

    mostrarPantalla();
    pantalla = (pantalla + 1) % 3;

    ultimaLectura = millis();
  }
}

// ---------------- DISPLAY ----------------
void mostrarPantalla() {

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);

  // -------- PANTALLA 1 --------
  if (pantalla == 0) {
    display.setCursor(0, 0);
    display.println("DATOS GENERALES");
    display.drawLine(0, 10, 127, 10, WHITE);

    display.setCursor(0, 20);
    display.print("Valor: ");
    display.println(campo1, 2);

    display.print("Uptime: ");
    display.println(uptime);
  }

  // -------- PANTALLA 2 --------
  if (pantalla == 1) {
    display.setCursor(0, 0);
    display.println("SENSORES");
    display.drawLine(0, 10, 127, 10, WHITE);

    display.setCursor(0, 20);
    display.print("Temp: ");
    display.print(temp, 1);
    display.println(" C");

    display.print("Hum: ");
    display.print(hum, 1);
    display.println(" %");
  }

  // -------- PANTALLA 3 --------
  if (pantalla == 2) {
    display.setCursor(0, 0);
    display.println("ESTADO");
    display.drawLine(0, 10, 127, 10, WHITE);

    display.setCursor(0, 20);
    display.println(estado);
  }

  display.display();
}