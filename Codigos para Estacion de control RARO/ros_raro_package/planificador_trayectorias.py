import rclpy
from rclpy.node import Node 
from std_msgs.msg import String

class PlanificadorTrayectorias(Node):

    def __init__(self):

        super().__init__("planificador_trayectorias")

        self.create_subscription(
            String,
            "estado_control",
            self.callback,
            10
        )

    def callback(self,msg):

        modo = msg.data

        if modo == "MANUAL":

            self.get_logger().info("Control manual")

        elif modo == "AUTO":

            self.get_logger().info("Calculando trayectoria")

        else:

            self.get_logger().warn("Modo desconocido")

def main(args=None):
    rclpy.init(args=args)

    planificador_trayectorias = PlanificadorTrayectorias()

    rclpy.spin(planificador_trayectorias)

    planificador_trayectorias.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()