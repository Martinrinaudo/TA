// ================================================================
//  TP2 IoT | SKETCH 1 — Envío de datos a ThingSpeak
//  OTA vía navegador web (subir archivo .bin desde el browser)
//
//  Hardware: ESP32 + DHT22 (pin 33) + Pulsador (pin 19)
//            + OLED SH1106 I2C (SDA=21, SCL=22)
//
//  Flujo:
//    1. Carga inicial por USB
//    2. Envía datos a ThingSpeak cada 16s
//    3. Para pasar al Sketch 2:
//       a. Exportar Sketch 2 compilado: Sketch → Export Compiled Binary
//       b. Abrir navegador en http://<IP_del_ESP32>
//       c. Seleccionar el archivo .bin y subir
//
//  La IP del ESP32 se muestra en el display y en el Serial Monitor
// ================================================================

#include <WiFi.h>
#include <ThingSpeak.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <WebServer.h>
#include <Update.h>

// ----------------------------------------------------------------
//  CONFIGURACIÓN — EDITAR ANTES DE CARGAR
// ----------------------------------------------------------------
const char* SSID      = "Caladux";
const char* PASSWORD  = "carlos123";

const unsigned long CHANNEL_ID = 3370344;
const char*         WRITE_KEY  = "0F6V5EO1Q5K9WTZ6";

// ----------------------------------------------------------------
//  PINES
// ----------------------------------------------------------------
#define DHTPIN   33
#define DHTTYPE  DHT22
#define PIN_PUSH 19
#define I2C_SDA  21
#define I2C_SCL  22
#define I2C_ADDR 0x3C

// ----------------------------------------------------------------
//  OBJETOS
// ----------------------------------------------------------------
DHT              dht(DHTPIN, DHTTYPE);
WiFiClient       tsClient;
WebServer        server(80);
Adafruit_SH1106G display(128, 64, &Wire, -1);

// ----------------------------------------------------------------
//  ESTADOS DEL SISTEMA
// ----------------------------------------------------------------
const char* ESTADOS[]  = {
  "No disponible",
  "En reparacion",
  "Habilitado",
  "Fuera de servicio"
};
const int NUM_ESTADOS = 4;
int indiceEstado      = 0;

// ----------------------------------------------------------------
//  CONTROL DE TIEMPOS
// ----------------------------------------------------------------
const unsigned long INTERVALO_TS      = 16000;
const unsigned long INTERVALO_DISPLAY =   500;

unsigned long tUltimoTS      = 0;
unsigned long tUltimoDisplay = 0;

// Debounce pulsador sin delay()
bool          btnEstado  = HIGH;
unsigned long tUltimoBtn = 0;
const unsigned long DEBOUNCE = 50;

