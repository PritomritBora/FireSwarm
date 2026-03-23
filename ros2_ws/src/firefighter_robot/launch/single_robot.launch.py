from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration
import os

def generate_launch_description():
    robot_id = LaunchConfiguration('robot_id', default='robot_1')
    model_path = LaunchConfiguration('model_path', default='models/fire_smoke.onnx')
    backend_url = LaunchConfiguration('backend_url', default='http://localhost:8000/alerts')

    return LaunchDescription([
        DeclareLaunchArgument('robot_id',    default_value='robot_1'),
        DeclareLaunchArgument('model_path',  default_value='models/fire_smoke.onnx'),
        DeclareLaunchArgument('backend_url', default_value='http://localhost:8000/alerts'),

        Node(package='firefighter_robot', executable='camera_node',
             name='camera_node', output='screen'),

        Node(package='firefighter_robot', executable='inference_node',
             name='inference_node', output='screen',
             parameters=[{'model_path': model_path}]),

        Node(package='firefighter_robot', executable='lidar_node',
             name='lidar_node', output='screen'),

        Node(package='firefighter_robot', executable='decision_node',
             name='decision_node', output='screen',
             parameters=[{'robot_id': robot_id}]),

        Node(package='firefighter_robot', executable='telemetry_node',
             name='telemetry_node', output='screen',
             parameters=[{'backend_url': backend_url}]),

        Node(package='firefighter_robot', executable='update_agent_node',
             name='update_agent_node', output='screen',
             parameters=[{'model_path': model_path}]),
    ])
