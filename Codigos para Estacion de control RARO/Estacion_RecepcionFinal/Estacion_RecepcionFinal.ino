//============================================================
//Librerias
//============================================================
#include <Arduino.h>
Filtre su búsqueda...
Tipo:

Todo
Tema:

Todo




#include <Wire.h>
#include <Adafruit_INA219.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <ModbusMaster.h>
#include <stdint.h>

//============================================================
//Declaracion de pines
//============================================================

//Pines de LoRa
#define RS485_RX 13   // TXD del convertidor -> RX ESP32
#define RS485_TX 14   // TX ESP32 -> RXD del convertidor

//Pines de Sensor NPK
#define NPK_RX 18   // TXD del sensor -> RX ESP32
#define NPK_TX 17   // TX ESP32 -> RXD del sensor

//Pines de Amperimetro y lcd
#define I2C_SDA 6
#define I2C_SCL 7

// Pines Sabertooth (PPM / Servo Signal)
#define SABERTOOTH_LEFT_PIN  10   // S1
#define SABERTOOTH_RIGHT_PIN 11   // S2

// Indicadores LED
#define Led_NPKRead          4

#define Led_LoRaConect       5
#define Led_LoRaunDesconect  8

#define Led_Aut              9
#define Led_Man              12


//============================================================
//Objetos y perifericos
//============================================================
HardwareSerial RT88H01(1);//UART1 LoRa

HardwareSerial SerialNPK(2); //UART2 NPK 
ModbusMaster node;

//Amperimetro
Adafruit_INA219 ina219;

// ============================================================
// CONSTANTES DE CONTROL RC Y JOYSTICK
// ============================================================
const int VALOR_MIN_CONTROL = -512;
const int VALOR_MAX_CONTROL = 512;
const int ZONA_MUERTA       = 50;

const int PULSO_REVERSA = 1000; // us
const int PULSO_NEUTRO  = 1500; // us
const int PULSO_AVANCE  = 2000; // us

const unsigned long PERIODO_RC_US   = 20000; // Periodo de 50Hz (20ms)
const unsigned long TIMEOUT_LORA_MS = 3000;  // Timeout de seguridad


//============================================================
//Variables globales Inicializadas
//============================================================
// Pulsos RC para Sabertooth
volatile int pulsoIzquierdo = PULSO_NEUTRO;
volatile int pulsoDerecho   = PULSO_NEUTRO;
unsigned long inicioFrameRC = 0;
bool frameActivo = false;

//Lecturas locales

  //NPK Promedios
  float humedadProm = 0;           
  float temperaturaProm = 0;
  float ecProm = 0;
  float phProm = 0;
  float nitrogenoProm = 0;
  float fosforoProm = 0;
  float potasioProm = 0;
  float salinidadProm = 0;
  float tdsProm = 0;
  //Amperiometro
  float AmpProm = 0.0;

  //NPK Sumas
  float sumaHum=0;
  float sumaTemp=0;
  float sumaEC = 0;
  float sumaPH = 0;
  float sumaN = 0 ;
  float sumaP = 0;
  float sumaK = 0;
  float sumaSal = 0;
  float sumaTDS = 0;

  int lecturasValidasNPK = 0;
  bool solicitandoNPK = false;
  unsigned long ultimoMuestreoNPK = 0;
  int Sensor_Estado = 0;
  
  //VariablesLoRa
  String bufferLoRa = "";
  unsigned long ultimoComandoLoRa = 0;

  String McLoRa = "";
  int Contador = 0;
  int Conexion = 0;
  int a = 0;
  int b = 0;
  int x = 0;
  int y = 0;
  int L1 = 0;
  int R1 = 0;
  int dpad = 0;
  int buttons = 0; 
  int misc = 0;
  int Lx = 0;
  int Ly = 0;
  int Rx = 0;
  int Ry = 0;
  int L2 = 0;
  int R2 = 0;
  float latitud = 0; 
  float longitud = 0;
  float altitud = 0;
  int NSatelites = 0;
  float VelGPS = 0;

//Logica de modo (0: Manual, 1: Autonomo);
int Modo = 0;
int prevMisc = 0;


//Lectura serial recibida
  float tempRobot = 0.0;
  int NSatelitesRobot = 0;

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

