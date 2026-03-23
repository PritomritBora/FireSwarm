import os
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess, IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration
from launch_ros.actions import Node
from ament_index_python.packages import get_package_share_directory


def generate_launch_description():
    pkg_share    = get_package_share_directory('firefighter_robot')
    nav2_bringup = get_package_share_directory('nav2_bringup')
    nav2_params  = os.path.join(pkg_share, 'config', 'nav2_params.yaml')
    world_file   = os.path.join(os.getcwd(), 'simulation/worlds/fire_building.sdf')
    map_file     = os.path.join(os.getcwd(), 'simulation/maps/fire_building_map.yaml')

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

        # ── Robot state publisher ───────────────────────────────────────────
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
                '/cmd_vel_smoothed@geometry_msgs/msg/Twist]gz.msgs.Twist',
                '/odom@nav_msgs/msg/Odometry[gz.msgs.Odometry',
                '/tf@tf2_msgs/msg/TFMessage[gz.msgs.Pose_V',
                '/joint_states@sensor_msgs/msg/JointState[gz.msgs.Model',
                '/clock@rosgraph_msgs/msg/Clock[gz.msgs.Clock',
            ],
            output='screen',
            parameters=[{
                'qos_overrides./tf_static.publisher.durability': 'transient_local',
            }]
        ),

        # ── Relay cmd_vel_smoothed → cmd_vel for Gazebo ────────────────────
        Node(
            package='topic_tools',
            executable='relay',
            name='cmd_vel_relay',
            arguments=['/cmd_vel_smoothed', '/cmd_vel'],
            output='screen'
        ),
        IncludeLaunchDescription(
            PythonLaunchDescriptionSource(
                os.path.join(nav2_bringup, 'launch', 'bringup_launch.py')
            ),
            launch_arguments={
                'map': map_file,
                'use_sim_time': 'true',
                'params_file': nav2_params,
                'autostart': 'true',
            }.items()
        ),

        # ── RViz ───────────────────────────────────────────────────────────
        Node(
            package='rviz2',
            executable='rviz2',
            name='rviz2',
            output='screen',
            arguments=['-d', os.path.join(pkg_share, 'config', 'navigation.rviz')],
        ),
    ])
