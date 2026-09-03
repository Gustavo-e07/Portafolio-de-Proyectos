
/*
 ============================================================
  GPS + TEMPERATURA + IMU + SABERTOOTH + 2 SERVOS
  Plataforma: ESP32
 ============================================================

 RECIBE POR SERIAL:

 MotorIzq,MotorDer,Servo1,Servo2,Boton

 Ejemplo:
 1500,1500,90,120,0


 MOTORES SABERTOOTH:
 1000 = reversa máxima
 1500 = detenido
 2000 = avance máximo


 SERVOS:
 0 a 180 grados


 BOTÓN:
 0 = servos siguen los valores recibidos
 1 = guardar posición actual de los servos
     y mantenerla fija


 ENVÍA POR SERIAL:

 latitud,longitud,altitud,satelites,velocidad,
 temperatura,ax,ay,az,gx,gy,gz
 ============================================================
*/

#include <TinyGPSPlus.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <ESP32Servo.h>


// ============================================================
// PINES
// ============================================================

// GPS
#define GPS_RX 8
#define GPS_TX 9

// Sensor analógico de temperatura
#define TEMP_PIN 7

// Sabertooth
#define MOTOR_IZQ_PIN 12
#define MOTOR_DER_PIN 13

// Servos
#define SERVO_1_PIN 5
#define SERVO_2_PIN 6

// IMU I2C
#define SDA_IMU 4
#define SCL_IMU 3


// ============================================================
// OBJETOS
// ============================================================

TinyGPSPlus gps;

HardwareSerial GPSserial(2);

Adafruit_MPU6050 mpu;

Servo MotorIzq;
Servo MotorDer;

Servo Servo1;
Servo Servo2;


// ============================================================
// MOTORES
// ============================================================

int valorMotorIzq = 1500;
int valorMotorDer = 1500;


// ============================================================
// SERVOS
// ============================================================

// Valores recibidos
int servo1Recibido = 90;
int servo2Recibido = 90;

// Posición actualmente aplicada
int posicionServo1 = 90;
int posicionServo2 = 90;

// Posición guardada cuando se presiona el botón
int posicionGuardadaServo1 = 90;
int posicionGuardadaServo2 = 90;

// Estado de bloqueo
bool servosBloqueados = false;

// Estado anterior del botón
int botonAnterior = 0;


// ============================================================
// SERIAL
// ============================================================

String tramaEntrada = "";


// ============================================================
// TEMPORIZACIÓN
// ============================================================

unsigned long tiempoAnteriorEnvio = 0;
unsigned long ultimoComandoMotores = 0;

const unsigned long INTERVALO_ENVIO = 200;

// Seguridad:
// detener motores si pasan 500 ms sin recibir comandos
const unsigned long TIMEOUT_MOTORES = 500;


// ============================================================
// SETUP
// ============================================================

void setup()
{
    // --------------------------------------------------------
    // Serial principal
    // --------------------------------------------------------

    Serial.begin(115200);


    // --------------------------------------------------------
    // GPS
    // --------------------------------------------------------

    GPSserial.begin(
        9600,
        SERIAL_8N1,
        GPS_RX,
        GPS_TX
    );


    // --------------------------------------------------------
    // Temperatura analógica
    // --------------------------------------------------------

    pinMode(TEMP_PIN, INPUT);

    analogReadResolution(12);


    // --------------------------------------------------------
    // IMU
    // --------------------------------------------------------

    Wire.begin(SDA_IMU, SCL_IMU);

    if (!mpu.begin())
    {
        Serial.println("ERROR_IMU");

        while (1)
        {
            delay(100);
        }
    }


    mpu.setAccelerometerRange(
        MPU6050_RANGE_8_G
    );

    mpu.setGyroRange(
        MPU6050_RANGE_500_DEG
    );

    mpu.setFilterBandwidth(
        MPU6050_BAND_21_HZ
    );


    // --------------------------------------------------------
    // Configuración PWM
    // --------------------------------------------------------

    MotorIzq.setPeriodHertz(50);
    MotorDer.setPeriodHertz(50);

    Servo1.setPeriodHertz(50);
    Servo2.setPeriodHertz(50);


    // --------------------------------------------------------
    // Sabertooth
    // --------------------------------------------------------

    MotorIzq.attach(
        MOTOR_IZQ_PIN,
        1000,
        2000
    );

    MotorDer.attach(
        MOTOR_DER_PIN,
        1000,
        2000
    );


    // --------------------------------------------------------
    // Servos
    // --------------------------------------------------------

    Servo1.attach(
        SERVO_1_PIN,
        500,
        2500
    );

    Servo2.attach(
        SERVO_2_PIN,
        500,
        2500
    );


    // --------------------------------------------------------
    // Estado inicial
    // --------------------------------------------------------

    // Motores detenidos
    MotorIzq.writeMicroseconds(1500);
    MotorDer.writeMicroseconds(1500);


    // Servos al centro
    Servo1.write(90);
    Servo2.write(90);


    ultimoComandoMotores = millis();


    delay(1000);
}


