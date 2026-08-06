//============================================================
//Librerias
//============================================================
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_INA219.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <ModbusMaster.h>


//============================================================
//Declaracion de pines
//============================================================

//Pines de LoRa
#define RS485_RX 37   // TXD del convertidor -> RX ESP32
#define RS485_TX 38   // TX ESP32 -> RXD del convertidor

//Pines de Sensor NPK
#define NPK_RX 2   // TXD del sensor -> RX ESP32
#define NPK_TX 1   // TX ESP32 -> RXD del sensor

//Pines de Amperimetro
#define I2C_SDA 6
#define I2C_SCL 7

//Pines de LCD
#define LCD_SDA 21
#define LCD_SCL 20
// Dirección I2C típica para pantallas OLED grandes (puede ser 0x3C o 0x3D)
#define SCREEN_ADDRESS 0x3C 
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

//Pines a los motores
#define MotorR 11
#define MotorL 10


#define Led_NPKRead 3

#define Led_LoRaConect 8
#define Led_LoRaunDesconect 18

#define Led_Aut 17
#define Led_Man 16


//============================================================
//Decalracion de dispositivos
//============================================================
//NPK
ModbusMaster node; // Create Modbus object

//LoRa
HardwareSerial RT88H01(2);

//Amperimetro
Adafruit_INA219 ina219;

// Pantalla OLED SH1106
Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

//LCD


//============================================================
//Variables globales Inicializadas
//============================================================
String buffer = "";//inicializamos mensaje

// Estado global de modo y bit anterior de "misc" para detección de flanco
int Modo = 0;
int prevMiscBit = 0;

//Variables LoRa recibidas
int Contador;
int Conexion =0;


//Variables a enviar
float Latitud=0;
float Longitud=0;
float Altitud=0;
int NSatelites =0;
float VelGPS =0;

//NPK
float humedadProm = 0;
float temperaturaProm = 0;
float ecProm = 0;
float phProm = 0;
float nitrogenoProm = 0;
float fosforoProm = 0;
float potasioProm = 0;
float salinidadProm = 0;
float tdsProm = 0;
float AmpProm = 0.0;

//seriales recibidos
float tempRobot = 0.0;
float EstacionX = 0;
float EstacionY = 0;
float EstacionZ = 0;
float RobotX = 0;
float RobotY = 0;
float RobotZ = 0;
float Roll = 0;
float Pitch = 0;
float Yaw = 0;
int realizarMedicion = 0;


//============================================================
//Funciones
//============================================================
void leerPromedioNPK() {

  float sumaHum = 0;
  float sumaTemp = 0;
  float sumaEC = 0;
  float sumaPH = 0;
  float sumaN = 0;
  float sumaP = 0;
  float sumaK = 0;
  float sumaSal = 0;

  int lecturasValidas = 0;

  while (lecturasValidas < 30) {

    // Lee 8 registros consecutivos
    uint8_t result = node.readHoldingRegisters(0x0000, 8);

    if (result == node.ku8MBSuccess) {

      sumaHum  += node.getResponseBuffer(0) / 10.0;
      sumaTemp += node.getResponseBuffer(1) / 10.0;
      sumaEC   += node.getResponseBuffer(2);
      sumaPH   += node.getResponseBuffer(3) / 10.0;
      sumaN    += node.getResponseBuffer(4);
      sumaP    += node.getResponseBuffer(5);
      sumaK    += node.getResponseBuffer(6);
      sumaSal  += node.getResponseBuffer(7);

      lecturasValidas++;
    }

    delay(200);
  }

  humedadProm     = sumaHum / 30.0;
  temperaturaProm = sumaTemp / 30.0;
  ecProm          = sumaEC / 30.0;
  phProm          = sumaPH / 30.0;
  nitrogenoProm   = sumaN / 30.0;
  fosforoProm     = sumaP / 30.0;
  potasioProm     = sumaK / 30.0;
  salinidadProm   = sumaSal / 30.0;
}


