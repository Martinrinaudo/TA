// ============================================================
//  TP1 IoT - 4K4 2026
//  ESP32 DevKit V1 — Placa de la cátedra
//
//  Librerías requeridas:
//    - Adafruit SH110X   (display SH1106G — NO SSD1306)
//    - Adafruit GFX
//    - DHT sensor library (Adafruit)
//
//  PINOUT (config.h de la cátedra):
//    GPIO 33  → DHT22
//    GPIO 32  → Potenciómetro (ADC1_CH4)
//    GPIO 19  → Pulsador modo (PIN_ENC_PUSH, INPUT_PULLUP)
//    GPIO 23  → LED externo rojo (PIN_LED_R)
//    GPIO  2  → LED integrado PWM (LED_BUILTIN)
//    GPIO 12  → Touch capacitivo PIN+ (T5)
//    GPIO 14  → Touch capacitivo PIN- (T6)
//    GPIO 21  → SDA display OLED
//    GPIO 22  → SCL display OLED
// ============================================================

#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// ------------------------------------------------------------
// PINOUT
// ------------------------------------------------------------
const int DHT_PIN      = GPIO_NUM_33;
const int PIN_ADC      = GPIO_NUM_32;
const int PIN_ENC_PUSH = GPIO_NUM_19;
const int PIN_LED_R    = GPIO_NUM_23;
const int PIN_LED_PWM  = 2;
const int touchMas     = 12;   // T5
const int touchMenos   = 14;   // T6

// ------------------------------------------------------------
// DISPLAY — SH1106G (igual que el profe)
// ------------------------------------------------------------
const int SCREEN_WIDTH  = 128;
const int SCREEN_HEIGHT = 64;
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

// ------------------------------------------------------------
// DHT22
// ------------------------------------------------------------
DHT dht(DHT_PIN, DHT22);

// ------------------------------------------------------------
// TOUCH — igual que ejemplo del profe
// ------------------------------------------------------------
volatile bool touchDetectado1 = false;
volatile bool touchDetectado2 = false;

void IRAM_ATTR handleTouch1() {
  touchDetectado1 = true;
}
void IRAM_ATTR handleTouch2() {
  touchDetectado2 = true;
}

// ------------------------------------------------------------
// ESTADO GLOBAL
// ------------------------------------------------------------
int  modo          = 1;
int  intensidadMax = 100;
int  intensidadMin =   0;

// Pulsador
bool          btnAnterior     = HIGH;
unsigned long btnUltimoTiempo = 0;
const int     DEBOUNCE_MS     = 50;

// Touch — debounce por delay igual que el profe (delay 100 en loop)
const int TOUCH_PASO = 5;   // % por toque

// DHT
float         temperatura      = 0.0;
float         humedad          = 0.0;
unsigned long dhtUltimaLectura = 0;
bool          dhtOK            = false;
const int     DHT_INTERVALO    = 2000;

// Pot / brillo — 8 bits igual que el profe
int valorPot      = 0;   // 0..255
int brightnessPct = 0;   // 0..100%

// ------------------------------------------------------------
// DISPLAY init
// ------------------------------------------------------------
void displayInit() {
  display.begin(0x3C, true);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println(F("   TP1 IoT - 4K4"));
  display.println(F("       2026"));
  display.setCursor(0, 30);
  display.println(F(" Presiona el boton"));
  display.println(F("  para cambiar modo"));
  display.display();
}

void dibujarEncabezado(const char* titulo) {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(titulo);
  display.drawLine(0, 10, SCREEN_WIDTH - 1, 10, SH110X_WHITE);
}

// ============================================================
// MODO 1
// ============================================================
void mostrarModo1() {
  dibujarEncabezado("MODO 1  LED+POT");
  display.setTextSize(2);
  display.setCursor(0, 14);
  display.printf("Bri:%d%%\n", brightnessPct);
  display.setTextSize(1);
  display.setCursor(0, 36);
  display.printf("Pot raw: %d\n", valorPot);
  display.setCursor(0, 48);
  display.printf("Max:%d%%  Min:%d%%", intensidadMax, intensidadMin);
  display.display();
}

// ============================================================
// MODO 2
// ============================================================
void mostrarModo2() {
  unsigned long ahora = millis();
  if (ahora - dhtUltimaLectura >= DHT_INTERVALO) {
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      temperatura = t;
      humedad     = h;
      dhtOK       = true;
    }
    dhtUltimaLectura = ahora;
  }

  dibujarEncabezado("MODO 2  AMBIENTAL");
  display.setTextSize(1);
  display.setCursor(0, 14);
  if (dhtOK) {
    display.printf("Temp  : %.1fC\n", temperatura);
    display.setCursor(0, 26);
    display.printf("Hum   : %.1f%%\n", humedad);
  } else {
    display.println("Temp  : leyendo...");
    display.setCursor(0, 26);
    display.println("Hum   : leyendo...");
  }
  display.setCursor(0, 44);
  display.printf("Uptime: %lu seg", millis() / 1000);
  display.display();
}

