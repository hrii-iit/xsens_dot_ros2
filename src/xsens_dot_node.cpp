#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <std_srvs/srv/trigger.hpp>

#include "xdpchandler/xdpchandler.h"

// Define a class that inherits from rclcpp::Node
class XSensDotNode : public rclcpp::Node
{
public:
    XSensDotNode() : Node("dot", rclcpp::NodeOptions().automatically_declare_parameters_from_overrides(true))
    {
        // Create a timer that calls the timer_callback function every 100 milliseconds
        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(5),
            std::bind(&XSensDotNode::timer_callback, this));

        // Check if the scan_duration parameter exists and retrieve its value, or use a default value of 10.0 seconds
        double scan_duration = 20.0;
        if (this->has_parameter("scan_duration"))
            scan_duration = this->get_parameter("scan_duration").as_double();
        
        // Get device ID remap parameters from the ROS2 parameter server
        auto result = this->list_parameters({"device_id_remap"}, 10);
        // List all paramters

        device_id_remap_.clear();
        if (result.names.empty())
        {
            RCLCPP_WARN(this->get_logger(), "No device_id_remap parameter found. Using default device IDs.");
        }
        else
        {
            for (const auto &name : result.names)
            {
                if (name.find("device_id_remap") == 0)
                {
                    std::string device_name = name.substr(std::string("device_id_remap.").size());
                    device_id_remap_[device_name] = this->get_parameter(name).as_string();
                    RCLCPP_INFO(this->get_logger(), "Device ID remap: %s -> %s", device_name.c_str(), device_id_remap_[device_name].c_str());
                }
            }
        }

        // Initialize the XdpcHandler
        if (!xdpcHandler.initialize())
            throw std::runtime_error("Failed to initialize XdpcHandler");

        xdpcHandler.scanForDots(scan_duration);

        if (xdpcHandler.detectedDots().empty())
        {
            xdpcHandler.cleanup();
            throw std::runtime_error("No Movella DOT device(s) found.");
        }

        xdpcHandler.connectDots();

        if (xdpcHandler.connectedDots().empty())
        {
            xdpcHandler.cleanup();
            throw std::runtime_error("Could not connect to any Movella DOT device(s). Aborting.");
        }

        for (auto& device : xdpcHandler.connectedDots())
        {        
            auto filterProfiles = device->getAvailableFilterProfiles();
            RCLCPP_INFO(this->get_logger(), "Device %s has %ld available filter profiles:", device->bluetoothAddress().c_str(), filterProfiles.size());
            for (auto& f : filterProfiles)
                RCLCPP_INFO_STREAM(this->get_logger(), "  - " << f.label());

            RCLCPP_INFO_STREAM(this->get_logger(), "Current profile: " << device->onboardFilterProfile().label());
            if (device->setOnboardFilterProfile(XsString("General")))
                RCLCPP_INFO(this->get_logger(), "Successfully set profile to General");
            else
                RCLCPP_ERROR(this->get_logger(), "Setting filter profile failed!");

            RCLCPP_INFO(this->get_logger(), "Setting quaternion CSV output");
            // device->setLogOptions(XsLogOptions::Quaternion);

            // XsString logFileName = XsString("logfile_") << device->bluetoothAddress().replacedAll(":", "-") << ".csv";
            // RCLCPP_INFO(this->get_logger(), "Enable logging to: %s", logFileName.c_str());
            // if (!device->enableLogging(logFileName))
                // RCLCPP_ERROR(this->get_logger(), "Failed to enable logging. Reason: %s", device->lastResultText().c_str());

            RCLCPP_INFO(this->get_logger(), "Putting device into measurement mode.");
            if (!device->startMeasurement(XsPayloadMode::ExtendedEuler))
            {

                RCLCPP_ERROR(this->get_logger(), "Could not put device into measurement mode. Reason: %s", device->lastResultText().c_str());
                continue;
            }

            // For each device ID define a publisher for the IMU data
            std::string device_id = get_remapped_device_id(device->bluetoothAddress().toStdString());
            RCLCPP_INFO(this->get_logger(), "Creating publisher for device %s with remapped ID %s", device->bluetoothAddress().c_str(), device_id.c_str());
            auto qos_profile = rclcpp::SensorDataQoS();
            imu_pub_map_[device_id] = this->create_publisher<sensor_msgs::msg::Imu>("~/" + device_id + "/imu", qos_profile);
        }

        RCLCPP_INFO(this->get_logger(), "Successfully connected to Movella DOT device(s).");

        for (auto const& device : xdpcHandler.connectedDots())
        {
            RCLCPP_INFO(this->get_logger(), "Resetting heading to default for device %s: ", device->bluetoothAddress().c_str());
            if (device->resetOrientation(XRM_DefaultAlignment))
                RCLCPP_INFO(this->get_logger(), "OK");
            else
                RCLCPP_INFO(this->get_logger(), "NOK: %s", device->lastResultText().c_str());
        }
        
        reset_orientation_srv_ = this->create_service<std_srvs::srv::Trigger>("~/reset_orientation",
            std::bind(&XSensDotNode::reset_orientation_callback, this, std::placeholders::_1, std::placeholders::_2));
    }

    ~XSensDotNode()
    {
        RCLCPP_INFO(this->get_logger(), "Stopping measurement...");
        for (auto device : xdpcHandler.connectedDots())
        {
            if (!device->stopMeasurement())
                RCLCPP_ERROR(this->get_logger(), "Failed to stop measurement.");

            if (!device->disableLogging())
                RCLCPP_ERROR(this->get_logger(), "Failed to disable logging.");
        }

        xdpcHandler.cleanup();
        RCLCPP_INFO(this->get_logger(), "Cleanup done.");
    }