void procesarMensajeLora(String mensaje) {
  String datos[22];

  int indice = 0;
  int inicio = 0;

  for (int i = 0; i < mensaje.length(); i++) {
    if (mensaje.charAt(i) == ',') {
      datos[indice] = mensaje.substring(inicio, i);
      inicio = i + 1;
      indice++;

      if (indice >= 22) {
        break;
      }
    }
  }
  
  datos[indice] = mensaje.substring(inicio);

  Contador =datos[0].toInt();
  Conexion = datos[1].toInt();
  int a = datos[2].toInt();
  int b = datos[3].toInt();
  int x = datos[4].toInt();
  int y = datos[5].toInt();
  int L1 = datos[6].toInt();
  int R1 = datos[7].toInt();
  int dpad = datos[8].toInt();
  int buttons = datos[9].toInt();
  int misc = datos[10].toInt();
  int Lx = datos[11].toInt();
  int Ly = datos[12].toInt();
  int Rx = datos[13].toInt();
  int Ry = datos[14].toInt();
  int L2 = datos[15].toInt();
  int R2 = datos[16].toInt();
  float Latitud = datos[17].toFloat();
  float longitud = datos[18].toFloat();
  float altitud = datos[19].toFloat();
  int NSatelites = datos[20].toInt();
  float VelGPS = datos[21].toFloat();

  digitalWrite(Led_LoRaConect, Conexion);
  digitalWrite(Led_LoRaunDesconect, !Conexion);

  // Toggle de `Modo` en el flanco ascendente del bit 0 de `misc`.
  // Así, cuando `misc` pasa de 0->1 se invierte `Modo`; mientras tanto
  // `Modo` permanece en su valor actual aún si `misc` baja a 0.
  int miscBit = misc & 0x01;
  if (miscBit && !prevMiscBit) {
    // flanco ascendente: toggle
    Modo = (Modo == 0) ? 1 : 0;
  }
  prevMiscBit = miscBit;

  digitalWrite(Led_Aut, Modo);
  digitalWrite(Led_Man, !Modo);

  String MensajeToLoRa =
    String(Contador) + "," +
    String(Conexion) + "," +
    String(Modo) + "," +
    String(Roll) + "," +
    String(Pitch) + "," +
    String(Yaw);

  RT88H01.println(MensajeToLoRa);

  if (Modo == 1) {
    // Modo Autonomo
    // Leer una línea del puerto serial principal y procesarla
    String MensajeSerial = "";
    while (Serial.available()) {
      char c = Serial.read();
      if (c == '\n') {
        break;
      }
      if (c != '\r') {
        MensajeSerial += c;
      }
    }

    if (MensajeSerial.length() > 0) {
      procesarMensajeSerial(MensajeSerial);
    }
    if (realizarMedicion == 1) {
        leerPromedioNPK();
      // Aquí puedes agregar el código para realizar la medición
      // y actualizar las variables de NPK y amperímetro
    }
    // Aquí puedes agregar el código adicional para el modo autónomo
  } else {
    // Modo Manual
    if(misc == 2)
    // Aquí puedes agregar el código para el modo manual
    leerPromedioNPK();
  }

  String MensajeToSerial =
    String(Contador) + "," +
    String(Conexion) + "," +
    String(Modo) + "," +

    String(Lx) + "," +
    String(Ly) + "," +
    String(Rx) + "," +
    String(Ry) + "," +

    String(L2) + "," +
    String(R2) + "," +

    String(Latitud,6) + "," +
    String(Longitud,6) + "," +
    String(Altitud,2) + "," +
    String(NSatelites) + "," +
    String(VelGPS,2) + "," +

    String(temperaturaProm,2) + "," +
    String(humedadProm,2) + "," +
    String(ecProm,2) + "," +
    String(phProm,2) + "," +
    String(nitrogenoProm,2) + "," +
    String(fosforoProm,2) + "," +
    String(potasioProm,2) + "," +
    String(salinidadProm,2) + "," +
    String(tdsProm,2) + "," +
    String(AmpProm,2);

  Serial.println(MensajeToSerial);

}

void procesarMensajeSerial(String mensaje) {
  String datos[11];

  int indice = 0;
  int inicio = 0;

  for (int i = 0; i < mensaje.length() && indice < 10; i++) {
    if (mensaje.charAt(i) == ',') {
      datos[indice] = mensaje.substring(inicio, i);
      inicio = i + 1;
      indice++;
    }
  }

  datos[indice] = mensaje.substring(inicio);
  indice++;

  if (indice < 11) {
    // Mensaje incompleto: no se actualiza el estado
    return;
  }

  tempRobot = datos[0].toFloat();
  EstacionX = datos[1].toFloat();
  EstacionY = datos[2].toFloat();
  EstacionZ = datos[3].toFloat();
  RobotX = datos[4].toFloat();
  RobotY = datos[5].toFloat();
  RobotZ = datos[6].toFloat();
  Roll = datos[7].toFloat();
  Pitch = datos[8].toFloat();
  Yaw = datos[9].toFloat();
  realizarMedicion = datos[10].toInt();
}


void setup() {
    //seria;
  Serial.begin(115200);
  delay(1000);

  //Pines de salida
  pinMode(Led_NPKRead, OUTPUT);
  pinMode(Led_LoRaConect, OUTPUT);
  pinMode(Led_LoRaunDesconect, OUTPUT);
  pinMode(Led_Aut, OUTPUT);
  pinMode(Led_Man, OUTPUT);

    //Inicializamos los pines de salida
    digitalWrite(Led_NPKRead, LOW);
    digitalWrite(Led_LoRaConect, Conexion);
    digitalWrite(Led_LoRaunDesconect, !Conexion);
    digitalWrite(Led_Aut, Modo);
    digitalWrite(Led_Man, !Modo);

    //LoRa
  RT88H01.begin(9600, SERIAL_8N1, RS485_RX, RS485_TX);
    
  //NPK
    Serial2.begin(9600, SERIAL_8N1, NPK_RX, NPK_TX);   
    node.begin(1, Serial2);    // ID del sensor

    /*
    // Inicia la comunicación I2C y la pantalla
    if(!display.begin(SCREEN_ADDRESS, true)) {     
        Serial.println(F("No se encontro la pantalla OLED"));
        for(;;); // Detiene el programa si falla
    }
    display.clearDisplay(); // Limpia el buffer de la pantalla
    */

    // El ESP32-S3 reasigna el hardware I2C a los pines asignados aquí:
    Wire.begin(I2C_SDA, I2C_SCL);

    if (!ina219.begin(&Wire)) {
        Serial.println("No se encuentra el modulo INA219. ¡Revisa pines y conexiones!");
        while (1) { delay(10); }
    }
  
}

void loop() {
   // display.clearDisplay();

  while (RT88H01.available()) {
    
    char c = RT88H01.read();

    if (c == '\n') {
      buffer.trim();

      if (buffer.length() > 0) {
        Serial.println("================================");
        Serial.print("Recibido: ");
        Serial.println(buffer);

        procesarMensajeLora(buffer);    

        Serial.println("================================");
      }

      buffer = "";
    } else {
      buffer += c;
    }
  }
  Modo = 0;
  
}