// ============================================================
// MODOS 3 y 4 — Touch igual que ejemplo del profe
// ============================================================
void mostrarModoConfig(bool esMaximo) {

  // Limpiar flags acumuladas de otros modos antes de procesar
  // (se procesan una sola vez por entrada al modo, igual que profe)
  if (touchDetectado1) {
    touchDetectado1 = false;
    if (esMaximo) {
      intensidadMax = min(100, intensidadMax + TOUCH_PASO);
    } else {
      intensidadMin = min(intensidadMax - TOUCH_PASO, intensidadMin + TOUCH_PASO);
      if (intensidadMin < 0) intensidadMin = 0;
    }
    Serial.printf("Touch+  M=%d  m=%d\n", intensidadMax, intensidadMin);
  } else if (touchDetectado2) {   // else if igual que el profe — uno por ciclo
    touchDetectado2 = false;
    if (esMaximo) {
      intensidadMax = max(intensidadMin + TOUCH_PASO, intensidadMax - TOUCH_PASO);
    } else {
      intensidadMin = max(0, intensidadMin - TOUCH_PASO);
    }
    Serial.printf("Touch-  M=%d  m=%d\n", intensidadMax, intensidadMin);
  }

  int         valor  = esMaximo ? intensidadMax : intensidadMin;
  const char* titulo = esMaximo ? "MODO 3  MAX (M)" : "MODO 4  MIN (m)";
  const char* etiq   = esMaximo ? "M = "            : "m = ";

  dibujarEncabezado(titulo);
  display.setTextSize(3);
  display.setCursor(10, 18);
  display.printf("%s%d%%", etiq, valor);
  display.setTextSize(1);
  display.setCursor(0, 56);
  display.print("+:GPIO12   -:GPIO14");
  display.display();
}

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("\n=== TP1 IoT 4K4 2026 ==="));

  pinMode(PIN_LED_R, OUTPUT);
  digitalWrite(PIN_LED_R, LOW);
  pinMode(PIN_ENC_PUSH, INPUT_PULLUP);

  // PWM 8 bits — igual que el profe
  analogReadResolution(8);
  analogWriteResolution(PIN_LED_PWM, 8);
  analogWrite(PIN_LED_PWM, 0);

  Wire.begin(21, 22);
  Serial.println(F("Iniciando Display..."));
  displayInit();
  Serial.println(F("Display iniciado."));
  delay(2000);

  dht.begin();
  Serial.println(F("DHT22 iniciado."));

  // Touch — umbral 40, igual que el profe
  touchAttachInterrupt(touchMas,   handleTouch1, 40);
  touchAttachInterrupt(touchMenos, handleTouch2, 40);
  Serial.println(F("Touch activos. GPIO12=+ GPIO14=-"));
}

// ============================================================
// LOOP
// ============================================================
void loop() {
  unsigned long ahora = millis();

  // 1. Potenciómetro → brillo LED integrado
  valorPot      = analogRead(PIN_ADC);        // 0..255
  brightnessPct = (int)(valorPot / 2.55f);    // 0..100%
  analogWrite(PIN_LED_PWM, valorPot);

  // 2. Alarma LED externo — todos los modos
  bool alarma = (brightnessPct > intensidadMax) || (brightnessPct < intensidadMin);
  digitalWrite(PIN_LED_R, alarma ? HIGH : LOW);

  // 3. Pulsador modo con debounce
  bool btnActual = digitalRead(PIN_ENC_PUSH);
  if (btnActual == LOW && btnAnterior == HIGH &&
      (ahora - btnUltimoTiempo > DEBOUNCE_MS)) {
    btnUltimoTiempo = ahora;
    // Al cambiar de modo limpiamos flags touch para no procesar
    // toques acumulados del modo anterior
    touchDetectado1 = false;
    touchDetectado2 = false;
    modo = (modo % 4) + 1;
    Serial.printf("[MODO] %d\n", modo);
  }
  btnAnterior = btnActual;

  // 4. Display
  switch (modo) {
    case 1: mostrarModo1();           break;
    case 2: mostrarModo2();           break;
    case 3: mostrarModoConfig(true);  break;
    case 4: mostrarModoConfig(false); break;
  }

  delay(100);  // igual que el profe en el ejemplo touch
}
