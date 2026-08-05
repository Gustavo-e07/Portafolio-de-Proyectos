#include <Arduino.h>

// ============================================================
// LORA / RT88H01
// ============================================================
#define LORA_RX 37
#define LORA_TX 38

HardwareSerial LoRaSerial(1);

// ============================================================
// PINES ESP32-S3 HACIA SABERTOOTH
// ============================================================
#define SABERTOOTH_LEFT_PIN   10   // S1
#define SABERTOOTH_RIGHT_PIN  11   // S2

// ============================================================
// RANGO DEL JOYSTICK
// ============================================================
const int VALOR_MIN_CONTROL = -400;
const int VALOR_MAX_CONTROL = 400;

// Sube si tu joystick tiene drift
const int ZONA_MUERTA = 60;

// ============================================================
// PULSOS RC PARA SABERTOOTH
// ============================================================
const int PULSO_REVERSA = 1000;
const int PULSO_NEUTRO  = 1500;
const int PULSO_AVANCE  = 2000;

// Para prueba déjalo en 1.00
const float LIMITE_VELOCIDAD = 1.00;

// ============================================================
// TIEMPOS
// ============================================================
const unsigned long PERIODO_RC_US = 20000;   // 50 Hz
const unsigned long TIMEOUT_LORA_MS = 3000;  // 3 segundos

// ============================================================
// VARIABLES DE PULSO RC
// ============================================================
volatile int pulsoIzquierdo = PULSO_NEUTRO;
volatile int pulsoDerecho   = PULSO_NEUTRO;

unsigned long inicioFrameRC = 0;
bool frameActivo = false;

// ============================================================
// VARIABLES LORA
// ============================================================
String bufferLoRa = "";
unsigned long ultimoComandoLoRa = 0;

int ultimoRX = 0;
int ultimoRY = 0;

unsigned long ultimoDebug = 0;

// ============================================================
// PROTOTIPOS
// ============================================================
void actualizarPulsosRC();
void leerLoRa();
void procesarMensaje(String mensaje);
void controlarSabertooth(int rx, int ry);
void detenerMotores();

int aplicarZonaMuerta(int valor);
int limitarControl(int valor);

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(3000);

  pinMode(SABERTOOTH_LEFT_PIN, OUTPUT);
  pinMode(SABERTOOTH_RIGHT_PIN, OUTPUT);

  digitalWrite(SABERTOOTH_LEFT_PIN, LOW);
  digitalWrite(SABERTOOTH_RIGHT_PIN, LOW);

  LoRaSerial.begin(9600, SERIAL_8N1, LORA_RX, LORA_TX);

  detenerMotores();

  Serial.println();
  Serial.println("=====================================");
  Serial.println("ESP32-S3 + LoRa + Sabertooth");
  Serial.println("Pulso RC manual no bloqueante");
  Serial.println("Dato 15 = RX, dato 16 = RY");
  Serial.println("=====================================");

  Serial.println("Neutro inicial 5 segundos...");

  unsigned long inicio = millis();

  while (millis() - inicio < 5000) {
    actualizarPulsosRC();
  }

  ultimoComandoLoRa = millis();

  Serial.println("Listo. Esperando LoRa...");
}

// ============================================================
// LOOP
// ============================================================
void loop() {
  // Esta funcion debe ejecutarse siempre y lo mas rapido posible
  actualizarPulsosRC();

  // Leer LoRa sin detener los pulsos RC
  leerLoRa();

  // Seguridad: si se pierde LoRa, detener motores
  if (millis() - ultimoComandoLoRa > TIMEOUT_LORA_MS) {
    detenerMotores();
  }

  // Debug ligero cada 500 ms
  if (millis() - ultimoDebug > 500) {
    ultimoDebug = millis();

    Serial.print("RX: ");
    Serial.print(ultimoRX);

    Serial.print(" | RY: ");
    Serial.print(ultimoRY);

    Serial.print(" | Pulso I: ");
    Serial.print(pulsoIzquierdo);

    Serial.print(" | Pulso D: ");
    Serial.println(pulsoDerecho);
  }
}

// ============================================================
// GENERADOR RC NO BLOQUEANTE
// ============================================================
void actualizarPulsosRC() {
  unsigned long ahora = micros();

  // Iniciar nuevo frame cada 20 ms
  if (!frameActivo || (ahora - inicioFrameRC >= PERIODO_RC_US)) {
    inicioFrameRC = ahora;
    frameActivo = true;

    digitalWrite(SABERTOOTH_LEFT_PIN, HIGH);
    digitalWrite(SABERTOOTH_RIGHT_PIN, HIGH);
  }

  // Apagar canal izquierdo cuando ya cumplio su ancho de pulso
  if ((ahora - inicioFrameRC) >= (unsigned long)pulsoIzquierdo) {
    digitalWrite(SABERTOOTH_LEFT_PIN, LOW);
  }

  // Apagar canal derecho cuando ya cumplio su ancho de pulso
  if ((ahora - inicioFrameRC) >= (unsigned long)pulsoDerecho) {
    digitalWrite(SABERTOOTH_RIGHT_PIN, LOW);
  }
}

