#include <ESPmDNS.h>
#include <Update.h>

const char* otaUsername = "admin";
const char* otaPassword = "admin";

const char* otaHTML = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>OTA Update – ESP32</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: Arial, sans-serif;
      background: #1a1a2e;
      color: #eee;
      display: flex;
      justify-content: center;
      align-items: center;
      min-height: 100vh;
    }
    .card {
      background: #16213e;
      border-radius: 12px;
      padding: 40px;
      width: 100%;
      max-width: 420px;
      box-shadow: 0 8px 32px rgba(0,0,0,0.4);
      text-align: center;
    }
    h2 {
      font-size: 1.5rem;
      margin-bottom: 8px;
      color: #00d4ff;
    }
    p { color: #aaa; margin-bottom: 28px; font-size: 0.9rem; }
    .file-label {
      display: block;
      border: 2px dashed #00d4ff;
      border-radius: 8px;
      padding: 30px 20px;
      cursor: pointer;
      margin-bottom: 20px;
      transition: background 0.2s;
    }
    .file-label:hover { background: #0f3460; }
    .file-label span { display: block; color: #00d4ff; font-size: 0.95rem; margin-top: 8px; }
    input[type='file'] { display: none; }
    #filename { font-size: 0.85rem; color: #aaa; margin-bottom: 20px; min-height: 18px; }
    button {
      background: #00d4ff;
      color: #1a1a2e;
      border: none;
      border-radius: 8px;
      padding: 14px 40px;
      font-size: 1rem;
      font-weight: bold;
      cursor: pointer;
      width: 100%;
      transition: background 0.2s;
    }
    button:hover { background: #00aacf; }
    #progress { display: none; margin-top: 20px; }
    progress { width: 100%; height: 12px; border-radius: 6px; }
    #status { margin-top: 12px; font-size: 0.9rem; color: #00d4ff; }
  </style>
</head>
<body>
  <div class="card">
    <h2>⚡ OTA Update</h2>
    <p>ESP32 – TP2 IoT</p>
    <form method="POST" action="/update" enctype="multipart/form-data" id="uploadForm">
      <label class="file-label" for="fileInput">
        📂
        <span>Seleccionar archivo .bin</span>
      </label>
      <input type="file" id="fileInput" name="update" accept=".bin" onchange="showFile(this)">
      <div id="filename">Ningún archivo seleccionado</div>
      <button type="submit">Actualizar firmware</button>
    </form>
    <div id="progress">
      <progress id="bar" value="0" max="100"></progress>
      <div id="status">Subiendo...</div>
    </div>
  </div>
  <script>
    function showFile(input) {
      document.getElementById('filename').textContent = input.files[0]?.name || '';
    }
    document.getElementById('uploadForm').addEventListener('submit', function(e) {
      e.preventDefault();
      const file = document.getElementById('fileInput').files[0];
      if (!file) { alert('Seleccioná un archivo .bin'); return; }
      const formData = new FormData();
      formData.append('update', file);
      const xhr = new XMLHttpRequest();
      xhr.open('POST', '/update');
      xhr.upload.onprogress = function(e) {
        if (e.lengthComputable) {
          const pct = Math.round(e.loaded / e.total * 100);
          document.getElementById('bar').value = pct;
          document.getElementById('status').textContent = 'Subiendo... ' + pct + '%';
        }
      };
      xhr.onload = function() {
        document.getElementById('status').textContent = xhr.responseText === 'OK'
          ? '✅ Actualización exitosa. Reiniciando...'
          : '❌ Error en la actualización.';
      };
      document.getElementById('progress').style.display = 'block';
      xhr.send(formData);
    });
  </script>
</body>
</html>
)rawliteral";

void iniciaOTA() {
  if (MDNS.begin(host)) {
    Serial.println("mDNS OK – http://esp32.local/");
  }

  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", otaHTML);
  });

  server.on("/update", HTTP_POST,
    []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/plain", Update.hasError() ? "FALLO" : "OK");
      ESP.restart();
    },
    []() {
      HTTPUpload& upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("Actualizando: %s\n", upload.filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) Update.printError(Serial);
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize)
          Update.printError(Serial);
      } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) Serial.printf("OK: %u bytes\n", upload.totalSize);
        else Update.printError(Serial);
      }
    }
  );

  server.begin();
  Serial.println("Servidor OTA iniciado.");
}
