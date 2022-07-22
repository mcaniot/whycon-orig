cmake_minimum_required(VERSION 3.5)
project(whycon_ros)

# Default to C99
if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()

# Default to C++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()


# find_package(catkin REQUIRED COMPONENTS
#     image_transport
#     roscpp
#     std_msgs
#     geometry_msgs
#     message_generation
#     dynamic_reconfigure
#     cv_bridge
#     tf
#     )
find_package(ament_cmake REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(visualization_msgs REQUIRED)
find_package(rclcpp REQUIRED)
find_package(tf2_ros REQUIRED)
find_package(image_transport REQUIRED)
find_package(OpenCV REQUIRED)

# generate_dynamic_reconfigure_options(cfg/whycon.cfg)

###################################
## catkin specific configuration ##
###################################
## The catkin_package macro generates cmake config files for your package
## Declare things to be passed to dependent projects
## LIBRARIES: libraries you create in this project that dependent projects also need
## CATKIN_DEPENDS: catkin_packages dependent projects also need
## DEPENDS: system dependencies of this project that dependent projects also need
# catkin_package(
#     #  INCLUDE_DIRS include
#     #  LIBRARIES charging
#     #  CATKIN_DEPENDS image_transport roscpp rospy std_msgs
#     CATKIN_DEPENDS message_runtime std_msgs geometry_msgs dynamic_reconfigure
#     #  DEPENDS system_lib
#     )
set(THIS_PACKAGE_INCLUDE_DEPENDS
    ament_cmake
    rclcpp
    visualization_msgs
    # dynamic_reconfigure
    tf2_ros
)
###########
## Build ##
###########

add_executable(whycon_node src/CWhycon.cpp
    src/CCircleDetect.cpp
    src/CRawImage.cpp
    src/CTimer.cpp
    src/sysmat.cpp
    src/CTransformation.cpp
    src/CNecklace.cpp
    src/CGui.cpp
)

ament_target_dependencies(whycon_node ${THIS_PACKAGE_INCLUDE_DEPENDS})
target_include_directories(whycon_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  ${OpenCV_INCLUDE_DIRS}
  )

target_link_libraries(whycon_node
    SDL
    SDL_ttf
    ${OpenCV_LIBS}
    )

#############
## Install ##
#############

install(TARGETS whycon_node
    DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY
    launch
    DESTINATION share/${PROJECT_NAME}
)

install(DIRECTORY
    etc
    DESTINATION share/${PROJECT_NAME}
)

install(DIRECTORY
    id
    DESTINATION share/${PROJECT_NAME}/
    )

if(<BUILD_TESTING)
    find_package(ament_lint_auto REQUIRED)
    # the following line skips the linter which checks for copyrights
    # uncomment the line when a copyright and license is not present in all source files
    #set(ament_cmake_copyright_FOUND TRUE)
    # the following line skips cpplint (only works in a git repo)
    # uncomment the line when this package is not in a git repo
    #set(ament_cmake_cpplint_FOUND TRUE)
    ament_lint_auto_find_test_dependencies()
endif()
  
ament_package()