//Paqute de datos
struct __attribute__((packed)) DatosRobot {

    // -------------------------
    // Control LoRa
    // -------------------------
    uint16_t Contador;
    uint8_t Conexion;

    int16_t a;
    int16_t b;
    int16_t x;
    int16_t y;

    int16_t L1;
    int16_t R1;
    int16_t dpad;
    uint16_t buttons;

    uint8_t misc;

    int16_t Lx;
    int16_t Ly;
    int16_t Rx;
    int16_t Ry;

    int16_t L2;
    int16_t R2;

    // -------------------------
    // GPS
    // -------------------------
    float latitud;
    float longitud;
    float altitud;

    uint8_t NSatelites;
    float VelGPS;

    // -------------------------
    // Estado
    // -------------------------
    uint8_t Modo;
    uint8_t Sensor_Estado;

    uint16_t pulsoIzquierdo;
    uint16_t pulsoDerecho;

    // -------------------------
    // Sensores NPK
    // -------------------------
    float humedad;
    float temperatura;
    float ec;
    float ph;
    float nitrogeno;
    float fosforo;
    float potasio;
    float salinidad;
    float tds;

    // -------------------------
    // Amperímetro
    // -------------------------
    float amperaje;
};

// ============================================================
// PROTOTIPOS
// ============================================================
void actualizarPulsosRC();
void leerLoRa();
void procesarMensajeLora(String mensaje);
void controlarSabertooth(int rx, int ry);
void detenerMotores();
void actualizarLEDs();
void iniciarLecturaNPK();
void atenderNPK();
void EnviarMensajes();


void setup() {
  Serial.begin(115200);
  delay(1000);

  // Configuración de Pines Sabertooth
  pinMode(SABERTOOTH_LEFT_PIN, OUTPUT);
  pinMode(SABERTOOTH_RIGHT_PIN, OUTPUT);
  digitalWrite(SABERTOOTH_LEFT_PIN, LOW);
  digitalWrite(SABERTOOTH_RIGHT_PIN, LOW);

  // Configuración de LEDs
  pinMode(Led_NPKRead, OUTPUT);
  pinMode(Led_LoRaConect, OUTPUT);
  pinMode(Led_LoRaunDesconect, OUTPUT);
  pinMode(Led_Aut, OUTPUT);
  pinMode(Led_Man, OUTPUT);

    actualizarLEDs();
    detenerMotores();

  // Inicializar LoRa (UART1)
    RT88H01.begin(9600, SERIAL_8N1, RS485_RX, RS485_TX);

    // Inicializar NPK Modbus (UART2)
    SerialNPK.begin(9600, SERIAL_8N1, NPK_RX, NPK_TX);
    delay(50);
    node.begin(1, SerialNPK);

    // Inicializar I2C
    Wire.begin(I2C_SDA, I2C_SCL);
    ina219.begin(&Wire);

    // Tiempo de estabilización con señal Neutro enviada a Sabertooth
    unsigned long inicio = millis();
    while (millis() - inicio < 2000) {
        actualizarPulsosRC();
    }

    ultimoComandoLoRa = millis();
}

void loop() {
      // 1. Generador RC continuo (DEBE ejecutarse constantemente)
    actualizarPulsosRC();

    // 2. Procesar tramas LoRa
    leerLoRa();

    // 3. Procesar lectura de sensor Modbus de forma asíncrona
    atenderNPK();

    // 4. Seguridad: Si se pierde la conexión LoRa, apagar motores
    if (millis() - ultimoComandoLoRa > TIMEOUT_LORA_MS) {
        Conexion = 0;
        detenerMotores();
    }

    // 5. Refrescar LEDs de estado
    actualizarLEDs();
 
}

