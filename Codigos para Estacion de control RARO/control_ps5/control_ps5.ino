#include <Bluepad32.h>

ControllerPtr myController = nullptr;

// ============================================================
// PINES LED
// ============================================================
#define LED_DESCONECTADO  32
#define LED_CONECTADO     33

// ============================================================
// CALLBACK: CONTROL CONECTADO
// ============================================================
void onConnectedController(ControllerPtr ctl) {
  if (myController == nullptr) {
    myController = ctl;
  }
}

// ============================================================
// CALLBACK: CONTROL DESCONECTADO
// ============================================================
void onDisconnectedController(ControllerPtr ctl) {
  if (myController == ctl) {
    myController = nullptr;
  }
}

//=======================================================
// funciones dualsense
//=========================================================
void accionesDualSense(ControllerPtr ctl) {
  // Boton A: color rojo
  if (ctl->a()) {
    ctl->setColorLED(255, 0, 0);
  }

  // Boton B: color verde
  if (ctl->b()) {
    ctl->setColorLED(0, 255, 0);
  }

  // Boton X: color azul
  if (ctl->x()) {
    ctl->setColorLED(0, 0, 255);
  }

  // Boton Y: vibracion
  if (ctl->y()) {
    ctl->playDualRumble(
      0,      // retardo en ms
      300,    // duracion en ms
      0x80,   // motor suave
      0x80    // motor fuerte
    );
  }

  // L1: prender una luz blanca
  if (ctl->l1()) {
    ctl->setPlayerLEDs(0x01);
  }

  // R1: prender todas las luces blancas
  if (ctl->r1()) {
    ctl->setPlayerLEDs(0x0F);
  }

  // L2: apagar luces blancas
  if (ctl->brake() > 500) {
    ctl->setPlayerLEDs(0x00);
  }
}
// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(LED_DESCONECTADO, OUTPUT);
  pinMode(LED_CONECTADO, OUTPUT);

  digitalWrite(LED_DESCONECTADO, HIGH);
  digitalWrite(LED_CONECTADO, LOW);

  BP32.setup(&onConnectedController, &onDisconnectedController);

  // No lo actives siempre, porque borra el emparejamiento Bluetooth.
  // Solo úsalo si quieres resetear los controles vinculados.
  // BP32.forgetBluetoothKeys();

  BP32.enableVirtualDevice(false);
}

// ============================================================
// LOOP
// ============================================================
void loop() {
  BP32.update();

  bool conectado = false;

  if (myController != nullptr && myController->isConnected()) {
    conectado = true;
  }

  if (conectado) {
    digitalWrite(LED_CONECTADO, HIGH);
    digitalWrite(LED_DESCONECTADO, LOW);
  } else {
    digitalWrite(LED_CONECTADO, LOW);
    digitalWrite(LED_DESCONECTADO, HIGH);
  }

  if (conectado && myController->hasData() && myController->isGamepad()) {
    accionesDualSense(myController);

    Serial.print(conectado);
    Serial.print(",");
    Serial.print(myController->a());
    Serial.print(",");
    Serial.print(myController->b());
    Serial.print(",");
    Serial.print(myController->x());
    Serial.print(",");
    Serial.print(myController->y());
    Serial.print(",");
    Serial.print(myController->l1());
    Serial.print(",");
    Serial.print(myController->r1());
    Serial.print(",");
    Serial.print(myController->dpad());
    Serial.print(",");
    Serial.print(myController->buttons());
    Serial.print(",");
    Serial.print(myController->miscButtons());
    Serial.print(",");
    Serial.print(myController->axisX());
    Serial.print(",");
    Serial.print(myController->axisY());
    Serial.print(",");
    Serial.print(myController->axisRX());
    Serial.print(",");
    Serial.print(myController->axisRY());
    Serial.print(",");
    Serial.print(myController->brake());
    Serial.print(",");
    Serial.println(myController->throttle());
  } else {
    Serial.println("0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0");
  }

  delay(200);
}
/*
Para las luces blancas de abajo del DualSense:

ctl->setPlayerLEDs(0x01);  // una luz
ctl->setPlayerLEDs(0x03);  // dos luces
ctl->setPlayerLEDs(0x07);  // tres luces
ctl->setPlayerLEDs(0x0F);  // cuatro luces
ctl->setPlayerLEDs(0x00);  // apagar

Para la vibración:

ctl->playDualRumble(0, 300, 0x80, 0x80);

Puedes subir o bajar la fuerza cambiando los últimos dos valores:

0x20  // suave
0x80  // media
0xFF  // fuerte
*/