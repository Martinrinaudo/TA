
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ThingSpeak.h>
#include <Ticker.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "secrets.h"

// ============================================================
// WiFi
// ============================================================

const char* ssid     = SECRET_SSID;
const char* password = SECRET_PASS;

// ============================================================
// ThingSpeak
// ============================================================

unsigned long CHANNEL_ID   = SECRET_CH_ID;
const char*   READ_API_KEY = SECRET_READ_APIKEY;

// ============================================================
// OTA WEB
// ============================================================

WebServer server(80);

const char* host = "esp32";

#include "OTA.h"

// ============================================================
// Display OLED SH1106
// ============================================================

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
#define OLED_ADDR     0x3C

Adafruit_SH1106G display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

// ============================================================
// Objetos
// ============================================================

WiFiClient wifiClient;

Ticker leerDatos;
Ticker rotarPantalla;

volatile bool leerFlag  = false;
volatile bool rotarFlag = false;

// ============================================================
// Variables de datos
// ============================================================

float  val_aleatorio    = 0;
float  val_temperatura  = 0;
float  val_humedad      = 0;
float  val_tiempo       = 0;

String val_estado       = "---";

bool datosDisponibles   = false;

int screenPage = 0;

// ============================================================
// Prototipos
// ============================================================

void realizarLectura();
void rotarDisplay();
void mostrarPagina(int pagina);

// ============================================================
// SETUP
// ============================================================

void setup() {

  Serial.begin(115200);

  Serial.println("\n[SKETCH 2] Grupo 1 – Lectura desde ThingSpeak");

  // ========================================================
  // Inicialización OLED
  // ========================================================

  display.begin(OLED_ADDR, true);

  display.clearDisplay();

  display.setTextSize(1);

  display.setTextColor(SH110X_WHITE);

  display.setCursor(0, 0);

  display.println("TP2 IoT - Grupo 1");
  display.println("Sketch 2: Lectura");

  display.display();

  // ========================================================
  // WiFi
  // ========================================================

  WiFi.begin(ssid, password);

  Serial.print("Conectando a WiFi");

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);

    Serial.print(".");
  }

  Serial.println("\nWiFi OK – IP: " + WiFi.localIP().toString());

  display.println("WiFi OK");

  display.println(WiFi.localIP().toString());

  display.display();

  delay(1500);

  // ========================================================
  // OTA WEB
  // ========================================================

  iniciaOTA();

  // ========================================================
  // ThingSpeak
  // ========================================================

  ThingSpeak.begin(wifiClient);

  // ========================================================
  // Pantalla OTA
  // ========================================================

  display.clearDisplay();

  display.setTextSize(1);

  display.setCursor(0, 0);

  display.println("-- OTA Listo --");

  display.setCursor(0, 14);

  display.println("Abrir en browser:");

  display.setCursor(0, 26);

  display.println("http://");

  display.println(WiFi.localIP().toString());

  display.setCursor(0, 50);

  display.println("user:admin pass:admin");

  display.display();

  delay(5000);

  // ========================================================
  // Tickers
  // ========================================================

  leerDatos.attach(16, []() {

    leerFlag = true;

  });

  rotarPantalla.attach(3, []() {

    rotarFlag = true;

  });

  // ========================================================
  // Primera lectura
  // ========================================================

  realizarLectura();

  Serial.println("[SKETCH 2] Listo.");

  Serial.println(
    "[OTA] Abrir en browser: http://"
    + WiFi.localIP().toString()
  );

  Serial.println("[OTA] O usar: http://esp32.local/");

  Serial.println(
    "[OTA] Usuario: admin | Contrasena: admin"
  );
}

// ============================================================
// LOOP
// ============================================================

void loop() {

  // ========================================================
  // Servidor Web OTA
  // ========================================================

  server.handleClient();

  // ========================================================
  // Lectura ThingSpeak
  // ========================================================

  if (leerFlag) {

    leerFlag = false;

    realizarLectura();
  }

  // ========================================================
  // Rotación Display
  // ========================================================

  if (rotarFlag) {

    rotarFlag = false;

    rotarDisplay();
  }

  delay(1);
}

// ============================================================
// LECTURA ThingSpeak
// ============================================================

