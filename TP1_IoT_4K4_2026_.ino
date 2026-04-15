#include <DHT.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

// ------------------------------------------------------------
// PINOUT (Cátedra + Tus cambios)
// ------------------------------------------------------------
const int DHT_PIN      = 33;
const int PIN_ADC      = 32;
const int PIN_ENC_PUSH = 19;
const int PIN_LED_R    = 23;
const int PIN_LED_PWM  = 2;
const int touchMas     = 27; // T7
const int touchMenos   = 14; // T6

// ------------------------------------------------------------
// DISPLAY SH1106G (1.3" OLED)
// ------------------------------------------------------------
#define i2c_Address 0x3C 
Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, &Wire, -1);
DHT dht(DHT_PIN, DHT22);

// ------------------------------------------------------------
// VARIABLES GLOBALES
// ------------------------------------------------------------
int modo = 1;
int intensidadMax = 100; 
int intensidadMin = 0;   
const int TOUCH_PASO = 5;

// Calibración adaptativa
int reposoMas = 0;
int reposoMenos = 0;
const float TOLERANCIA = 0.85; 

int valorPot = 0;
int brightnessPct = 0;
float temperatura = 0, humedad = 0;
unsigned long dhtUltimaLectura = 0;

// ------------------------------------------------------------
// FUNCIONES DE SOPORTE
// ------------------------------------------------------------

void dibujarEncabezado(const char* titulo) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println(titulo);
  display.drawLine(0, 10, 127, 10, SH110X_WHITE);
}

void mostrarModo1() {
  dibujarEncabezado("MODO 1: LED & POT");
  display.setTextSize(2);
  display.setCursor(0, 15);
  display.printf("Bri: %d%%", brightnessPct);
  display.setTextSize(1);
  display.setCursor(0, 40);
  display.printf("Pot (12bit): %d", valorPot);
  display.setCursor(0, 52);
  display.printf("M: %d%% | m: %d%%", intensidadMax, intensidadMin);
  display.display();
}

void mostrarModo2() {
  unsigned long ahora = millis();
  if (ahora - dhtUltimaLectura >= 2000) {
    temperatura = dht.readTemperature();
    humedad = dht.readHumidity();
    dhtUltimaLectura = ahora;
  }
  dibujarEncabezado("MODO 2: AMBIENTAL");
  display.setTextSize(1);
  display.setCursor(0, 15);
  display.printf("Temp: %.1f C", temperatura);
  display.setCursor(0, 30);
  display.printf("Hum:  %.1f %%", humedad);
  display.setCursor(0, 50);
  display.printf("Uptime: %lu seg", ahora / 1000);
  display.display();
}

void mostrarModoConfig(bool esMaximo) {
  int rMas = touchRead(touchMas);
  int rMenos = touchRead(touchMenos);

  // Detección basada en el porcentaje de caída del valor de reposo
  if (rMas < (reposoMas * TOLERANCIA)) {
    if (esMaximo) {
      if (intensidadMax + TOUCH_PASO <= 100) intensidadMax += TOUCH_PASO;
    } else {
      if (intensidadMin + TOUCH_PASO < intensidadMax) intensidadMin += TOUCH_PASO;
    }
  }
  
  if (rMenos < (reposoMenos * TOLERANCIA)) {
    if (esMaximo) {
      if (intensidadMax - TOUCH_PASO > intensidadMin) intensidadMax -= TOUCH_PASO;
    } else {
      if (intensidadMin - TOUCH_PASO >= 0) intensidadMin -= TOUCH_PASO;
    }
  }

  dibujarEncabezado(esMaximo ? "MODO 3: CONF MAX (M)" : "MODO 4: CONF MIN (m)");
  
  display.setTextSize(2);
  display.setCursor(25, 20);
  display.printf("%s %d%%", esMaximo ? "M:" : "m:", esMaximo ? intensidadMax : intensidadMin);
  
  display.setTextSize(1);
  display.setCursor(0, 45);
  display.printf("Carga T+(27): %d", rMas);
  display.setCursor(0, 55);
  display.printf("Carga T-(14): %d", rMenos);
  
  display.display();
}

// ------------------------------------------------------------
// SETUP
// ------------------------------------------------------------
void setup() {
  Serial.begin(115200);
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_ENC_PUSH, INPUT_PULLUP);

  // 12 BITS para cumplir con el enunciado
  analogReadResolution(12); 
  analogWriteResolution(PIN_LED_PWM, 12);

  Wire.begin(21, 22);
  if(!display.begin(i2c_Address, true)) {
    Serial.println("Error display");
    for(;;);
  }
  
  display.clearDisplay();
  display.display(); 
  dht.begin();

  // Calibración inicial automática
  long sumaM = 0, suma_m = 0;
  for(int i=0; i<15; i++) {
    sumaM += touchRead(touchMas);
    suma_m += touchRead(touchMenos);
    delay(20);
  }
  reposoMas = sumaM / 15;
  reposoMenos = suma_m / 15;
}

// ------------------------------------------------------------
// LOOP
// ------------------------------------------------------------
void loop() {
  // 1. Lectura y PWM
  valorPot = analogRead(PIN_ADC); 
  brightnessPct = map(valorPot, 0, 4095, 0, 100);
  analogWrite(PIN_LED_PWM, valorPot);

  // 2. Alarma crítica (LED Externo)
  if (brightnessPct > intensidadMax || brightnessPct < intensidadMin) {
    digitalWrite(PIN_LED_R, HIGH);
  } else {
    digitalWrite(PIN_LED_R, LOW);
  }

  // 3. Pulsador Modo
  static bool btnAnt = HIGH;
  bool btnAct = digitalRead(PIN_ENC_PUSH);
  if (btnAct == LOW && btnAnt == HIGH) {
    delay(50); 
    modo = (modo % 4) + 1;
  }
  btnAnt = btnAct;

  // 4. Pantalla
  switch (modo) {
    case 1: mostrarModo1(); break;
    case 2: mostrarModo2(); break;
    case 3: mostrarModoConfig(true); break;
    case 4: mostrarModoConfig(false); break;
  }
  
  delay(150); 
}