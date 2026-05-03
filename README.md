Esta todo, hay que chequear todo por las dudas y probarlo en la placa real porque nosotros lo probamos con wokwi
-Con ese json lo podes probar en wokwi, tiene dos leds porque flasho la IA con que el led integrado no se ve
-Los botones son por el punto que pide usar el touch


# Proyecto IoT: Comunicación Bidireccional ESP32 y ThingSpeak

Este repositorio contiene la solución para el **Trabajo Práctico N°2**, enfocado en la telemetría de datos ambientales y la actualización remota de dispositivos mediante **OTA (Over-The-Air)**.

## 🚀 Cambios y Mejoras Realizadas (Sketch 2)

Tras las pruebas iniciales, se realizaron ajustes críticos en el código de lectura para asegurar la estabilidad y el cumplimiento de las consignas:

1.  **Corrección del Sistema de Visualización (OLED):**
    *   Se forzó la inicialización del bus I2C en los pines físicos específicos de la placa (**SDA: 21, SCL: 22**) mediante `Wire.begin(21, 22);`[cite: 3, 5].
    *   Se implementó un sistema de **validación de arranque** que detiene el programa e informa por consola si el hardware del display no es detectado[cite: 5].
    *   Se aseguró la actualización efectiva del buffer con `display.display();` tras cada cambio de datos[cite: 5].

2.  **Solución de Legibilidad (Pantalla Rotativa):**
    *   Para evitar el amontonamiento de texto en la pantalla de 128x64, se creó una lógica de **3 pantallas virtuales** que rotan automáticamente cada 3 segundos[cite: 5].
    *   **Pantalla 1:** Datos de control (Valor Aleatorio y Uptime)[cite: 5].
    *   **Pantalla 2:** Datos ambientales (Temperatura y Humedad)[cite: 5].
    *   **Pantalla 3:** Estado del sistema[cite: 5].

3.  **Captura del "Status" de ThingSpeak:**
    *   Se corrigió la lectura del estado. A diferencia de los sensores numéricos, el estado se captura mediante el método especializado `ThingSpeak.getStatus()`, asegurando que los mensajes como "Habilitado" o "En reparación" se muestren correctamente[cite: 5].

## 🛠️ Descripción General del Código

El sistema se divide en dos fases de operación:

### Fase 1: Envío de Datos (Sketch 1)
*   Lee un sensor **DHT22** (Temperatura y Humedad)[cite: 4].
*   Genera un valor aleatorio (100-500) y calcula el tiempo de actividad (*uptime*)[cite: 4].
*   Permite cambiar el estado del dispositivo mediante un **pulsador físico** (Pin 0), rotando entre cuatro estados predefinidos[cite: 4].
*   Publica toda la información en ThingSpeak cada 16 segundos[cite: 4].

### Fase 2: Lectura y Actualización OTA (Sketch 2)
*   **Habilitación de OTA:** El dispositivo queda a la escucha de nuevas actualizaciones de firmware sin necesidad de cables[cite: 2, 5].
*   **Sincronización:** Consulta el canal de ThingSpeak utilizando `readMultipleFields` para obtener todos los valores en una sola transacción HTTP, optimizando el consumo de energía y ancho de banda[cite: 5].
*   **Interfaz de Usuario:** Utiliza un menú cíclico en el display OLED para presentar la información de manera clara y profesional[cite: 5].

## 📋 Requisitos de Hardware
*   **ESP32** (DevKit V4)[cite: 3].
*   **Sensor DHT22** (Conectado al Pin 33)[cite: 3].
*   **Display OLED SSD1306** (Conexión I2C)[cite: 3].
*   **Pulsador** (Conectado al Pin 0)[cite: 3].

---

**Nota:** Las claves de API y el ID de canal deben configurarse en las constantes `Channel`, `APIKey` y `ReadAPIKey` antes de la compilación[cite: 4, 5].