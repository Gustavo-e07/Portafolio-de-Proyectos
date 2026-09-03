/*
 ============================================================
   ESP32 + SIM808 GPS + MPU9250/MPU6500 + TEMPERATURA
   + SABERTOOTH + 2 SERVOS
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

 0 - 180 grados


 ENVÍA POR SERIAL:

 latitud,
 longitud,
 altitud,
 satelites,
 velocidad,
 temperatura,
 ax,
 ay,
 az,
 gx,
 gy,
 gz,
 mx,
 my,
 mz

 Total: 15 valores
 ============================================================
*/


#include <TinyGPSPlus.h>
#include <Wire.h>
#include <MPU9250_asukiaaa.h>
#include <ESP32Servo.h>


// ============================================================
// PINES
// ============================================================

// SIM808 GPS
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

// MPU9250 I2C
#define SDA_IMU 4
#define SCL_IMU 3


// ============================================================
// OBJETOS
// ============================================================

// GPS
TinyGPSPlus gps;

// UART 2 para SIM808
HardwareSerial GPSserial(2);


// MPU9250
MPU9250_asukiaaa imu;


// Sabertooth
Servo MotorIzq;
Servo MotorDer;


// Servos
Servo Servo1;
Servo Servo2;


// ============================================================
// VARIABLES DE MOTORES
// ============================================================

int valorMotorIzq = 1500;
int valorMotorDer = 1500;


// ============================================================
// VARIABLES SERVOS
// ============================================================

int valorServo1 = 90;
int valorServo2 = 90;


// ============================================================
// BOTÓN
// ============================================================

int valorBoton = 0;


// ============================================================
// SERIAL
// ============================================================

String tramaEntrada = "";


// ============================================================
// TEMPORIZACIÓN
// ============================================================

unsigned long tiempoAnteriorEnvio = 0;

unsigned long ultimoComandoMotores = 0;


// Envío de sensores:
// 200 ms = 5 Hz

const unsigned long INTERVALO_ENVIO = 200;


// Watchdog motores

const unsigned long TIMEOUT_MOTORES = 500;


// ============================================================
// SETUP
// ============================================================

void setup()
{
    // ========================================================
    // SERIAL PRINCIPAL
    // Comunicación con ROS 2
    // ========================================================

    Serial.begin(115200);


    // ========================================================
    // SIM808 GPS
    // ========================================================

    GPSserial.begin(
        9600,
        SERIAL_8N1,
        GPS_RX,
        GPS_TX
    );


    // ========================================================
    // SENSOR ANALÓGICO TEMPERATURA
    // ========================================================

    pinMode(
        TEMP_PIN,
        INPUT
    );


    analogReadResolution(12);


    // ========================================================
    // I2C MPU9250
    // ========================================================

    Wire.begin(
        SDA_IMU,
        SCL_IMU
    );


    imu.setWire(
        &Wire
    );


    // --------------------------------------------------------
    // Inicializar acelerómetro
    // --------------------------------------------------------

    imu.beginAccel();


    // --------------------------------------------------------
    // Inicializar giroscopio
    // --------------------------------------------------------

    imu.beginGyro();


    // --------------------------------------------------------
    // Inicializar magnetómetro
    // --------------------------------------------------------

    imu.beginMag();


    // ========================================================
    // PWM
    // ========================================================

    MotorIzq.setPeriodHertz(50);
    MotorDer.setPeriodHertz(50);

    Servo1.setPeriodHertz(50);
    Servo2.setPeriodHertz(50);


    // ========================================================
    // SABERTOOTH
    // ========================================================

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


    // ========================================================
    // SERVOS
    // ========================================================

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


    // ========================================================
    // ESTADO INICIAL
    // ========================================================

    // Motores detenidos

    MotorIzq.writeMicroseconds(
        1500
    );

    MotorDer.writeMicroseconds(
        1500
    );


    // Servos centrados

    Servo1.write(
        90
    );

    Servo2.write(
        90
    );


    ultimoComandoMotores =
        millis();


    delay(1000);
}


// ============================================================
// LOOP
// ============================================================

void loop()
{
    // Leer continuamente GPS
    leerGPS();


    // Recibir comandos desde ROS 2
    leerComandoSerial();


    // Watchdog motores
    verificarTimeoutMotores();


    // Enviar sensores
    if (
        millis() - tiempoAnteriorEnvio
        >= INTERVALO_ENVIO
    )
    {
        tiempoAnteriorEnvio =
            millis();

        enviarSensores();
    }
}


