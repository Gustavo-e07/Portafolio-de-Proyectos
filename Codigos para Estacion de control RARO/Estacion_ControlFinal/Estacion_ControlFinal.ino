// ============================================================
// Librerias
// ============================================================
#include <Arduino.h>
#include <TinyGPSPlus.h>
#include <SPI.h>
#include <FS.h>
#include <SD.h>
#include <TFT_eSPI.h>
#include <JPEGDecoder.h>
#include <Bluepad32.h>


// ============================================================
// PINES
// ============================================================
// Pantalla TFT
#define SD_CS 5
#define TFT_CS 15
#define TOUCH_CS 21

// LEDs Indicadores
#define LED_CtrlDESC 32
#define LED_CtrlCONECT 33
#define LED_LoRaDESC 12
#define LED_LoRaCONECT 13

//Modulo GPS
#define GPS_RX 26
#define GPS_TX 25

//Modulo LoRa
#define RS485_RX 27
#define RS485_TX 14

// ============================================================
// CONFIGURACIÓN GENERAL
// ============================================================
// Pantalla TFT
const char *imagenFondo = "/Menu estacion de control.jpg";
TFT_eSPI tft = TFT_eSPI();

//Control bluetooth
ControllerPtr myController = nullptr;

//Modulo GPS
TinyGPSPlus gps;
HardwareSerial GPSserial(2);

//Modulo Lora
HardwareSerial RT88H01(1);

//Otros
unsigned long contador = 0;
unsigned long tiempoAnterior = 0;


// ============================================================
// FUNCIONES
// ============================================================
// MOSTRAR IMAGEN DE FONDO
void mostrarFondo() {
  tft.fillScreen(TFT_BLACK);

  // Dibuja la imagen desde la esquina superior izquierda
  drawSdJpeg(imagenFondo, 0, 0);
}

// ESCRIBIR TEXTO SOBRE LA IMAGEN
void escribirTexto() {
  // Fondo transparente para el texto
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);

  tft.setCursor(15, 20);
  tft.println("Estacion de");
  tft.setCursor(15, 45);
  tft.println("control RARO");

  // Texto con fondo de color
  tft.setTextColor(TFT_BLACK, TFT_YELLOW);
  tft.setCursor(15, 90);
  tft.println("Sistema listo");

  // Texto más abajo
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(15, 130);
  tft.println("Modo: Manual");

  tft.setTextColor(TFT_GREEN);
  tft.setCursor(15, 160);
  tft.println("Conexion: OK");
}

// DIBUJAR JPG DESDE SD
void drawSdJpeg(const char *filename, int xpos, int ypos) {
  File jpegFile = SD.open(filename, FILE_READ);

  if (!jpegFile) {
    Serial.print("ERROR: Archivo no encontrado: ");
    Serial.println(filename);

    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, 10);
    tft.println("Imagen no");
    tft.setCursor(10, 35);
    tft.println("encontrada");
    return;
  }
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("Dibujando Imagen...");
  tft.setCursor(10, 35);
  tft.println(filename);

  bool decoded = JpegDec.decodeSdFile(jpegFile);

  if (decoded) {
    jpegRender(xpos, ypos);
  } else {
    Serial.println("Formato JPG no soportado");

    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, 10);
    tft.println("JPG no");
    tft.setCursor(10, 35);
    tft.println("soportado");
  }
}

