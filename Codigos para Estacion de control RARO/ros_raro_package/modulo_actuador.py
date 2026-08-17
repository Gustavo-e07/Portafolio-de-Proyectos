import serial

import rclpy
from rclpy.node import Node

from std_msgs.msg import String


class ModuloActuador(Node):

    def __init__(self):

        super().__init__("modulo_actuador")

        # Puerto serial del chasis
        self.serial = serial.Serial(
            "/dev/ttyUSB0",
            115200,
            timeout=0.1
        )


        #
        #Publicaciones
        #++++++++++++++++
        self.pub_modo = self.create_publisher(
            String,
            "modo",
            10
    )

        self.pub_control_RARO = self.create_publisher(
            String,
            "control_RARO",
            10
        )

        self.pub_control_Gimball = self.create_publisher(
            String,
            "control_Gimball",
            10
        )
        self.pub_GPS_Estacion = self.create_subscription(
            String,
            "gps_estacion",
            10
        )
        

        self.pub_Sensando = self.create_subscription(
           String,
           "sensando",
           10
        )

        self.pub_NPK = self.create_subscription(
            String,
            "npk",
            10
        )
        self.pub_estado_muestra = self.create_subscription(
            String,
            "estado_muestra",
            10
        )

        #
        #Suscripciones
        #=============================
        self.sub_TempRobot = self.create_subscription(
            String,
            "temp_robot",
            self.callback_joystick,
            10
        )

        self.sub_UbiEstacion = self.create_subscription(
            String,
            "ubi_estacion",
            self.callback_joystick,
            10
        )

        self.sub_UbiRobot = self.create_subscription(
            String,
            "ubi_robot",
            self.callback_joystick,
            10
        )

        self.sub_OdometriaRobot = self.create_subscription(
            String,
            "odometria_robot",
            self.callback_joystick,
            10
        )

        self.sub_RealizarMedicion = self.create_subscription(
            String,
            "realizar_medicion",
            self.callback_joystick,
            10
        )
        
     
        self.timer = self.create_timer(
            0.02,
            self.leer_serial
        )

    def leer_serial(self):

        if self.serial.in_waiting == 0:
            return
        
        try:
            linea = self.serial.readline().decode().strip()

            datos = linea.split(",")

            if len(datos) != 25:
                self.get_logger().warn("Trama inválida")
                return

            Contador = datos[0]
            ControlConectado = datos[1]
            Modo = datos[2]

            Js_Robot = ",".join([datos[3], datos[4], datos[7], datos[8]])

            Js_Gimball = ",".join([datos[5], datos[6]])


            GPSEstacion = ",".join(datos[9:14]) #no se usa el 14 solo 9,10,11,12,13

            Sensando = datos[14]

            NPK = ",".join(datos[15:24])

            EstadoMuestra = datos[24]

            modo = String()
            modo.data = Modo
            self.pub_modo.publish(modo)

            joyRobot = String()
            joyRobot.data = Js_Robot
            self.pub_control_RARO(joyRobot)

            joyGimball = String()
            joyGimball.data = Js_Gimball
            self.pub_control_Gimball.publish(joyGimball)

            gpsEstacion = String()
            gpsEstacion.data = GPSEstacion
            self.pub_GPS_Estacion.publish(gpsEstacion)

            sensando = String()
            sensando.data = Sensando
            self.pub_Sensando.publish(sensando)

            npk = String()
            npk.data = NPK
            self.pub_NPK.publish(npk)

            estadoMuestra = String()
            estadoMuestra.data = EstadoMuestra
            self.pub_estado_muestra.publish(estadoMuestra)

            self.get_logger().info(linea) #muestra en la terminal del robot un mensaje  informativo  de 
                                        #tipo info con el contenido de la variable linea para que el 
                                        #operador pueda ver que datos de esatn procesando en timepo real

        except Exception as e:
            self.get_logger().error(str(e))

def main(args=None):
    rclpy.init(args=args)

    modulo_actuador = ModuloActuador()

    rclpy.spin(modulo_actuador)

    modulo_actuador.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