void prepararDatos(DatosRobot &datos) {

    datos.Contador = Contador;
    datos.Conexion = Conexion;

    datos.a = a;
    datos.b = b;
    datos.x = x;
    datos.y = y;

    datos.L1 = L1;
    datos.R1 = R1;
    datos.dpad = dpad;
    datos.buttons = buttons;

    datos.misc = misc;

    datos.Lx = Lx;
    datos.Ly = Ly;
    datos.Rx = Rx;
    datos.Ry = Ry;

    datos.L2 = L2;
    datos.R2 = R2;

    datos.latitud = latitud;
    datos.longitud = longitud;
    datos.altitud = altitud;

    datos.NSatelites = NSatelites;
    datos.VelGPS = VelGPS;

    datos.Modo = Modo;
    datos.Sensor_Estado = Sensor_Estado;

    datos.pulsoIzquierdo = pulsoIzquierdo;
    datos.pulsoDerecho = pulsoDerecho;

    datos.humedad = humedadProm;
    datos.temperatura = temperaturaProm;
    datos.ec = ecProm;
    datos.ph = phProm;
    datos.nitrogeno = nitrogenoProm;
    datos.fosforo = fosforoProm;
    datos.potasio = potasioProm;
    datos.salinidad = salinidadProm;
    datos.tds = tdsProm;

    datos.amperaje = AmpProm;
}

void enviarDatos() {

    DatosRobot datos;

    prepararDatos(datos);

    Serial.write((uint8_t*)&datos, sizeof(datos));
}

// ============================================================
// GENERADOR RC NO BLOQUEANTE (50 Hz por Software)
// ============================================================
void actualizarPulsosRC() {
    unsigned long ahora = micros();

    // Iniciar nuevo pulso cada 20000 us (50Hz)
    if (!frameActivo || (ahora - inicioFrameRC >= PERIODO_RC_US)) {
        inicioFrameRC = ahora;
        frameActivo = true;

        digitalWrite(SABERTOOTH_LEFT_PIN, HIGH);
        digitalWrite(SABERTOOTH_RIGHT_PIN, HIGH);
    }

    // Apagar pin izquierdo al cumplir su ancho de pulso
    if ((ahora - inicioFrameRC) >= (unsigned long)pulsoIzquierdo) {
        digitalWrite(SABERTOOTH_LEFT_PIN, LOW);
    }

    // Apagar pin derecho al cumplir su ancho de pulso
    if ((ahora - inicioFrameRC) >= (unsigned long)pulsoDerecho) {
        digitalWrite(SABERTOOTH_RIGHT_PIN, LOW);
    }
}

// ============================================================
// RECEPCIÓN LORA
// ============================================================
void leerLoRa() {
    while (RT88H01.available()) {
        char c = RT88H01.read();

        if (c == '\n') {
            bufferLoRa.trim();
            if (bufferLoRa.length() > 0) {
                procesarMensajeLora(bufferLoRa);
            }
            bufferLoRa = "";
        } else {
            bufferLoRa += c;
            if (bufferLoRa.length() > 250) bufferLoRa = "";
        }
    }
}

// ============================================================
// PROCESAR MENSAJE LORA Y ENCLAVAMIENTO DE MODO
// ============================================================
void procesarMensajeLora(String mensaje) {
    String datos[23];
    int indice = 0, inicio = 0;

    for (int i = 0; i < mensaje.length(); i++) {
        if (mensaje.charAt(i) == ',') {
            datos[indice] = mensaje.substring(inicio, i);
            inicio = i + 1;
            indice++;
            if (indice >= 23) break;
        }
    }
    datos[indice] = mensaje.substring(inicio);

    if (indice < 22) return; // Asegurar que recibimos la trama completa

    McLoRa     = datos[0];
    Contador   = datos[1].toInt();
    Conexion   = datos[2].toInt();
    a          = datos[3].toInt();
    b          = datos[4].toInt();
    x          = datos[5].toInt();
    y          = datos[6].toInt();
    L1         = datos[7].toInt();
    R1         = datos[8].toInt();
    dpad       = datos[9].toInt();
    buttons    = datos[10].toInt();
    misc       = datos[11].toInt();
    Lx         = datos[12].toInt();
    Ly         = datos[13].toInt();
    Rx         = datos[14].toInt();
    Ry         = datos[15].toInt();
    L2         = datos[16].toInt();
    R2         = datos[17].toInt();
    latitud    = datos[18].toFloat();
    longitud   = datos[19].toFloat();
    altitud    = datos[20].toFloat();
    NSatelites = datos[21].toInt();
    VelGPS     = datos[22].toFloat();

    ultimoComandoLoRa = millis();

    // ============================================================
    // PROCESAR COMANDOS MISC
    // ============================================================
    // misc == 1 -> Cambiar entre Manual y Autónomo
    if (misc == 1 && prevMisc != 1) {

        Modo = (Modo == 0) ? 1 : 0;

        Serial.print("CAMBIO DE MODO A: ");
        Serial.println(Modo == 1 ? "AUTÓNOMO" : "MANUAL");
    }


    // misc == 2 -> Iniciar lectura del sensor NPK
    if (misc == 2 && prevMisc != 2) {

        Serial.print("INICIANDO LECTURA NPK");

        iniciarLecturaNPK();
    }


    // Guardar estado anterior
    prevMisc = misc;


    // ============================================================
    // CONTROL DE MOTORES
    // ============================================================

    if (Modo == 0) {

        // Modo Manual
        controlarSabertooth(Lx, Ly);

    } else {

        // Modo Autónomo
        detenerMotores();
    }
}

