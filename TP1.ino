// ============================================================
//  TP1 IoT - 4K4 2026
//  ESP32 DevKit V1
//
//  Modos (ciclados por pulsador):
//    1 - Brillo LED + valor potenciómetro
//    2 - Temperatura, humedad y uptime
//    3 - Configurar intensidad máxima (M)
//    4 - Configurar intensidad mínima (m)
// ============================================================

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

// ------------------------------------------------------------
// Descomentar para simulación en Wokwi
// (reemplaza touchRead por digitalRead con botones)
// ------------------------------------------------------------
//#define SIMULACION_WOKWI 

// ------------------------------------------------------------
// PINES
// ------------------------------------------------------------
#define PIN_LED_BUILTIN    2    // LED integrado (PWM)
#define PIN_LED_EXTERNO    23   // LED rojo externo (alarma)
#define PIN_POT            32   // Potenciómetro (ADC1)
#define PIN_DHT            33   // Sensor DHT22
#define PIN_PULSADOR       19   // Botón modo (INPUT_PULLUP, activo LOW)
#define PIN_TOUCH_PLUS     12   // Touch capacitivo / botón sim. (aumenta)
#define PIN_TOUCH_MINUS    14   // Touch capacitivo / botón sim. (disminuye)

// ------------------------------------------------------------
// OLED
// ------------------------------------------------------------
#define SCREEN_WIDTH       128
#define SCREEN_HEIGHT      64
#define OLED_RESET         -1
#define OLED_ADDRESS       0x3C

// ------------------------------------------------------------
// PWM
// ------------------------------------------------------------
#define PWM_CANAL          0
#define PWM_FRECUENCIA     5000
#define PWM_RESOLUCION     12      // 12 bits → 0..4095
#define PWM_MAX_VAL        4095

// ------------------------------------------------------------
// TIMING Y UMBRALES
// ------------------------------------------------------------
#define DEBOUNCE_BTN_MS    50
#define DEBOUNCE_TOUCH_MS  300
#define DHT_INTERVALO_MS   2000
#define TOUCH_UMBRAL       30     // < umbral = tocado (hardware real)
#define TOUCH_PASO         5      // % por cada toque

// ------------------------------------------------------------
// OBJETOS
// ------------------------------------------------------------
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHT dht(PIN_DHT, DHT22);

// ------------------------------------------------------------
// ESTADO GLOBAL
// ------------------------------------------------------------
int  modo          = 1;
int  intensidadMax = 100;   // M (%)
int  intensidadMin = 0;     // m (%)

// Pulsador
bool           btnAnterior       = HIGH;
unsigned long  btnUltimoTiempo   = 0;

// Touch
unsigned long  touchPlusUltimo   = 0;
unsigned long  touchMinusUltimo  = 0;

// DHT22 (lectura no bloqueante)
float          temperatura       = 0.0;
float          humedad           = 0.0;
unsigned long  dhtUltimaLectura  = 0;

// Pot / brillo (compartido entre loop y modos)
int            valorPot          = 0;
int            brightnessPct     = 0;

// ============================================================
// HELPERS
// ============================================================

bool leerTouchPlus() {
#ifdef SIMULACION_WOKWI
  return digitalRead(PIN_TOUCH_PLUS) == LOW;
#else
  return touchRead(PIN_TOUCH_PLUS) < TOUCH_UMBRAL;
#endif
}

bool leerTouchMinus() {
#ifdef SIMULACION_WOKWI
  return digitalRead(PIN_TOUCH_MINUS) == LOW;
#else
  return touchRead(PIN_TOUCH_MINUS) < TOUCH_UMBRAL;
#endif
}

void dibujarEncabezado(const char* titulo) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(titulo);
  display.drawLine(0, 10, SCREEN_WIDTH - 1, 10, SSD1306_WHITE);
}

// ============================================================
// MODO 1 — Brillo LED + Potenciómetro
// ============================================================
void mostrarModo1() {
  dibujarEncabezado("MODO 1  LED + POT");

  display.setTextSize(2);
  display.setCursor(0, 15);
  display.print("Bri: ");
  display.print(brightnessPct);
  display.println("%");

  display.setTextSize(1);
  display.setCursor(0, 38);
  display.print("Pot raw : ");
  display.println(valorPot);

  display.setCursor(0, 50);
  display.print("Max:");
  display.print(intensidadMax);
  display.print("%  Min:");
  display.print(intensidadMin);
  display.println("%");

  display.display();
}

// ============================================================
// MODO 2 — Variables ambientales + uptime
// ============================================================
void mostrarModo2() {
  unsigned long ahora = millis();
  if (ahora - dhtUltimaLectura >= DHT_INTERVALO_MS) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t)) temperatura = t;
    if (!isnan(h)) humedad    = h;
    dhtUltimaLectura = ahora;
  }

  unsigned long uptime = millis() / 1000;

  dibujarEncabezado("MODO 2  AMBIENTAL");

  display.setTextSize(1);
  display.setCursor(0, 14);
  display.print("Temp  : ");
  display.print(temperatura, 1);
  display.println(" C");

  display.setCursor(0, 26);
  display.print("Hum   : ");
  display.print(humedad, 1);
  display.println(" %");

  display.setCursor(0, 42);
  display.print("Uptime: ");
  display.print(uptime);
  display.println(" seg");

  display.display();
}