// ============================================================
// LEER GPS SIM808
// ============================================================

void leerGPS()
{
    while (
        GPSserial.available()
    )
    {
        char c =
            GPSserial.read();


        gps.encode(
            c
        );
    }
}


// ============================================================
// LEER SERIAL DESDE ROS 2
// ============================================================

void leerComandoSerial()
{
    while (
        Serial.available()
    )
    {
        char c =
            Serial.read();


        // ----------------------------------------------------
        // Fin de trama
        // ----------------------------------------------------

        if (c == '\n')
        {
            if (
                tramaEntrada.length() > 0
            )
            {
                procesarComando(
                    tramaEntrada
                );
            }


            tramaEntrada = "";
        }


        // ----------------------------------------------------
        // Ignorar retorno de carro
        // ----------------------------------------------------

        else if (c != '\r')
        {
            tramaEntrada += c;


            // Protección contra basura serial

            if (
                tramaEntrada.length() > 100
            )
            {
                tramaEntrada = "";
            }
        }
    }
}


// ============================================================
// PROCESAR COMANDO
// ============================================================

void procesarComando(
    String mensaje
)
{
    /*
       Trama:

       MotorIzq,MotorDer,Servo1,Servo2,Boton


       Ejemplo:

       1500,1500,90,120,0
    */


    // ========================================================
    // BUSCAR COMAS
    // ========================================================

    int coma1 =
        mensaje.indexOf(',');


    int coma2 =
        mensaje.indexOf(
            ',',
            coma1 + 1
        );


    int coma3 =
        mensaje.indexOf(
            ',',
            coma2 + 1
        );


    int coma4 =
        mensaje.indexOf(
            ',',
            coma3 + 1
        );


    // ========================================================
    // VALIDAR TRAMA
    // ========================================================

    if (
        coma1 == -1 ||
        coma2 == -1 ||
        coma3 == -1 ||
        coma4 == -1
    )
    {
        return;
    }


    // ========================================================
    // EXTRAER DATOS
    // ========================================================

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


    int nuevoBoton =
        mensaje.substring(
            coma4 + 1
        ).toInt();


    // ========================================================
    // MOTOR IZQUIERDO
    // ========================================================

    if (
        nuevoMotorIzq >= 1000 &&
        nuevoMotorIzq <= 2000
    )
    {
        valorMotorIzq =
            nuevoMotorIzq;


        MotorIzq.writeMicroseconds(
            valorMotorIzq
        );
    }


    // ========================================================
    // MOTOR DERECHO
    // ========================================================

    if (
        nuevoMotorDer >= 1000 &&
        nuevoMotorDer <= 2000
    )
    {
        valorMotorDer =
            nuevoMotorDer;


        MotorDer.writeMicroseconds(
            valorMotorDer
        );
    }


    // ========================================================
    // WATCHDOG
    // ========================================================

    if (
        nuevoMotorIzq >= 1000 &&
        nuevoMotorIzq <= 2000 &&
        nuevoMotorDer >= 1000 &&
        nuevoMotorDer <= 2000
    )
    {
        ultimoComandoMotores =
            millis();
    }


    // ========================================================
    // SERVO 1
    // ========================================================

    if (
        nuevoServo1 >= 0 &&
        nuevoServo1 <= 180
    )
    {
        valorServo1 =
            nuevoServo1;


        Servo1.write(
            valorServo1
        );
    }


    // ========================================================
    // SERVO 2
    // ========================================================

    if (
        nuevoServo2 >= 0 &&
        nuevoServo2 <= 180
    )
    {
        valorServo2 =
            nuevoServo2;


        Servo2.write(
            valorServo2
        );
    }


    // ========================================================
    // BOTÓN
    // ========================================================

    if (
        nuevoBoton == 0 ||
        nuevoBoton == 1
    )
    {
        valorBoton =
            nuevoBoton;
    }
}


// ============================================================
// WATCHDOG SABERTOOTH
// ============================================================

void verificarTimeoutMotores()
{
    if (
        millis() - ultimoComandoMotores
        > TIMEOUT_MOTORES
    )
    {
        valorMotorIzq =
            1500;

        valorMotorDer =
            1500;


        MotorIzq.writeMicroseconds(
            1500
        );


        MotorDer.writeMicroseconds(
            1500
        );
    }
}


// ============================================================
// TEMPERATURA ANALÓGICA
// ============================================================