void realizarLectura() {

  if (WiFi.status() != WL_CONNECTED) {

    Serial.println(
      "[WiFi] Sin conexion, omitiendo lectura."
    );

    return;
  }

  Serial.println(
    "[TS] Leyendo canal ThingSpeak..."
  );

  int statusCode =
    ThingSpeak.readMultipleFields(
      CHANNEL_ID,
      READ_API_KEY
    );

  // ========================================================
  // Lectura exitosa
  // ========================================================

  if (statusCode == 200) {

    val_aleatorio =
      ThingSpeak.getFieldAsFloat(1);

    val_temperatura =
      ThingSpeak.getFieldAsFloat(2);

    val_humedad =
      ThingSpeak.getFieldAsFloat(3);

    val_tiempo =
      ThingSpeak.getFieldAsFloat(4);

    val_estado =
      ThingSpeak.getStatus();

    if (val_estado.length() == 0) {

      val_estado = "Sin estado";
    }

    datosDisponibles = true;

    Serial.println("------------------------------------");

    Serial.printf(
      "[TS] Aleatorio  : %.0f\n",
      val_aleatorio
    );

    Serial.printf(
      "[TS] Temperatura: %.1f C\n",
      val_temperatura
    );

    Serial.printf(
      "[TS] Humedad    : %.1f %%\n",
      val_humedad
    );

    Serial.printf(
      "[TS] Tiempo     : %.0f s\n",
      val_tiempo
    );

    Serial.printf(
      "[TS] Estado     : %s\n",
      val_estado.c_str()
    );

    Serial.println("[TS] Lectura OK");

    Serial.println("------------------------------------");
  }

  // ========================================================
  // Error de lectura
  // ========================================================

  else {

    Serial.printf(
      "[TS] Error de lectura. Codigo: %d\n",
      statusCode
    );
  }
}

// ============================================================
// ROTACIÓN DISPLAY
// ============================================================

void rotarDisplay() {

  if (!datosDisponibles) return;

  screenPage = (screenPage + 1) % 3;

  mostrarPagina(screenPage);
}

// ============================================================
// MOSTRAR PAGINA OLED
// ============================================================

void mostrarPagina(int pagina) {

  display.clearDisplay();

  display.setTextColor(SH110X_WHITE);

  display.setTextSize(1);

  display.setCursor(0, 0);

  switch (pagina) {

    // ======================================================
    // PAGINA 1
    // ======================================================

    case 0:

      display.println("=== Datos (1/3) ===");

      display.setCursor(0, 14);

      display.printf(
        "Aleatorio: %.0f",
        val_aleatorio
      );

      display.setCursor(0, 26);

      display.printf(
        "Temp: %.1f C",
        val_temperatura
      );

      display.setCursor(0, 38);

      display.printf(
        "Hum : %.1f %%",
        val_humedad
      );

      break;

    // ======================================================
    // PAGINA 2
    // ======================================================

    case 1:

      display.println("=== Estado (2/3) ===");

      display.setCursor(0, 14);

      display.printf(
        "Uptime: %.0f s",
        val_tiempo
      );

      display.setCursor(0, 28);

      display.println("Estado:");

      display.setCursor(0, 40);

      display.println(val_estado.c_str());

      break;

    // ======================================================
    // PAGINA 3
    // ======================================================

    case 2:

      display.println("== Resumen (3/3) ==");

      display.setCursor(0, 14);

      display.printf(
        "R:%.0f T:%.1fC",
        val_aleatorio,
        val_temperatura
      );

      display.setCursor(0, 26);

      display.printf(
        "H:%.1f%% t:%.0fs",
        val_humedad,
        val_tiempo
      );

      display.setCursor(0, 40);

      display.printf(
        "St:%s",
        val_estado.c_str()
      );

      break;
  }

  // ========================================================
  // Indicadores de pagina
  // ========================================================

  for (int i = 0; i < 3; i++) {

    if (i == pagina) {

      display.fillRect(
        54 + i * 10,
        58,
        6,
        4,
        SH110X_WHITE
      );

    } else {

      display.drawRect(
        54 + i * 10,
        58,
        6,
        4,
        SH110X_WHITE
      );
    }
  }

  display.display();
}