// RENDERIZAR JPG EN TFT
void jpegRender(int xpos, int ypos) {
  uint16_t *pImg;

  uint16_t mcu_w = JpegDec.MCUWidth;
  uint16_t mcu_h = JpegDec.MCUHeight;

  uint32_t max_x = JpegDec.width;
  uint32_t max_y = JpegDec.height;

  bool swapBytes = tft.getSwapBytes();
  tft.setSwapBytes(true);

  uint32_t min_w = jpg_min(mcu_w, max_x % mcu_w);
  uint32_t min_h = jpg_min(mcu_h, max_y % mcu_h);

  uint32_t win_w = mcu_w;
  uint32_t win_h = mcu_h;

  max_x += xpos;
  max_y += ypos;

  while (JpegDec.read()) {
    pImg = JpegDec.pImage;

    int mcu_x = JpegDec.MCUx * mcu_w + xpos;
    int mcu_y = JpegDec.MCUy * mcu_h + ypos;

    if (mcu_x + mcu_w <= max_x) {
      win_w = mcu_w;
    } else {
      win_w = min_w;
    }

    if (mcu_y + mcu_h <= max_y) {
      win_h = mcu_h;
    } else {
      win_h = min_h;
    }

    if (win_w != mcu_w) {
      uint16_t *cImg;
      int p = 0;
      cImg = pImg + win_w;

      for (int h = 1; h < win_h; h++) {
        p += mcu_w;

        for (int w = 0; w < win_w; w++) {
          *cImg = *(pImg + w + p);
          cImg++;
        }
      }
    }

    if ((mcu_x + win_w) <= tft.width() && (mcu_y + win_h) <= tft.height()) {
      tft.pushImage(mcu_x, mcu_y, win_w, win_h, pImg);
    } else if ((mcu_y + win_h) >= tft.height()) {
      JpegDec.abort();
    }
  }

  tft.setSwapBytes(swapBytes);
}

// CALLBACK: CONTROL CONECTADO
void onConnectedController(ControllerPtr ctl) {
  if (myController == nullptr) {
    myController = ctl;
  }
}

// CALLBACK: CONTROL DESCONECTADO
void onDisconnectedController(ControllerPtr ctl) {
  if (myController == ctl) {
    myController = nullptr;
  }
}

// funciones dualsense
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
      0,     // retardo en ms
      300,   // duracion en ms
      0x80,  // motor suave
      0x80   // motor fuerte
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

// ============================================================
// SETUP
// ============================================================
void setup() {

  Serial.begin(115200);
  delay(500);

  // **************************************************
  // Declaracion de Puertos UART
  // **************************************************
  GPSserial.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);
  RT88H01.begin(9600, SERIAL_8N1, RS485_RX, RS485_TX);

  // **************************************************
  // Ajuste de Pines
  // **************************************************
  //Pantalla TFT
  pinMode(TOUCH_CS, OUTPUT);
  pinMode(TFT_CS, OUTPUT);
  pinMode(SD_CS, OUTPUT);

  //LEDs Indicadores
  pinMode(LED_CtrlDESC, OUTPUT);
  pinMode(LED_CtrlCONECT, OUTPUT);
  pinMode(LED_LoRaDESC, OUTPUT);
  pinMode(LED_LoRaCONECT, OUTPUT);

  // **************************************************
  // Inicializacion de Pines
  // **************************************************
  //Pantalla TFT
  digitalWrite(TOUCH_CS, HIGH);
  digitalWrite(TFT_CS, HIGH);
  digitalWrite(SD_CS, HIGH);

  //LEDs Indicadores
  digitalWrite(LED_CtrlDESC, HIGH);
  digitalWrite(LED_CtrlCONECT, LOW);
  digitalWrite(LED_LoRaDESC, HIGH);
  digitalWrite(LED_LoRaCONECT, LOW);

  // Inicializar pantalla
  tft.begin();
  tft.setRotation(1);  // 0 = vertical 240x320 normalmente
  tft.fillScreen(TFT_BLACK);

  // Inicializar SD usando la misma instancia SPI de TFT_eSPI
  if (!SD.begin(SD_CS, tft.getSPIinstance())) {
    Serial.println("Error: no se pudo montar la SD");
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, 10);
    tft.println("SD ERROR");
    return;
  }

  Serial.println("SD detectada correctamente");

  // Mostrar imagen de fondo
  mostrarFondo();

  // Escribir texto encima
  escribirTexto();

  //Codigo de control
  BP32.setup(&onConnectedController, &onDisconnectedController);

  // No lo actives siempre, porque borra el emparejamiento Bluetooth.
  // Solo úsalo si quieres resetear los controles vinculados.
  //BP32.forgetBluetoothKeys();

  BP32.enableVirtualDevice(false);
}