// ============================================================
// CONTROL DIFERENCIAL MOTORES
// ============================================================
void controlarSabertooth(int rx, int ry) {
    // Zona Muerta
    if (abs(rx) < ZONA_MUERTA) rx = 0;
    if (abs(ry) < ZONA_MUERTA) ry = 0;

    rx = constrain(rx, VALOR_MIN_CONTROL, VALOR_MAX_CONTROL);
    ry = constrain(ry, VALOR_MIN_CONTROL, VALOR_MAX_CONTROL);

    int avance = ry;
    int giro   = rx;

    int vIzquierda = avance + giro;
    int vDerecha   = avance - giro;

    vIzquierda = constrain(vIzquierda, VALOR_MIN_CONTROL, VALOR_MAX_CONTROL);
    vDerecha   = constrain(vDerecha,   VALOR_MIN_CONTROL, VALOR_MAX_CONTROL);

    pulsoIzquierdo = map(vIzquierda, VALOR_MIN_CONTROL, VALOR_MAX_CONTROL, PULSO_REVERSA, PULSO_AVANCE);
    pulsoDerecho   = map(vDerecha,   VALOR_MIN_CONTROL, VALOR_MAX_CONTROL, PULSO_REVERSA, PULSO_AVANCE);
}

void detenerMotores() {
    pulsoIzquierdo = PULSO_NEUTRO;
    pulsoDerecho   = PULSO_NEUTRO;
}

// ============================================================
// ACTUALIZAR ESTADO DE LEDS
// ============================================================
void actualizarLEDs() {
    digitalWrite(Led_Aut, Modo == 1 ? HIGH : LOW);
    digitalWrite(Led_Man, Modo == 0 ? HIGH : LOW);

    digitalWrite(Led_LoRaConect, Conexion == 1 ? HIGH : LOW);
    digitalWrite(Led_LoRaunDesconect, Conexion == 0 ? HIGH : LOW);

    digitalWrite(Led_NPKRead, Sensor_Estado == 1 ? HIGH : LOW);
    
 //       Serial.print("Escucho a: ");
    Serial.print(McLoRa);
 //       Serial.print(" Contador:");
//    Serial.print(Contador);
//        Serial.print(" Conexion:");
    Serial.print(Conexion);
        Serial.print(" a");
    Serial.print(a);
        Serial.print(" b");
    Serial.print(b);
        Serial.print(" x");
    Serial.print(x);
        Serial.print(" y");
    Serial.print(y);
        Serial.print(" L1");
    Serial.print(L1);
        Serial.print(" R1");
    Serial.print(R1);
        Serial.print(" dpad");
    Serial.print(dpad);
        Serial.print(" buttons");
    Serial.print(buttons);
        Serial.print(" Modo");
    Serial.print(Modo);
        Serial.print(" estado sensor");
    Serial.print(Sensor_Estado);
        Serial.print(" pulso izquierdo");
    Serial.print(pulsoIzquierdo);
        Serial.print(" pulso derecho");
    Serial.print(pulsoDerecho);
        Serial.print(" Rx");
    Serial.print(Rx);
        Serial.print(" RY");
    Serial.print(Ry);
        Serial.print(" latitud");
    Serial.print(latitud);
        Serial.print(" longitud");
    Serial.print(longitud);
        Serial.print(" altitud");
    Serial.print(altitud);
        Serial.print(" satelites");
    Serial.print(NSatelites);
        Serial.print(" velGPS");
    Serial.println(VelGPS);
        Serial.print(" N");
    Serial.print("Humedad: ");
        Serial.print(humedadProm, 2);
    Serial.println(" %");

    Serial.print("Temperatura: ");
        Serial.print(temperaturaProm, 2);
    Serial.println(" °C");

    Serial.print("EC: ");
        Serial.print(ecProm, 2);
    Serial.println();

    Serial.print("pH: ");
        Serial.print(phProm, 2);
    Serial.println();

    Serial.print("Nitrogeno: ");
        Serial.print(nitrogenoProm, 2);
    Serial.println();

    Serial.print("Fosforo: ");
        Serial.print(fosforoProm, 2);
    Serial.println();

    Serial.print("Potasio: ");
        Serial.print(potasioProm, 2);
    Serial.println();

    Serial.print("Salinidad: ");
        Serial.print(salinidadProm, 2);
    Serial.println();

    Serial.print("TDS: ");
    Serial.print(tdsProm, 2);
    Serial.println(" ppm");

    Serial.print("Amperaje: ");
    Serial.print(AmpProm, 2);
    Serial.println(" A");

    Serial.println("----------------------");
    

}