// ============================================================
// LEER LORA
// ============================================================
void leerLoRa() {
  while (LoRaSerial.available()) {
    char c = LoRaSerial.read();

    if (c == '\n') {
      bufferLoRa.trim();

      if (bufferLoRa.length() > 0) {
        procesarMensaje(bufferLoRa);
      }

      bufferLoRa = "";
    } 
    else {
      bufferLoRa += c;

      // Proteccion por si llega basura o no llega salto de linea
      if (bufferLoRa.length() > 250) {
        bufferLoRa = "";
      }
    }
  }
}

// ============================================================
// PROCESAR MENSAJE LORA
//
// Formato esperado:
// CTRL,contador,controlConectado,a,b,x,y,l1,r1,dpad,buttons,misc,
// axisLX,axisLY,axisRX,axisRY,brake,throttle,...
//
// Dato 15 contando desde 1 = datos[14] = RX
// Dato 16 contando desde 1 = datos[15] = RY
// ============================================================
void procesarMensaje(String mensaje) {
  String datos[30];

  int indice = 0;
  int inicio = 0;

  for (int i = 0; i < mensaje.length(); i++) {
    if (mensaje.charAt(i) == ',') {
      datos[indice] = mensaje.substring(inicio, i);
      inicio = i + 1;
      indice++;

      if (indice >= 29) {
        break;
      }
    }
  }

  datos[indice] = mensaje.substring(inicio);

  int totalDatos = indice + 1;

  if (totalDatos < 16) {
    return;
  }

  int rx = datos[14].toInt();
  int ry = datos[15].toInt();

  ultimoRX = rx;
  ultimoRY = ry;
  ultimoComandoLoRa = millis();

  controlarSabertooth(rx, ry);
}

// ============================================================
// CONTROL DIFERENCIAL
// ============================================================
void controlarSabertooth(int rx, int ry) {
  rx = aplicarZonaMuerta(rx);
  ry = aplicarZonaMuerta(ry);

  rx = limitarControl(rx);
  ry = limitarControl(ry);

  // Si adelante sale al reves, cambia ry por -ry
  int avance = ry;

  // Si gira al lado contrario, cambia rx por -rx
  int giro = rx;

  int velocidadIzquierda = avance + giro;
  int velocidadDerecha   = avance - giro;

  velocidadIzquierda = limitarControl(velocidadIzquierda);
  velocidadDerecha   = limitarControl(velocidadDerecha);

  velocidadIzquierda = velocidadIzquierda * LIMITE_VELOCIDAD;
  velocidadDerecha   = velocidadDerecha * LIMITE_VELOCIDAD;

  int nuevoPulsoIzquierdo = map(
    velocidadIzquierda,
    VALOR_MIN_CONTROL,
    VALOR_MAX_CONTROL,
    PULSO_REVERSA,
    PULSO_AVANCE
  );

  int nuevoPulsoDerecho = map(
    velocidadDerecha,
    VALOR_MIN_CONTROL,
    VALOR_MAX_CONTROL,
    PULSO_REVERSA,
    PULSO_AVANCE
  );

  nuevoPulsoIzquierdo = constrain(nuevoPulsoIzquierdo, PULSO_REVERSA, PULSO_AVANCE);
  nuevoPulsoDerecho   = constrain(nuevoPulsoDerecho, PULSO_REVERSA, PULSO_AVANCE);

  pulsoIzquierdo = nuevoPulsoIzquierdo;
  pulsoDerecho   = nuevoPulsoDerecho;
}

// ============================================================
// DETENER MOTORES
// ============================================================
void detenerMotores() {
  pulsoIzquierdo = PULSO_NEUTRO;
  pulsoDerecho   = PULSO_NEUTRO;
}

// ============================================================
// ZONA MUERTA
// ============================================================
int aplicarZonaMuerta(int valor) {
  if (abs(valor) < ZONA_MUERTA) {
    return 0;
  }

  return valor;
}

// ============================================================
// LIMITAR RANGO
// ============================================================
int limitarControl(int valor) {
  if (valor > VALOR_MAX_CONTROL) {
    valor = VALOR_MAX_CONTROL;
  }

  if (valor < VALOR_MIN_CONTROL) {
    valor = VALOR_MIN_CONTROL;
  }

  return valor;
}