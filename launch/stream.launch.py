# Copyright (c) 2024 IIT-HRII. All rights reserved.

import os
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare

from launch import LaunchDescription

def generate_launch_description():
    
    # device_id_remap_arg_name = "device_id_remap"
    # device_id_remap_arg = DeclareLaunchArgument(
    #     device_id_remap_arg_name,
    #     default_value=PathJoinSubstitution(
    #         [FindPackageShare("xsens_dot_ros2"), "config", "device_id_remap.yaml"]
    #     ),
    #     description="Path to the device ID remap configuration file.",
    # )

    # device_id_remap_config = LaunchConfiguration(device_id_remap_arg_name)

    enable_logging_arg_name = "enable_logging"
    enable_logging_arg = DeclareLaunchArgument(
        enable_logging_arg_name,
        default_value="true",
        description="Enable logging for the xsens_dot_node.",
    )

    config_file = PathJoinSubstitution(
        [FindPackageShare("xsens_dot_ros2"), "config", "device_id_remap.yaml"]
    )
    return LaunchDescription(
        [
            # device_id_remap_arg,
            enable_logging_arg,
            Node(
                package="xsens_dot_ros2",
                executable="xsens_dot_node",
                output="screen",
                namespace="xsens",
                parameters=[
                    # device_id_remap_config
                    config_file,
                    {enable_logging_arg_name: LaunchConfiguration(enable_logging_arg_name)}
                ],
                arguments=["--ros-args", "--log-level", "info"],
            ),
        ]
    )
