import serial

import rclpy
from rclpy.node import Node
from std_msgs.msg import String


class Chasis(Node):

    def __init__(self):

        super().__init__("chasis")

        # Puerto serial del chasis
        self.serial = serial.Serial(
            "/dev/ttyUSB1",
            115200,
            timeout=0.1
        )

        #==============================
        #Publicaciones
        #==============================

        self.pub_GPS_Chasis = self.create_publisher(
            String,
            "gps_chasis",
            10
        )

        self.pub_IMU_Chasis = self.create_publisher(
            String,
            "imu_chasis",
            10
        )

        self.pub_Temp_Chasis = self.create_publisher(
            String,
            "temp_chasis",
            10
        )


        #=============================
        #Suscripciones
        #=============================
        self.sub_LeerControl = self.create_subscription(
            String,
            "LeerControl",
            self.callback_LeerControl,
            10
        )

        self.sub_Movimiento = self.create_subscription(
            String,
            "movimiento",
            self.callback_Movimiento,
            10
        )

        self.sub_control_RARO = self.create_subscription(
            String,
            "control_RARO",
            self.callback_control_RARO,
            10
        )

        self.sub_control_Gimball = self.create_subscription(
            String,
            "control_Gimball",
            self.callback_control_Gimball,
            10
        )

        
        # Lee continuamente el puerto serial
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

            if len(datos) != 9:
                self.get_logger().warn("Trama inválida")
                return

            GPSRobot = ",".join(datos[0:5])
            IMURobot = ",".join(datos[5:8])
            TempRobot = datos[8]

            GPS_msg = String()
            GPS_msg.data = GPSRobot
            self.pub_GPS_Chasis.publish(GPS_msg)

            IMU_msg = String()
            IMU_msg.data = IMURobot
            self.pub_IMU_Chasis.publish(IMU_msg)

            Temp_msg = String()
            Temp_msg.data = TempRobot
            self.pub_Temp_Chasis.publish(Temp_msg)

            self.get_logger().info(linea)

        except Exception as e:
            self.get_logger().error(str(e))



def main(args=None):
    rclpy.init(args=args)

    chasis = Chasis()

    rclpy.spin(chasis)

    chasis.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()