// ============================================================
// LOOP
// ============================================================
void loop() {
  BP32.update();

  // ============================================================
  // LEER GPS SIEMPRE
  // ============================================================
  while (GPSserial.available() > 0) {
    char c = GPSserial.read();

    // Si quieres ver datos crudos NMEA, descomenta:
    // Serial.write(c);

    gps.encode(c);
  }

  // ============================================================
  // VERIFICAR CONTROL
  // ============================================================
  bool controlConectado = false;

  if (myController != nullptr && myController->isConnected() && myController->isGamepad()) {
    controlConectado = true;
  }

  // ============================================================
  // ENVIAR DATOS CADA 1 SEGUNDO
  // ============================================================
  if (millis() - tiempoAnterior >= 300) {
    tiempoAnterior = millis();
    contador++;

    // ============================================================
    // VARIABLES DEL CONTROL
    // Si no hay control, se quedan en 0
    // ============================================================
    int a = 0;
    int b = 0;
    int x = 0;
    int y = 0;
    int l1 = 0;
    int r1 = 0;
    int dpad = 0;
    int buttons = 0;
    int misc = 0;
    int axisLX = 0;
    int axisLY = 0;
    int axisRX = 0;
    int axisRY = 0;
    int brake = 0;
    int throttle = 0;

    if (controlConectado) {
      accionesDualSense(myController);

      a = myController->a();
      b = myController->b();
      x = myController->x();
      y = myController->y();
      l1 = myController->l1();
      r1 = myController->r1();
      dpad = myController->dpad();
      buttons = myController->buttons();
      misc = myController->miscButtons();
      axisLX = myController->axisX();
      axisLY = myController->axisY();
      axisRX = myController->axisRX();
      axisRY = myController->axisRY();
      brake = myController->brake();
      throttle = myController->throttle();

      digitalWrite(LED_CtrlCONECT, HIGH);
      digitalWrite(LED_CtrlDESC, LOW);
    } else {
      digitalWrite(LED_CtrlCONECT, LOW);
      digitalWrite(LED_CtrlDESC, HIGH);
    }

    // ============================================================
    // VARIABLES DEL GPS
    // Si no son válidas, se manda "xxxx"
    // ============================================================
    String latitud = "xxxx";
    String longitud = "xxxx";
    String satelites = "xxxx";
    String altitud = "xxxx";
    String velocidadGPS = "xxxx";

    if (gps.location.isValid()) {
      latitud = String(gps.location.lat(), 6);
      longitud = String(gps.location.lng(), 6);
    }

    if (gps.satellites.isValid()) {
      satelites = String(gps.satellites.value());
    }

    if (gps.altitude.isValid()) {
      altitud = String(gps.altitude.meters(), 2);
    }

    if (gps.speed.isValid()) {
      velocidadGPS = String(gps.speed.kmph(), 2);
    }

    // ============================================================
    // ARMAR MENSAJE CSV
    // ============================================================
    String mensaje;

    mensaje += "EstacionControl,";
    mensaje += String(contador);
    mensaje += ",";
    mensaje += String(controlConectado);
    mensaje += ",";
    mensaje += String(a);
    mensaje += ",";
    mensaje += String(b);
    mensaje += ",";
    mensaje += String(x);
    mensaje += ",";
    mensaje += String(y);
    mensaje += ",";
    mensaje += String(l1);
    mensaje += ",";
    mensaje += String(r1);
    mensaje += ",";
    mensaje += String(dpad);
    mensaje += ",";
    mensaje += String(buttons);
    mensaje += ",";
    mensaje += String(misc);
    mensaje += ",";
    mensaje += String(axisLX);
    mensaje += ",";
    mensaje += String(axisLY);
    mensaje += ",";
    mensaje += String(axisRX);
    mensaje += ",";
    mensaje += String(axisRY);
    mensaje += ",";
    mensaje += String(brake);
    mensaje += ",";
    mensaje += String(throttle);
    mensaje += ",";
    mensaje += latitud;
    mensaje += ",";
    mensaje += longitud;
    mensaje += ",";
    mensaje += altitud;
    mensaje += ",";
    mensaje += satelites;
    mensaje += ",";
    mensaje += velocidadGPS;

    // ============================================================
    // ENVIAR POR RT88H01 Y MOSTRAR EN MONITOR SERIAL
    // ============================================================
    RT88H01.println(mensaje);

    Serial.print("Enviado: ");
    Serial.println(mensaje);
  }

  delay(5);
}
