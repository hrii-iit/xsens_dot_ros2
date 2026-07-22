cmake_minimum_required(VERSION 3.5)
project(xsens_dot_ros2)

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Find dependencies
find_package(ament_cmake REQUIRED)
# find_package(ament_cmake_python REQUIRED)
find_package(ament_cmake_auto REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclpy REQUIRED)
ament_auto_find_build_dependencies()

include_directories(
    include
    include/third_party
    include/third_party/xdpchandler
)

add_library(${PROJECT_NAME}_data_processor SHARED IMPORTED)
set_target_properties(${PROJECT_NAME}_data_processor PROPERTIES
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/include/third_party/lib/libdataprocessor.so"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/include/third_party"
)

add_library(${PROJECT_NAME}_xstypes SHARED IMPORTED)
set_target_properties(${PROJECT_NAME}_xstypes PROPERTIES
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/include/third_party/lib/libxstypes.so"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/include/third_party"
)

add_library(${PROJECT_NAME}_movella_dot_pc_sdk SHARED IMPORTED)
set_target_properties(${PROJECT_NAME}_movella_dot_pc_sdk PROPERTIES
    IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/include/third_party/lib/libmovelladot_pc_sdk.so"
    INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/include/third_party"
)

add_library(xdpchandler_library 
    include/third_party/xdpchandler/xdpchandler.cpp)
add_library(xdpchandler_conio 
    include/third_party/xdpchandler/conio.c)

# Create xsens dot executable
add_executable(xsens_dot_node src/xsens_dot_node.cpp)
ament_target_dependencies(xsens_dot_node rclcpp sensor_msgs std_srvs)
target_link_libraries(xsens_dot_node 
    xdpchandler_library
    xdpchandler_conio
    ${PROJECT_NAME}_xstypes
    ${PROJECT_NAME}_data_processor
    ${PROJECT_NAME}_movella_dot_pc_sdk
)

install(TARGETS
  xsens_dot_node
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY
  launch
  config
  DESTINATION share/${PROJECT_NAME}/
)

ament_package()
