#include <ps5Controller.h>

void setup() {
  Serial.begin(115200);
  // Inicia la conexión Bluetooth (el número indica el canal)
  ps5.begin(20); 
  Serial.println("Esperando a conectar el mando...");
}

void loop() {
  // Verifica si el mando está conectado
  if (ps5.isConnected()) {
    Serial.println("¡Mando conectado!");

    // Leer botones (ejemplo: botón X)
    if (ps5.cross) {
      Serial.println("Botón X presionado");
    }

    // Leer joysticks analógicos (valores de 0 a 255)
    int joystickX = ps5.ljoyX; 
    
    // Controlar vibración del motor izquierdo (0-255)
    ps5.setRumble(0, 255); 
  }
  delay(20); // Pausa recomendada para refresco
}