private:

    std::string get_remapped_device_id(const std::string& device_id)
    {
        std::string remapped_id = device_id;
        std::replace(remapped_id.begin(), remapped_id.end(), ':', '_');
        auto it = device_id_remap_.find(remapped_id);
        if (it != device_id_remap_.end())
            return it->second;
        return remapped_id;
    }

    void reset_orientation_callback(const std::shared_ptr<std_srvs::srv::Trigger::Request> request, std::shared_ptr<std_srvs::srv::Trigger::Response> response)
    {
        (void)request;
        response->success = true;
        response->message = "Orientation reset results:\n";
        for (auto const& device : xdpcHandler.connectedDots())
        {
            RCLCPP_INFO(this->get_logger(), "Resetting heading to default for device %s: ", device->bluetoothAddress().c_str());
            if (device->resetOrientation(XRM_DefaultAlignment))
                response->message += "Device " + device->bluetoothAddress().toStdString() + ": OK\n";
            else
            {
                response->message += "Device " + device->bluetoothAddress().toStdString() + ": NOK: " + device->lastResultText().toStdString() + "\n";
                response->success = false;
            }
        }
        RCLCPP_INFO(this->get_logger(), "%s", response->message.c_str());
    }

    void timer_callback()
    {
        if (xdpcHandler.packetsAvailable())
        {
            for (auto const& device : xdpcHandler.connectedDots())
            {
                // Retrieve a packet
                XsDataPacket packet = xdpcHandler.getNextPacket(device->bluetoothAddress());
                std::string device_id = get_remapped_device_id(device->bluetoothAddress().toStdString());
                RCLCPP_DEBUG_STREAM(this->get_logger(), "Device ID: " << device_id << " | Packet ID: " << packet.packetId());
                sensor_msgs::msg::Imu imu_msg;
                imu_msg.header.stamp = this->now();
                imu_msg.header.frame_id = device_id;

                if (packet.containsOrientation())
                {
                    XsEuler euler = packet.orientationEuler();

                    RCLCPP_DEBUG_STREAM(this->get_logger(), "Roll:" << std::right << std::setw(7) << std::fixed << std::setprecision(2) << euler.roll()
                        << ", Pitch:" << std::right << std::setw(7) << std::fixed << euler.pitch()
                        << ", Yaw:" << std::right << std::setw(7) << std::fixed << std::setprecision(2) << euler.yaw()
                        << "| ");
                }
                if (packet.containsCalibratedGyroscopeData())
                {
                    XsVector gyro = packet.calibratedGyroscopeData();
                    RCLCPP_DEBUG_STREAM(this->get_logger(), "Gyro X:" << std::right << std::setw(7) << std::fixed << std::setprecision(2) << gyro[0]
                        << ", Y:" << std::right << std::setw(7) << std::fixed << gyro[1]
                        << ", Z:" << std::right << std::setw(7) << std::fixed << std::setprecision(2) << gyro[2]
                        << "| ");
                    imu_msg.angular_velocity.x = gyro[0];
                    imu_msg.angular_velocity.y = gyro[1];
                    imu_msg.angular_velocity.z = gyro[2];
                }
                if (packet.containsCalibratedAcceleration())
                {
                    XsVector accel = packet.calibratedAcceleration();
                    RCLCPP_DEBUG_STREAM(this->get_logger(), "Accel X:" << std::right << std::setw(7) << std::fixed << std::setprecision(2) << accel[0]
                        << ", Y:" << std::right << std::setw(7) << std::fixed << accel[1]
                        << ", Z:" << std::right << std::setw(7) << std::fixed << std::setprecision(2) << accel[2]
                        << "| ");
                    imu_msg.linear_acceleration.x = accel[0];
                    imu_msg.linear_acceleration.y = accel[1];
                    imu_msg.linear_acceleration.z = accel[2];
                }
                if (packet.containsOrientation())
                {
                    XsQuaternion quat = packet.orientationQuaternion();
                    imu_msg.orientation.x = quat[0];
                    imu_msg.orientation.y = quat[1];
                    imu_msg.orientation.z = quat[2];
                    imu_msg.orientation.w = quat[3];
                }
                imu_pub_map_[device_id]->publish(imu_msg);
            }

            // if (!orientationResetDone && (XsTime::timeStampNow() - startTime) > 5000)
            // {
            //     for (auto const& device : xdpcHandler.connectedDots())
            //     {
            //         RCLCPP_INFO(this->get_logger(), "Resetting heading for device %s: ", device->bluetoothAddress().c_str());
            //         if (device->resetOrientation(XRM_Heading))
            //             RCLCPP_INFO(this->get_logger(), "OK");
            //         else
            //             RCLCPP_INFO(this->get_logger(), "NOK: %s", device->lastResultText().c_str());
            //     }
            //     RCLCPP_INFO(this->get_logger(), "-");
            //     orientationResetDone = true;
            // }
        }
        XsTime::msleep(0);
    } 

    std::map<std::string, rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr> imu_pub_map_;
    rclcpp::Service<std_srvs::srv::Trigger>::SharedPtr reset_orientation_srv_;
    rclcpp::TimerBase::SharedPtr timer_;

    std::map<std::string, std::string> device_id_remap_;

    XdpcHandler xdpcHandler;


};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    try
    {
        rclcpp::spin(std::make_shared<XSensDotNode>());
    }
    catch (const std::exception & e)
    {
        RCLCPP_ERROR(rclcpp::get_logger("rclcpp"), "Exception: %s", e.what());
    }
    rclcpp::shutdown();
    return 0;
}



//     return 0;
// }