// ============================================================
// LOOP
// ============================================================

void loop()
{
    // Leer GPS continuamente
    leerGPS();


    // Recibir comandos
    leerComandoSerial();


    // Seguridad de los motores
    verificarTimeoutMotores();


    // Enviar sensores
    if (
        millis() - tiempoAnteriorEnvio
        >= INTERVALO_ENVIO
    )
    {
        tiempoAnteriorEnvio = millis();

        enviarSensores();
    }
}


// ============================================================
// GPS
// ============================================================

void leerGPS()
{
    while (GPSserial.available())
    {
        char c = GPSserial.read();

        gps.encode(c);
    }
}


// ============================================================
// RECIBIR SERIAL
// ============================================================

void leerComandoSerial()
{
    while (Serial.available())
    {
        char c = Serial.read();


        // Nueva línea = fin de trama
        if (c == '\n')
        {
            procesarComando(
                tramaEntrada
            );

            tramaEntrada = "";
        }

        // Ignorar retorno de carro
        else if (c != '\r')
        {
            tramaEntrada += c;
        }
    }
}


// ============================================================
// PROCESAR COMANDO
// ============================================================

void procesarComando(String mensaje)
{
    /*
     Trama esperada:

     MotorIzq,MotorDer,Servo1,Servo2,Boton

     Ejemplo:

     1500,1500,90,120,0
    */


    // --------------------------------------------------------
    // Buscar comas
    // --------------------------------------------------------

    int coma1 = mensaje.indexOf(',');

    int coma2 = mensaje.indexOf(
        ',',
        coma1 + 1
    );

    int coma3 = mensaje.indexOf(
        ',',
        coma2 + 1
    );

    int coma4 = mensaje.indexOf(
        ',',
        coma3 + 1
    );


    // --------------------------------------------------------
    // Comprobar trama
    // --------------------------------------------------------

    if (
        coma1 == -1 ||
        coma2 == -1 ||
        coma3 == -1 ||
        coma4 == -1
    )
    {
        return;
    }


    // --------------------------------------------------------
    // Extraer datos
    // --------------------------------------------------------

    int nuevoMotorIzq =
        mensaje.substring(
            0,
            coma1
        ).toInt();


    int nuevoMotorDer =
        mensaje.substring(
            coma1 + 1,
            coma2
        ).toInt();


    int nuevoServo1 =
        mensaje.substring(
            coma2 + 1,
            coma3
        ).toInt();


    int nuevoServo2 =
        mensaje.substring(
            coma3 + 1,
            coma4
        ).toInt();


    int boton =
        mensaje.substring(
            coma4 + 1
        ).toInt();


    // ========================================================
    // CONTROL DE MOTORES
    // ========================================================

    if (
        nuevoMotorIzq >= 1000 &&
        nuevoMotorIzq <= 2000 &&
        nuevoMotorDer >= 1000 &&
        nuevoMotorDer <= 2000
    )
    {
        valorMotorIzq = nuevoMotorIzq;
        valorMotorDer = nuevoMotorDer;


        MotorIzq.writeMicroseconds(
            valorMotorIzq
        );

        MotorDer.writeMicroseconds(
            valorMotorDer
        );


        // Reiniciar watchdog
        ultimoComandoMotores = millis();
    }


    // ========================================================
    // GUARDAR VALORES RECIBIDOS DE LOS SERVOS
    // ========================================================

    if (
        nuevoServo1 >= 0 &&
        nuevoServo1 <= 180
    )
    {
        servo1Recibido =
            nuevoServo1;
    }


    if (
        nuevoServo2 >= 0 &&
        nuevoServo2 <= 180
    )
    {
        servo2Recibido =
            nuevoServo2;
    }


    // ========================================================
    // DETECTAR CUANDO SE PRESIONA EL BOTÓN
    // ========================================================

    /*
       Solamente guardamos la posición cuando ocurre
       el cambio:

       0 -> 1

       Esto evita guardar repetidamente mientras
       el botón permanece presionado.
    */

    if (
        boton == 1 &&
        botonAnterior == 0
    )
    {
        // Guardar posición actual

        posicionGuardadaServo1 =
            posicionServo1;

        posicionGuardadaServo2 =
            posicionServo2;


        // Activar bloqueo

        servosBloqueados = true;
    }


    // ========================================================
    // LIBERAR SERVOS
    // ========================================================

    /*
       Cuando el botón vuelve a cero,
       los servos vuelven a seguir los
       valores recibidos.
    */

    if (boton == 0)
    {
        servosBloqueados = false;
    }


    // Guardar estado del botón
    botonAnterior = boton;


    // ========================================================
    // CONTROL DE SERVOS
    // ========================================================

    if (!servosBloqueados)
    {
        // ---------------------------------------------
        // Control normal
        // ---------------------------------------------

        posicionServo1 =
            servo1Recibido;

        posicionServo2 =
            servo2Recibido;


        Servo1.write(
            posicionServo1
        );

        Servo2.write(
            posicionServo2
        );
    }

    else
    {
        // ---------------------------------------------
        // Posición bloqueada
        // ---------------------------------------------

        posicionServo1 =
            posicionGuardadaServo1;

        posicionServo2 =
            posicionGuardadaServo2;


        Servo1.write(
            posicionGuardadaServo1
        );

        Servo2.write(
            posicionGuardadaServo2
        );
    }
}


