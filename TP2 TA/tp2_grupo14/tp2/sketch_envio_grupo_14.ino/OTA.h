#include <ESPmDNS.h>
#include <Update.h>

const char* otaUsername = "admin";
const char* otaPassword = "admin";

void iniciaOTA() {
  if (MDNS.begin(host)) {
    Serial.println("mDNS OK – http://esp32.local/");
  }

  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html",
      "<form method='POST' action='/update' enctype='multipart/form-data'>"
      "<h2>OTA Update – ESP32</h2>"
      "<input type='file' name='update'><br><br>"
      "<input type='submit' value='Actualizar'>"
      "</form>");
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
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
          Serial.printf("Actualización OK: %u bytes\n", upload.totalSize);
        } else {
          Update.printError(Serial);
        }
      }
    }
  );

  server.begin();
  Serial.println("Servidor OTA iniciado.");
}