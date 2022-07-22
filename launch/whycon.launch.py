#!/usr/bin/env python3
"""Launcher for whycon node."""

import os

from ament_index_python.packages import get_package_share_path

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.conditions import IfCondition, UnlessCondition, LaunchConfigurationEquals
from launch.substitutions import LaunchConfiguration

from launch_ros.actions import Node


def generate_launch_description() -> LaunchDescription:
    """Generate ROS2 launch description."""
    package_path = get_package_share_path('whycon_ros')

    # whycon node
    config = os.path.join( package_path, 'config', 'settings.yaml' )
    whycon_node = Node(
        package='whycon_ros',
        executable='whycon_node',
        parameters=[
            config,
        ],
    )

    # ######  RETURN LAUNCH CONFIG #######
    return LaunchDescription([
        whycon_node
    ])