// ============================================================
// SENSOR NPK MODBUS ASÍNCRONO
// ============================================================
void iniciarLecturaNPK() {
    if (solicitandoNPK) return;
    solicitandoNPK = true;
    Sensor_Estado = 1;
    lecturasValidasNPK = 0;
    sumaHum = 0; 
    sumaTemp = 0; 
    sumaEC = 0; 
    sumaPH = 0;
    sumaN = 0; 
    sumaP = 0; 
    sumaK = 0; 
    sumaSal = 0; 
    sumaTDS = 0;
    ultimoMuestreoNPK = millis();
}

void atenderNPK() {
    if (!solicitandoNPK) return;

    if (millis() - ultimoMuestreoNPK >= 50) {
        ultimoMuestreoNPK = millis();

        uint8_t result = node.readHoldingRegisters(0x0000, 9);
        if (result == node.ku8MBSuccess) {
            sumaHum  += node.getResponseBuffer(0) / 10.0;
            sumaTemp += node.getResponseBuffer(1) / 10.0;
            sumaEC   += node.getResponseBuffer(2);
            sumaPH   += node.getResponseBuffer(3) / 10.0;
            sumaN    += node.getResponseBuffer(4);
            sumaP    += node.getResponseBuffer(5);
            sumaK    += node.getResponseBuffer(6);
            sumaSal  += node.getResponseBuffer(7);
            sumaTDS  += node.getResponseBuffer(8);
            lecturasValidasNPK++;
        }

        if (lecturasValidasNPK >= 30) {
            humedadProm     = sumaHum / 30.0;
            temperaturaProm = sumaTemp / 30.0;
            ecProm          = sumaEC / 30.0;
            phProm          = sumaPH / 30.0;
            nitrogenoProm   = sumaN / 30.0;
            fosforoProm     = sumaP / 30.0;
            potasioProm     = sumaK / 30.0;
            salinidadProm   = sumaSal / 30.0;
            tdsProm         = sumaTDS / 30.0;
           Serial.print("Hum: "); Serial.print(humedadProm); Serial.print("% | ");
           Serial.print("Temp: "); Serial.print(temperaturaProm); Serial.print("C | ");
           Serial.print("EC: "); Serial.print(ecProm); Serial.print(" | ");
           Serial.print("pH: "); Serial.print(phProm); Serial.print(" | ");
            Serial.print("N: "); Serial.print(nitrogenoProm); Serial.print(" | ");
            Serial.print("P: "); Serial.print(fosforoProm); Serial.print(" | ");
            Serial.print("K: "); Serial.print(potasioProm); Serial.print(" | ");
            Serial.print("Sal: "); Serial.print(salinidadProm); Serial.print(" | ");
            Serial.print("TDS: "); Serial.println(tdsProm);
            Sensor_Estado = 0;
            solicitandoNPK = false;
        }
    }
}