// ============================================================
// MODOS 3 y 4 — Configuración con touch
// ============================================================
void mostrarModoConfig(bool esMaximo) {
  unsigned long ahora = millis();

  // Touch PLUS
  if (leerTouchPlus() && (ahora - touchPlusUltimo > DEBOUNCE_TOUCH_MS)) {
    touchPlusUltimo = ahora;
    if (esMaximo) {
      intensidadMax = min(100, intensidadMax + TOUCH_PASO);
    } else {
      // m no puede superar M - TOUCH_PASO
      intensidadMin = min(intensidadMax - TOUCH_PASO, intensidadMin + TOUCH_PASO);
    }
  }

  // Touch MINUS
  if (leerTouchMinus() && (ahora - touchMinusUltimo > DEBOUNCE_TOUCH_MS)) {
    touchMinusUltimo = ahora;
    if (esMaximo) {
      // M no puede bajar de m + TOUCH_PASO
      intensidadMax = max(intensidadMin + TOUCH_PASO, intensidadMax - TOUCH_PASO);
    } else {
      intensidadMin = max(0, intensidadMin - TOUCH_PASO);
    }
  }

  int valor        = esMaximo ? intensidadMax : intensidadMin;
  const char* titulo = esMaximo ? "MODO 3  MAX (M)" : "MODO 4  MIN (m)";
  const char* etiq   = esMaximo ? "M max = " : "m min = ";

  dibujarEncabezado(titulo);

  display.setTextSize(3);
  display.setCursor(10, 18);
  display.print(etiq);

  display.setTextSize(3);
  display.setCursor(28, 38);
  display.print(valor);
  display.println("%");

  display.setTextSize(1);
  display.setCursor(0, 57);
  display.print("+:GPIO12  -:GPIO14");

  display.display();
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== TP1 IoT 4K4 2026 ===");

  // Pines digitales
  pinMode(PIN_LED_EXTERNO, OUTPUT);
  digitalWrite(PIN_LED_EXTERNO, LOW);
  pinMode(PIN_PULSADOR, INPUT_PULLUP);

#ifdef SIMULACION_WOKWI
  pinMode(PIN_TOUCH_PLUS,  INPUT_PULLUP);
  pinMode(PIN_TOUCH_MINUS, INPUT_PULLUP);
#endif

  // PWM 12 bits — LED integrado
  // Código nuevo (compatible con ESP32 Core 3.0+)
ledcAttach(PIN_LED_BUILTIN, PWM_FRECUENCIA, PWM_RESOLUCION);
ledcWrite(PIN_LED_BUILTIN, 0);

  // OLED
  Wire.begin();
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("[ERROR] OLED no encontrado");
    while (true) { delay(500); }
  }

  // DHT22
  dht.begin();

  // Pantalla de bienvenida
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 10);
  display.println("   TP1 IoT - 4K4");
  display.println("      2026");
  display.setCursor(0, 38);
  display.println(" Presiona el boton");
  display.println("  para cambiar modo");
  display.display();
  delay(2000);
}

// ============================================================
// LOOP
// ============================================================
void loop() {
  unsigned long ahora = millis();

  // --- 1. Leer potenciómetro y actualizar PWM (siempre activo) ---
  valorPot      = analogRead(PIN_POT);
  brightnessPct = (int)((valorPot / (float)PWM_MAX_VAL) * 100.0);
  ledcWrite(PIN_LED_BUILTIN, valorPot);

  // --- 2. Alarma LED externo (siempre activa) ---
  bool alarma = (brightnessPct > intensidadMax) || (brightnessPct < intensidadMin);
  digitalWrite(PIN_LED_EXTERNO, alarma ? HIGH : LOW);

  // --- 3. Leer pulsador con debounce ---
  bool btnActual = digitalRead(PIN_PULSADOR);
  if (btnActual == LOW && btnAnterior == HIGH &&
      (ahora - btnUltimoTiempo > DEBOUNCE_BTN_MS)) {
    btnUltimoTiempo = ahora;
    modo = (modo % 4) + 1;
    Serial.print("Modo: ");
    Serial.println(modo);
  }
  btnAnterior = btnActual;

  // --- 4. Mostrar pantalla según modo ---
  switch (modo) {
    case 1: mostrarModo1();              break;
    case 2: mostrarModo2();              break;
    case 3: mostrarModoConfig(true);     break;
    case 4: mostrarModoConfig(false);    break;
  }

  delay(50);  // ~20 fps, evita parpadeo en OLED
}