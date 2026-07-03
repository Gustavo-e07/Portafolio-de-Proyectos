#include <Arduino.h>

// ============================================================
// ESP32 + RS485 TTL automatico + RT88H01
// RECEPTOR
// ============================================================

#define RS485_RX 16   // TXD del convertidor -> RX ESP32
#define RS485_TX 17   // TX ESP32 -> RXD del convertidor

//HardwareSerial RT88H01(2);

//String buffer = "";

void setup() {
 // Serial.begin(115200);
 // delay(1000);

  //RT88H01.begin(9600, SERIAL_8N1, RS485_RX, RS485_TX);

 // Serial.println("Receptor RT88H01 listo");
 // Serial.println("Esperando datos...");
}

void loop() {
  while (RT88H01.available()) {
    char c = RT88H01.read();

    if (c == '\n') {
      buffer.trim();

      if (buffer.length() > 0) {
        Serial.println("================================");
        Serial.print("Recibido: ");
        Serial.println(buffer);

        procesarMensaje(buffer);

        Serial.println("================================");
      }

      buffer = "";
    } else {
      buffer += c;
    }
  }
}

void procesarMensaje(String mensaje) {
  String datos[11];

  int indice = 0;
  int inicio = 0;

  for (int i = 0; i < mensaje.length(); i++) {
    if (mensaje.charAt(i) == ',') {
      datos[indice] = mensaje.substring(inicio, i);
      inicio = i + 1;
      indice++;

      if (indice >= 10) {
        break;
      }
    }
  }

  datos[indice] = mensaje.substring(inicio);

  String robot = datos[0];
  unsigned long paquete = datos[1].toInt();
  float latitud = datos[2].toFloat();
  float longitud = datos[3].toFloat();
  float velocidad = datos[4].toFloat();
  float bateria = datos[5].toFloat();
  float temperatura = datos[6].toFloat();
  float roll = datos[7].toFloat();
  float pitch = datos[8].toFloat();
  float yaw = datos[9].toFloat();
  String modo = datos[10];

  Serial.print("Robot: ");
  Serial.println(robot);

  Serial.print("Paquete: ");
  Serial.println(paquete);

  Serial.print("Latitud: ");
  Serial.println(latitud, 6);

  Serial.print("Longitud: ");
  Serial.println(longitud, 6);

  Serial.print("Velocidad: ");
  Serial.print(velocidad);
  Serial.println(" km/h");

  Serial.print("Bateria: ");
  Serial.print(bateria);
  Serial.println(" %");

  Serial.print("Temperatura: ");
  Serial.print(temperatura);
  Serial.println(" C");

  Serial.print("Roll: ");
  Serial.println(roll);

  Serial.print("Pitch: ");
  Serial.println(pitch);

  Serial.print("Yaw: ");
  Serial.println(yaw);

  Serial.print("Modo: ");
  Serial.println(modo);
}
