#include <SPI.h>
#include <FS.h>
#include <SD.h>

#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();

#include <JPEGDecoder.h>

// ============================================================
// PINES
// ============================================================
// Ajusta estos pines si tu módulo usa otros
#define SD_CS    5
#define TFT_CS   15
#define TOUCH_CS 21

// ============================================================
// CONFIGURACIÓN GENERAL
// ============================================================
const char* imagenFondo = "/Menu estacion de control.jpg";

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(500);

  // Evita conflictos en el bus SPI
  pinMode(TOUCH_CS, OUTPUT);
  pinMode(TFT_CS, OUTPUT);
  pinMode(SD_CS, OUTPUT);

  digitalWrite(TOUCH_CS, HIGH);
  digitalWrite(TFT_CS, HIGH);
  digitalWrite(SD_CS, HIGH);

  // Inicializar pantalla
  tft.begin();
  tft.setRotation(1);   // 0 = vertical 240x320 normalmente
  tft.fillScreen(TFT_BLACK);

  Serial.println("Inicializando SD...");

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
  //escribirTexto();
}

// ============================================================
// LOOP
// ============================================================
void loop() {
  // Aquí puedes actualizar texto si quieres
}

// ============================================================
// MOSTRAR IMAGEN DE FONDO
// ============================================================
void mostrarFondo() {
  tft.fillScreen(TFT_BLACK);

  // Dibuja la imagen desde la esquina superior izquierda
  drawSdJpeg(imagenFondo, 0, 0);
}

// ============================================================
// ESCRIBIR TEXTO SOBRE LA IMAGEN
// ============================================================
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

// ============================================================
// DIBUJAR JPG DESDE SD
// ============================================================
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

// ============================================================
// RENDERIZAR JPG EN TFT
// ============================================================
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
    } 
    else if ((mcu_y + win_h) >= tft.height()) {
      JpegDec.abort();
    }
  }

  tft.setSwapBytes(swapBytes);

}