// ============================================================
// SEGURIDAD MOTORES
// ============================================================

void verificarTimeoutMotores()
{
    if (
        millis() - ultimoComandoMotores
        > TIMEOUT_MOTORES
    )
    {
        valorMotorIzq = 1500;
        valorMotorDer = 1500;


        MotorIzq.writeMicroseconds(
            1500
        );

        MotorDer.writeMicroseconds(
            1500
        );
    }
}


// ============================================================
// TEMPERATURA
// ============================================================

float leerTemperatura()
{
    int adc =
        analogRead(TEMP_PIN);


    float voltaje =
        adc * 3.3 / 4095.0;


    /*
       Conversión de ejemplo para LM35:

       10 mV / °C
    */

    float temperatura =
        voltaje * 100.0;


    return temperatura;
}


// ============================================================
// ENVIAR SENSORES
// ============================================================

void enviarSensores()
{
    // --------------------------------------------------------
    // IMU
    // --------------------------------------------------------

    sensors_event_t a;
    sensors_event_t g;
    sensors_event_t temperaturaIMU;


    mpu.getEvent(
        &a,
        &g,
        &temperaturaIMU
    );


    // --------------------------------------------------------
    // Sensor temperatura
    // --------------------------------------------------------

    float temperatura =
        leerTemperatura();


    // --------------------------------------------------------
    // GPS
    // --------------------------------------------------------

    double latitud = 0;
    double longitud = 0;

    float altitud = 0;
    float velocidad = 0;

    int satelites = 0;


    if (gps.location.isValid())
    {
        latitud =
            gps.location.lat();

        longitud =
            gps.location.lng();
    }


    if (gps.altitude.isValid())
    {
        altitud =
            gps.altitude.meters();
    }


    if (gps.speed.isValid())
    {
        velocidad =
            gps.speed.mps();
    }


    if (gps.satellites.isValid())
    {
        satelites =
            gps.satellites.value();
    }


    // ========================================================
    // TRAMA DE SALIDA
    // ========================================================

    Serial.print(latitud, 7);
    Serial.print(",");

    Serial.print(longitud, 7);
    Serial.print(",");

    Serial.print(altitud, 2);
    Serial.print(",");

    Serial.print(satelites);
    Serial.print(",");

    Serial.print(velocidad, 2);
    Serial.print(",");

    Serial.print(temperatura, 2);
    Serial.print(",");


    // --------------------------------------------------------
    // Acelerómetro
    // --------------------------------------------------------

    Serial.print(
        a.acceleration.x,
        3
    );

    Serial.print(",");


    Serial.print(
        a.acceleration.y,
        3
    );

    Serial.print(",");


    Serial.print(
        a.acceleration.z,
        3
    );

    Serial.print(",");


    // --------------------------------------------------------
    // Giroscopio
    // --------------------------------------------------------

    Serial.print(
        g.gyro.x,
        3
    );

    Serial.print(",");


    Serial.print(
        g.gyro.y,
        3
    );

    Serial.print(",");


    Serial.println(
        g.gyro.z,
        3
    );
}

