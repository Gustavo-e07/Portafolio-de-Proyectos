#include <Arduino.h>

// ============================================================
// ESP32 + RS485 TTL automatico + RT88H01
// TRANSMISOR
// ============================================================

#define RS485_RX 27   // TXD del convertidor -> RX ESP32
#define RS485_TX 14   // TX ESP32 -> RXD del convertidor

HardwareSerial RT88H01(2);

float latitud = 25.543210;
float longitud = -99.123456;
float velocidad = 0.0;
float bateria = 87.5;
float temperatura = 28.4;
float roll = 0.0;
float pitch = 0.0;
float yaw = 180.0;

String modo = "MANUAL";

unsigned long contador = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  RT88H01.begin(9600, SERIAL_8N1, RS485_RX, RS485_TX);

  Serial.println("Transmisor RT88H01 listo");
  Serial.println("Enviando variables por RS485/Lora...");
}

void loop() {
  contador++;

  // Variables simuladas
  velocidad = random(0, 100) / 10.0;
  temperatura = 25.0 + random(0, 80) / 10.0;
  roll = random(-300, 300) / 10.0;
  pitch = random(-200, 200) / 10.0;
  yaw = random(0, 3600) / 10.0;

  bateria -= 0.1;
  if (bateria < 0) {
    bateria = 100;
  }

  String mensaje = "";

  mensaje += "RARO";
  mensaje += ",";
  mensaje += String(contador);
  mensaje += ",";
  mensaje += String(latitud, 6);
  mensaje += ",";
  mensaje += String(longitud, 6);
  mensaje += ",";
  mensaje += String(velocidad, 2);
  mensaje += ",";
  mensaje += String(bateria, 1);
  mensaje += ",";
  mensaje += String(temperatura, 2);
  mensaje += ",";
  mensaje += String(roll, 2);
  mensaje += ",";
  mensaje += String(pitch, 2);
  mensaje += ",";
  mensaje += String(yaw, 2);
  mensaje += ",";
  mensaje += modo;

  RT88H01.println(mensaje);

  Serial.print("Enviado: ");
  Serial.println(mensaje);

  delay(1000);
}