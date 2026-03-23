import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():
    pkg_share = get_package_share_directory('firefighter_robot')
    nav2_params = os.path.join(pkg_share, 'config', 'nav2_params.yaml')
    world_file  = os.path.join(os.getcwd(), 'simulation/worlds/fire_building.sdf')

    # Read TurtleBot3 Waffle URDF and strip namespace placeholders
    urdf_path = '/opt/ros/jazzy/share/turtlebot3_description/urdf/turtlebot3_waffle.urdf'
    with open(urdf_path, 'r') as f:
        robot_description = f.read().replace('${namespace}', '')

    return LaunchDescription([

        # ── Gazebo ─────────────────────────────────────────────────────────
        ExecuteProcess(
            cmd=['gz', 'sim', world_file],
            additional_env={
                'GZ_SIM_RESOURCE_PATH': '/opt/ros/jazzy/share/turtlebot3_gazebo/models',
                'TURTLEBOT3_MODEL': 'waffle',
            },
            output='screen'
        ),

        # ── Robot state publisher — publishes static TF from URDF ───────────
        Node(
            package='robot_state_publisher',
            executable='robot_state_publisher',
            name='robot_state_publisher',
            output='screen',
            parameters=[{
                'robot_description': robot_description,
                'use_sim_time': True,
                'frame_prefix': '',
            }]
        ),

        # ── ROS-Gazebo bridge ───────────────────────────────────────────────
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
                '/joint_states@sensor_msgs/msg/JointState[gz.msgs.Model',
            ],
            output='screen',
            parameters=[{
                'qos_overrides./tf_static.publisher.durability': 'transient_local',
            }]
        ),

        # ── SLAM Toolbox (mapping mode) ─────────────────────────────────────
        Node(
            package='slam_toolbox',
            executable='async_slam_toolbox_node',
            name='slam_toolbox',
            output='screen',
            parameters=[nav2_params, {'use_sim_time': True}],
        ),

        # ── Lifecycle manager to auto-activate slam_toolbox ─────────────────
        Node(
            package='nav2_lifecycle_manager',
            executable='lifecycle_manager',
            name='lifecycle_manager_slam',
            output='screen',
            parameters=[{
                'use_sim_time': True,
                'autostart': True,
                'node_names': ['slam_toolbox'],
            }]
        ),

        # ── RViz ───────────────────────────────────────────────────────────
        Node(
            package='rviz2',
            executable='rviz2',
            name='rviz2',
            output='screen',
            arguments=['-d', os.path.join(pkg_share, 'config', 'mapping.rviz')],
        ),
    ])
