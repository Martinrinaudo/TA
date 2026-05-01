#include <WiFi.h>
#include <ThingSpeak.h>
#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <WebServer.h>

// --- CONFIGURACIÓN WIFI Y THINGSPEAK ---
const char* ssid = "Francisco's Iphone"; // Cambiar por tu WiFi si usas hardware real
const char* password = "";        // Cambiar por tu password si usas hardware real
unsigned long myChannelNumber = 3365192; // TU CHANNEL ID
const char* myWriteAPIKey = "DYFY685KO34CWYFS"; // TU WRITE API KEY
WiFiClient client;

// --- CONFIGURACIÓN DE PINES Y SENSORES ---
#define DHTPIN 33
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
#define PIN_PUSH 0 // Cambia al pin que uses para el pulsador
#define I2C_ADDRESS 0x3c
Adafruit_SH1106G display(128, 64, &Wire, -1);

// --- VARIABLES DE ESTADO Y CONTROL ---
const char* estados[] = {"No disponible", "En reparacion", "Habilitado", "Fuera de servicio"};
int indiceEstado = 0;
unsigned long ultimaVezThingSpeak = 0;
const unsigned long intervaloThingSpeak = 16000; // 16 segundos
// Variables para el pulsador
bool btnAnt = HIGH;
void setup() {
  Serial.begin(115200);
  
  // Inicialización de sensores y display
  dht.begin();
  if (!display.begin(I2C_ADDRESS, true)) {
    Serial.println("Error al iniciar OLED");
    while (1);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  // Conexión WiFi
  Serial.print("Conectando a WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  ThingSpeak.begin(client);
  pinMode(PIN_PUSH, INPUT_PULLUP);
}
void loop() {
  unsigned long ahora = millis();
  // 1. LÓGICA DEL PULSADOR (Rotación de estados)
  bool btnAct = digitalRead(PIN_PUSH);
  if (btnAct == LOW && btnAnt == HIGH) {
    delay(50); // Debounce
    indiceEstado = (indiceEstado + 1) % 4;
    Serial.print("Estado cambiado a: ");
    Serial.println(estados[indiceEstado]);
  }
  btnAnt = btnAct;
  // 2. ACTUALIZACIÓN DEL DISPLAY
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("SISTEMA DE MONITOREO");
  display.drawLine(0, 10, 127, 10, SH110X_WHITE);
  display.setCursor(0, 25);
  display.print("Estado: ");
  display.setCursor(0, 40);
  display.setTextSize(1);
  display.println(estados[indiceEstado]);
  display.display();
  // 3. ENVÍO DE DATOS A THINGSPEAK (Cada 16 segundos)
  if (ahora - ultimaVezThingSpeak >= intervaloThingSpeak) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    long valorAleatorio = random(100, 501);
    unsigned long segundosUptime = ahora / 1000;
    // Verificar si las lecturas son válidas
    if (isnan(t) || isnan(h)) {
      Serial.println("Error al leer el sensor DHT!");
      t = 0.0; h = 0.0;
    }
    // Seteamos los campos (Fields)
    ThingSpeak.setField(1, (float)valorAleatorio);
    ThingSpeak.setField(2, t);
    ThingSpeak.setField(3, h);
    ThingSpeak.setField(4, (long)segundosUptime);
    ThingSpeak.setStatus(estados[indiceEstado]);
    // Envío oficial
    int respuesta = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    // Validación por Serial
    Serial.println("\n--- Enviando a ThingSpeak ---");
    Serial.printf("Aleatorio: %ld\n", valorAleatorio);
    Serial.printf("Temp: %.2f C\n", t);
    Serial.printf("Hum: %.2f %%\n", h);
    Serial.printf("Uptime: %lu seg\n", segundosUptime);
    Serial.printf("Estado: %s\n", estados[indiceEstado]);
    if (respuesta == 200) {
      Serial.println(">> Datos grabados correctamente en el canal.");
    } else {
      Serial.print(">> ERROR al grabar. Codigo: ");
      Serial.println(respuesta);
    }
    ultimaVezThingSpeak = ahora;
  }
}
