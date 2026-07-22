# XSens Dot ROS 2

This package provides a ROS 2 interface for the XSens Dot PC SDK, enabling communication with XSens Dot devices. It allows users to access sensor data and control the devices through ROS 2 topics and services.

# Build and installation
To build and install the package, follow these steps:
```
dhb
```
And select the [Dockerfile](Dockerfile) to build.


# Remap Device IDs
To remap the device IDs, you can set in [](config/device_id_remap.yaml) the remapping of the device IDs. The remapping is done by specifying the original device ID and the new device ID in the following format:
```
original_device_id: new_device_id

The LED should blink in yellow