import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription, ExecuteProcess
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():
    pkg_share = get_package_share_directory('firefighter_robot')

    robot_id    = LaunchConfiguration('robot_id',    default='robot_1')
    model_path  = LaunchConfiguration('model_path',  default=os.path.join(os.getcwd(), 'models/fire_smoke.onnx'))
    backend_url = LaunchConfiguration('backend_url', default='http://localhost:8001/alerts')
    world_file  = LaunchConfiguration('world_file',  default=os.path.join(os.getcwd(), 'simulation/worlds/fire_building.sdf'))

    return LaunchDescription([
        DeclareLaunchArgument('robot_id',    default_value='robot_1'),
        DeclareLaunchArgument('model_path',  default_value=os.path.join(os.getcwd(), 'models/fire_smoke.onnx')),
        DeclareLaunchArgument('backend_url', default_value='http://localhost:8001/alerts'),
        DeclareLaunchArgument('world_file',  default_value=os.path.join(os.getcwd(), 'simulation/worlds/fire_building.sdf')),

        # ── Gazebo simulation ──────────────────────────────────────────────
        ExecuteProcess(
            cmd=['gz', 'sim', world_file],
            additional_env={
                'GZ_SIM_RESOURCE_PATH': '/opt/ros/jazzy/share/turtlebot3_gazebo/models',
                'TURTLEBOT3_MODEL': 'waffle',
            },
            output='screen'
        ),

        # ── ROS-Gazebo bridge ──────────────────────────────────────────────
        Node(
            package='ros_gz_bridge',
            executable='parameter_bridge',
            name='ros_gz_bridge',
            arguments=[
                '/scan@sensor_msgs/msg/LaserScan[gz.msgs.LaserScan',
                '/camera/image_raw@sensor_msgs/msg/Image[gz.msgs.Image',
                '/cmd_vel@geometry_msgs/msg/Twist]gz.msgs.Twist',
                '/odom@nav_msgs/msg/Odometry[gz.msgs.Odometry',
                '/tf@tf2_msgs/msg/TFMessage[gz.msgs.Pose_V',
            ],
            output='screen'
        ),

        # ── Robot nodes ────────────────────────────────────────────────────
        Node(
            package='firefighter_robot',
            executable='camera_node',
            name='camera_node',
            output='screen',
            parameters=[{
                'input_topic':  '/camera/image_raw',
                'output_topic': '/robot/camera/image',
            }]
        ),

        Node(
            package='firefighter_robot',
            executable='inference_node',
            name='inference_node',
            output='screen',
            parameters=[{
                'model_path':     model_path,
                'image_topic':    '/robot/camera/image',
                'detection_topic': '/robot/detections',
            }]
        ),

        Node(
            package='firefighter_robot',
            executable='lidar_node',
            name='lidar_node',
            output='screen',
            parameters=[{
                'lidar_topic':    '/scan',
                'survivor_topic': '/robot/survivor_candidates',
                'marker_topic':   '/robot/survivor_markers',
            }]
        ),

        Node(
            package='firefighter_robot',
            executable='decision_node',
            name='decision_node',
            output='screen',
            parameters=[{'robot_id': robot_id}]
        ),

        Node(
            package='firefighter_robot',
            executable='telemetry_node',
            name='telemetry_node',
            output='screen',
            parameters=[{'backend_url': backend_url}]
        ),

        Node(
            package='firefighter_robot',
            executable='update_agent_node',
            name='update_agent_node',
            output='screen',
            parameters=[{
                'model_path':    model_path,
                'registry_url':  'http://localhost:8001/model/latest',
            }]
        ),
    ])