// ----------------------------------------------------------------
//  PÁGINA HTML del servidor OTA
// ----------------------------------------------------------------
const char* HTML_OTA = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>OTA Update - ESP32 Sketch 1</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: sans-serif;
      background: #f5f5f5;
      display: flex;
      justify-content: center;
      align-items: center;
      min-height: 100vh;
      padding: 20px;
    }
    .card {
      background: white;
      border-radius: 12px;
      padding: 40px;
      max-width: 480px;
      width: 100%;
      box-shadow: 0 4px 16px rgba(0,0,0,0.1);
      text-align: center;
    }
    h2 { color: #333; margin-bottom: 8px; font-size: 22px; }
    .sub { color: #888; font-size: 14px; margin-bottom: 32px; }
    .badge {
      display: inline-block;
      background: #e8f5e9;
      color: #2e7d32;
      padding: 4px 12px;
      border-radius: 20px;
      font-size: 13px;
      margin-bottom: 24px;
    }
    label {
      display: block;
      background: #f9f9f9;
      border: 2px dashed #ccc;
      border-radius: 8px;
      padding: 24px;
      cursor: pointer;
      margin-bottom: 20px;
      transition: border-color 0.2s;
    }
    label:hover { border-color: #4CAF50; }
    label p { color: #555; font-size: 14px; margin-top: 8px; }
    input[type="file"] { display: none; }
    #filename { color: #4CAF50; font-weight: bold; margin-top: 8px; font-size: 13px; min-height: 18px; }
    button {
      background: #4CAF50;
      color: white;
      border: none;
      padding: 14px 32px;
      border-radius: 8px;
      font-size: 16px;
      cursor: pointer;
      width: 100%;
      transition: background 0.2s;
    }
    button:hover { background: #388E3C; }
    button:disabled { background: #aaa; cursor: not-allowed; }
    #progress { display: none; margin-top: 20px; }
    #bar-wrap { background: #eee; border-radius: 6px; height: 12px; overflow: hidden; margin: 8px 0; }
    #bar { background: #4CAF50; height: 100%; width: 0%; transition: width 0.3s; border-radius: 6px; }
    #msg { font-size: 14px; color: #555; margin-top: 8px; }
  </style>
</head>
<body>
  <div class="card">
    <h2>Actualizacion OTA</h2>
    <p class="sub">Subir nuevo firmware al ESP32</p>
    <div class="badge">Sketch 1 activo — Enviando datos</div>
    <form id="otaForm">
      <label for="fileInput">
        <svg width="40" height="40" viewBox="0 0 24 24" fill="none" stroke="#4CAF50" stroke-width="1.5">
          <path d="M21 15v4a2 2 0 01-2 2H5a2 2 0 01-2-2v-4"/>
          <polyline points="17 8 12 3 7 8"/>
          <line x1="12" y1="3" x2="12" y2="15"/>
        </svg>
        <p>Seleccionar archivo .bin</p>
        <p id="filename">Ningun archivo seleccionado</p>
        <input type="file" id="fileInput" accept=".bin" onchange="archivoSeleccionado(this)">
      </label>
      <button type="button" id="btnSubir" onclick="subirFirmware()" disabled>
        Subir firmware
      </button>
    </form>
    <div id="progress">
      <div id="bar-wrap"><div id="bar"></div></div>
      <p id="msg">Subiendo...</p>
    </div>
  </div>
  <script>
    function archivoSeleccionado(input) {
      var name = input.files[0] ? input.files[0].name : 'Ningun archivo seleccionado';
      document.getElementById('filename').textContent = name;
      document.getElementById('btnSubir').disabled = !input.files[0];
    }
    function subirFirmware() {
      var file = document.getElementById('fileInput').files[0];
      if (!file) return;
      var xhr = new XMLHttpRequest();
      xhr.open('POST', '/update', true);
      xhr.upload.onprogress = function(e) {
        if (e.lengthComputable) {
          var pct = Math.round((e.loaded / e.total) * 100);
          document.getElementById('bar').style.width = pct + '%';
          document.getElementById('msg').textContent = 'Subiendo: ' + pct + '%';
        }
      };
      xhr.onload = function() {
        document.getElementById('msg').textContent =
          xhr.status === 200
            ? 'Actualizacion exitosa. El ESP32 se reinicia...'
            : 'Error en la actualizacion. Intentar de nuevo.';
        document.getElementById('bar').style.background =
          xhr.status === 200 ? '#4CAF50' : '#f44336';
      };
      xhr.onerror = function() {
        document.getElementById('msg').textContent = 'Error de conexion con el ESP32.';
        document.getElementById('bar').style.background = '#f44336';
      };
      document.getElementById('btnSubir').disabled = true;
      document.getElementById('progress').style.display = 'block';
      var form = new FormData();
      form.append('update', file);
      xhr.send(form);
    }
  </script>
</body>
</html>
)rawliteral";

// ================================================================
//  FUNCIONES DEL SERVIDOR OTA WEB
// ================================================================
void iniciarServidorOTA() {

  // Ruta raiz: muestra el formulario HTML
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", HTML_OTA);
  });

  // Ruta /update: recibe el .bin y flashea el ESP32
  server.on("/update", HTTP_POST,
    // Callback al TERMINAR la subida completa
    []() {
      bool exito = !Update.hasError();
      server.send(200, "text/plain", exito ? "OK" : "ERROR");

      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SH110X_WHITE);
      display.setCursor(0, 15);

      if (exito) {
        display.println("  OTA Web: OK!");
        display.setCursor(0, 30); display.println("  Reiniciando...");
        Serial.println("OTA Web: Exito. Reiniciando...");
      } else {
        display.println("  OTA Web: ERROR");
        Serial.println("OTA Web: ERROR al flashear.");
      }
      display.display();
      delay(2000);
      ESP.restart();
    },

    // Callback por cada chunk del binario que llega
    []() {
      HTTPUpload& upload = server.upload();

      if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("OTA Web: Recibiendo '%s' (%u bytes)\n",
                      upload.filename.c_str(), upload.totalSize);
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SH110X_WHITE);
        display.setCursor(0, 15); display.println("  Recibiendo OTA...");
        display.setCursor(0, 30); display.println("  No desconectar!");
        display.display();
        // UPDATE_SIZE_UNKNOWN: usa todo el espacio de flash disponible
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
          Update.printError(Serial);
        }
      }
      else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
      }
      else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
          Serial.printf("OTA Web: %u bytes escritos correctamente.\n", upload.totalSize);
        } else {
          Update.printError(Serial);
        }
      }
    }
  );

  server.begin();
  Serial.println("Servidor OTA Web iniciado.");
  Serial.print("Abrir en el navegador: http://");
  Serial.println(WiFi.localIP());
}

// ================================================================
//  FUNCIÓN: Actualizar OLED
// ================================================================
void mostrarDisplay(unsigned long ahora) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);

  display.setCursor(0, 0);
  display.println("SISTEMA DE ENVIO");
  display.drawLine(0, 10, 127, 10, SH110X_WHITE);

  display.setCursor(0, 20);
  display.print("Estado:");
  display.setCursor(0, 32);
  display.println(ESTADOS[indiceEstado]);

  display.setCursor(0, 50);
  display.printf("Up: %lu s", ahora / 1000);

  display.display();
}