float leerTemperatura()
{
    int adc =
        analogRead(
            TEMP_PIN
        );


    // ADC -> Voltaje

    float voltaje =
        adc * 3.3 / 4095.0;


    /*
       TEMPORALMENTE:

       Conversión tipo LM35:

       10 mV / °C

       Si el sensor es otro,
       cambiaremos esta ecuación.
    */

    float temperatura =
        voltaje * 100.0;


    return temperatura;
}


// ============================================================
// LEER Y ENVIAR SENSORES
// ============================================================

void enviarSensores()
{
    // ========================================================
    // MPU9250
    // ========================================================

    // --------------------------------------------------------
    // Leer acelerómetro
    // --------------------------------------------------------

    imu.accelUpdate();


    // --------------------------------------------------------
    // Leer giroscopio
    // --------------------------------------------------------

    imu.gyroUpdate();


    // --------------------------------------------------------
    // Leer magnetómetro
    // --------------------------------------------------------

    imu.magUpdate();


    // ========================================================
    // ACELERACIÓN
    // ========================================================

    float ax =
        imu.accelX();

    float ay =
        imu.accelY();

    float az =
        imu.accelZ();


    // ========================================================
    // GIROSCOPIO
    // ========================================================

    float gx =
        imu.gyroX();

    float gy =
        imu.gyroY();

    float gz =
        imu.gyroZ();


    // ========================================================
    // MAGNETÓMETRO
    // ========================================================

    float mx =
        imu.magX();

    float my =
        imu.magY();

    float mz =
        imu.magZ();


    // ========================================================
    // TEMPERATURA
    // ========================================================

    float temperatura =
        leerTemperatura();


    // ========================================================
    // GPS
    // ========================================================

    double latitud =
        0.0;

    double longitud =
        0.0;


    float altitud =
        0.0;

    float velocidad =
        0.0;


    int satelites =
        0;


    // --------------------------------------------------------
    // Latitud y longitud
    // --------------------------------------------------------

    if (
        gps.location.isValid()
    )
    {
        latitud =
            gps.location.lat();


        longitud =
            gps.location.lng();
    }


    // --------------------------------------------------------
    // Altitud
    // --------------------------------------------------------

    if (
        gps.altitude.isValid()
    )
    {
        altitud =
            gps.altitude.meters();
    }


    // --------------------------------------------------------
    // Velocidad
    // --------------------------------------------------------

    if (
        gps.speed.isValid()
    )
    {
        velocidad =
            gps.speed.mps();
    }


    // --------------------------------------------------------
    // Satélites
    // --------------------------------------------------------

    if (
        gps.satellites.isValid()
    )
    {
        satelites =
            gps.satellites.value();
    }


    // ========================================================
    // TRAMA SERIAL
    // ========================================================

    /*
       ORDEN:

       1  Latitud
       2  Longitud
       3  Altitud
       4  Satélites
       5  Velocidad
       6  Temperatura

       7  AX
       8  AY
       9  AZ

       10 GX
       11 GY
       12 GZ

       13 MX
       14 MY
       15 MZ
    */


    // 1 Latitud
    Serial.print(
        latitud,
        7
    );

    Serial.print(",");


    // 2 Longitud
    Serial.print(
        longitud,
        7
    );

    Serial.print(",");


    // 3 Altitud
    Serial.print(
        altitud,
        2
    );

    Serial.print(",");


    // 4 Satélites
    Serial.print(
        satelites
    );

    Serial.print(",");


    // 5 Velocidad
    Serial.print(
        velocidad,
        2
    );

    Serial.print(",");


    // 6 Temperatura
    Serial.print(
        temperatura,
        2
    );

    Serial.print(",");


    // ========================================================
    // ACELERÓMETRO
    // ========================================================

    // 7 AX
    Serial.print(
        ax,
        3
    );

    Serial.print(",");


    // 8 AY
    Serial.print(
        ay,
        3
    );

    Serial.print(",");


    // 9 AZ
    Serial.print(
        az,
        3
    );

    Serial.print(",");


    // ========================================================
    // GIROSCOPIO
    // ========================================================

    // 10 GX
    Serial.print(
        gx,
        3
    );

    Serial.print(",");


    // 11 GY
    Serial.print(
        gy,
        3
    );

    Serial.print(",");


    // 12 GZ
    Serial.print(
        gz,
        3
    );

    Serial.print(",");


    // ========================================================
    // MAGNETÓMETRO
    // ========================================================

    // 13 MX
    Serial.print(
        mx,
        3
    );

    Serial.print(",");


    // 14 MY
    Serial.print(
        my,
        3
    );

    Serial.print(",");


    // 15 MZ
    Serial.println(
        mz,
        3
    );
}

