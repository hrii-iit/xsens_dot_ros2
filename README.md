# XSens Dot ROS 2

This package provides a ROS 2 interface for the XSens Dot PC SDK, enabling communication with XSens Dot devices. It allows users to access sensor data and control the devices through ROS 2 topics and services.

# Build and installation
To build and install the package, follow these steps:
```
dhb
```
And select the [Dockerfile](Dockerfile) to build.

# Usage
To use the XSens Dot ROS 2 package, enable the bluetooth and dbus services in the docker container by running the following command:
```
dhr -v /var/run/dbus:/var/run/dbus -v /run/dbus:/run/dbus
```

Launch the XSens dot streaming:
```bash
ros2 launch xsens_dot_ros2 stream.launch.py
```

# Remap Device IDs
To remap the device IDs, you can set in [](config/device_id_remap.yaml) the remapping of the device IDs. The remapping is done by specifying the original device ID and the new device ID in the following format:
```
original_device_id: new_device_id

The LED should blink in yellow