// ================================================================
//  FUNCIÓN: Conexión WiFi con feedback en display
// ================================================================
void conectarWiFi() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0, 0);
  display.println("Conectando WiFi...");
  display.display();

  WiFi.mode(WIFI_STA);
  WiFi.begin(SSID, PASSWORD);
  Serial.print("Conectando");

  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 20) {
    delay(500);
    Serial.print(".");
    intentos++;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);  display.println("Conectando WiFi...");
    display.setCursor(0, 20); display.printf("Intento: %d/20", intentos);
    display.display();
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi OK! IP: " + WiFi.localIP().toString());
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);  display.println("WiFi OK!");
    display.setCursor(0, 15); display.println(WiFi.localIP().toString());
    display.setCursor(0, 30); display.println("OTA: abrir IP");
    display.setCursor(0, 45); display.println("en el navegador");
    display.display();
    delay(3000);
  } else {
    Serial.printf("\nERROR WiFi. Status: %d\n", WiFi.status());
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SH110X_WHITE);
    display.setCursor(0, 0);  display.println("ERROR WiFi");
    display.setCursor(0, 15); display.println("Verificar red");
    display.display();
    delay(3000);
  }
}

// ================================================================
//  SETUP — corre una sola vez al arrancar
// ================================================================
void setup() {
  Serial.begin(115200);
  pinMode(PIN_PUSH, INPUT_PULLUP);
  dht.begin();
  Wire.begin(I2C_SDA, I2C_SCL);

  if (!display.begin(I2C_ADDR, true)) {
    Serial.println("ERROR CRITICO: Display no responde. Verificar cableado.");
  }
  display.clearDisplay();
  display.display();

  conectarWiFi();

  if (WiFi.status() == WL_CONNECTED) {
    iniciarServidorOTA();
  }

  ThingSpeak.begin(tsClient);

  // Primera ejecución inmediata (no espera 16 segundos)
  tUltimoTS      = millis() - INTERVALO_TS;
  tUltimoDisplay = millis();

  Serial.println("=== SKETCH 1 - WEB OTA - LISTO ===");
  Serial.print("Pagina OTA: http://");
  Serial.println(WiFi.localIP());
  mostrarDisplay(millis());
}

// ================================================================
//  LOOP — corre indefinidamente
// ================================================================
void loop() {
  server.handleClient(); // Atiende peticiones del navegador (OTA)
  unsigned long ahora = millis();

  // --- DISPLAY: refresco cada 500ms, siempre, incluso sin WiFi ---
  if (ahora - tUltimoDisplay >= INTERVALO_DISPLAY) {
    mostrarDisplay(ahora);
    tUltimoDisplay = ahora;
  }

  // --- PULSADOR: debounce sin delay() ---
  bool btnActual = digitalRead(PIN_PUSH);
  if (btnActual == LOW && btnEstado == HIGH && (ahora - tUltimoBtn > DEBOUNCE)) {
    tUltimoBtn   = ahora;
    indiceEstado = (indiceEstado + 1) % NUM_ESTADOS;
    Serial.printf("Estado cambiado: %s\n", ESTADOS[indiceEstado]);
  }
  btnEstado = btnActual;

  // --- RECONEXIÓN WiFi no bloqueante ---
  if (WiFi.status() != WL_CONNECTED) {
    static unsigned long tUltimoRecon = 0;
    if (ahora - tUltimoRecon >= 8000) {
      Serial.println("WiFi caido. Reintentando...");
      WiFi.disconnect();
      delay(100);
      WiFi.begin(SSID, PASSWORD);
      tUltimoRecon = ahora;
    }
    return;
  }

  // --- ENVÍO A THINGSPEAK cada 16s ---
  if (ahora - tUltimoTS >= INTERVALO_TS) {
    tUltimoTS = ahora;

    float t = dht.readTemperature();
    float h = dht.readHumidity();

    if (isnan(t) || isnan(h)) {
      Serial.println("ADVERTENCIA: DHT22 no responde. Enviando ceros.");
      t = 0.0;
      h = 0.0;
    }

    long aleatorio = random(100, 501);
    long uptime    = ahora / 1000;

    ThingSpeak.setField(1, (float)aleatorio);
    ThingSpeak.setField(2, t);
    ThingSpeak.setField(3, h);
    ThingSpeak.setField(4, uptime);
    ThingSpeak.setStatus(ESTADOS[indiceEstado]);

    Serial.println("\n--- ENVIANDO A THINGSPEAK ---");
    Serial.printf("  Campo 1 - Aleatorio: %ld\n",     aleatorio);
    Serial.printf("  Campo 2 - Temp:      %.2f C\n",  t);
    Serial.printf("  Campo 3 - Hum:       %.2f %%\n", h);
    Serial.printf("  Campo 4 - Uptime:    %ld s\n",   uptime);
    Serial.printf("  Status  - Estado:    %s\n",      ESTADOS[indiceEstado]);

    int resp = ThingSpeak.writeFields(CHANNEL_ID, WRITE_KEY);

    if (resp == 200) Serial.println("  >> EXITO: Datos grabados correctamente.");
    else             Serial.printf("  >> ERROR: Codigo HTTP %d\n", resp);

    Serial.println("-----------------------------");
  }
}
