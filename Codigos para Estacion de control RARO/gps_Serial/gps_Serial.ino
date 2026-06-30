#include <TinyGPSPlus.h>

// ============================================================
// Hiwonder GPS Module V1.0 con ESP32
// ============================================================
// GPS TX  -> ESP32 GPIO 26
// GPS RX  -> ESP32 GPIO 25
// GPS GND -> GND
// GPS VCC -> 5V o 3.3V según el módulo

#define GPS_RX 26
#define GPS_TX 25

#define GPS_BAUD 9600

TinyGPSPlus gps;
HardwareSerial GPSserial(2);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("================================");
  Serial.println("Iniciando Hiwonder GPS V1.0");
  Serial.println("UART: Serial2");
  Serial.println("Baudrate GPS: 9600");
  Serial.println("RX ESP32: GPIO 26");
  Serial.println("TX ESP32: GPIO 25");
  Serial.println("================================");

  GPSserial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX, GPS_TX);

  Serial.println("Esperando datos NMEA del GPS...");
}

void loop() {
  while (GPSserial.available() > 0) {
    char c = GPSserial.read();

    // Descomenta esta línea si quieres ver los datos crudos NMEA:
    Serial.write(c);

    gps.encode(c);
  }

  static unsigned long tiempoAnterior = 0;

  if (millis() - tiempoAnterior >= 1000) {
    tiempoAnterior = millis();

    Serial.println();
    Serial.println("========== GPS ==========");

    Serial.print("Caracteres recibidos: ");
    Serial.println(gps.charsProcessed());

    Serial.print("Sentencias correctas: ");
    Serial.println(gps.passedChecksum());

    Serial.print("Sentencias con error: ");
    Serial.println(gps.failedChecksum());

    if (gps.location.isValid()) {
      Serial.print("Latitud : ");
      Serial.println(gps.location.lat(), 6);

      Serial.print("Longitud: ");
      Serial.println(gps.location.lng(), 6);
    } else {
      Serial.println("Ubicacion: no valida todavia");
    }

    if (gps.satellites.isValid()) {
      Serial.print("Satelites: ");
      Serial.println(gps.satellites.value());
    } else {
      Serial.println("Satelites: no valido");
    }

    if (gps.altitude.isValid()) {
      Serial.print("Altitud: ");
      Serial.print(gps.altitude.meters());
      Serial.println(" m");
    } else {
      Serial.println("Altitud: no valida");
    }

    if (gps.speed.isValid()) {
      Serial.print("Velocidad: ");
      Serial.print(gps.speed.kmph());
      Serial.println(" km/h");
    } else {
      Serial.println("Velocidad: no valida");
    }

    if (gps.date.isValid() && gps.time.isValid()) {
      Serial.print("Fecha UTC: ");
      Serial.print(gps.date.day());
      Serial.print("/");
      Serial.print(gps.date.month());
      Serial.print("/");
      Serial.println(gps.date.year());

      Serial.print("Hora UTC : ");
      Serial.print(gps.time.hour());
      Serial.print(":");
      Serial.print(gps.time.minute());
      Serial.print(":");
      Serial.println(gps.time.second());
    } else {
      Serial.println("Fecha/Hora: no valida");
    }

    Serial.println("=========================");
  }
}