#ifndef CWHYCON_H
#define CWHYCON_H

#define _LARGEFILE_SOURCE                                                          
#define _FILE_OFFSET_BITS 64

#include <stdlib.h>
#include <string>
#include <cmath>
#include <opencv2/opencv.hpp>
#include <SDL/SDL.h>

// WhyCon libs
#include "CGui.h"
#include "CTimer.h"
#include "CCircleDetect.h"
#include "CTransformation.h"
#include "CNecklace.h"
#include "CRawImage.h"

// ROS libraries
#include <ament_index_cpp/get_package_share_directory.hpp>
#include "rclcpp/rclcpp.hpp"
#include <tf2/LinearMath/Quaternion.h>
#include <tf2/LinearMath/Vector3.h>
#include <geometry_msgs/msg/quaternion.hpp>
#include <sensor_msgs/msg/camera_info.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <visualization_msgs/msg/marker_array.hpp>
#include <visualization_msgs/msg/marker.hpp>


using namespace cv;


class CWhycon : public rclcpp::Node
{

    public:
        CWhycon();
        ~CWhycon();
        int imageWidth = 1280;          // default camera resolution
        int imageHeight = 720;         // default camera resolution
        float circleDiameter = 0.012;  // default black circle diameter [m];
        float fieldLength = 1.0;       // X dimension of the coordinate system
        float fieldWidth = 1.0;        // Y dimension of the coordinate system

        // marker detection variables
        bool identify = false;  // whether to identify ID
        int numMarkers = 0;     // num of markers to track
        int numFound = 0;       // num of markers detected in the last step
        int numStatic = 0;      // num of non-moving markers
        int maxMarkers;         // maximum number of markers

        // circle identification
        int idBits = 0;         // num of ID bits
        int idSamples = 360;    // num of samples to identify ID
        int hammingDist = 1;    // hamming distance of ID code

        // program flow control
        bool stop = false;      // stop and exit ?
        int moveVal = 1;        // how many frames to process ?
        int moveOne = moveVal;  // how many frames to process now (setting moveOne to 0 or lower freezes the video stream)

        void cameraInfoCallback(const sensor_msgs::msg::CameraInfo::SharedPtr msg);
        void imageCallback(const sensor_msgs::msg::Image::SharedPtr msg);

    private:

        // GUI-related stuff
        CGui* gui;                 // drawing, events capture
        bool useGui = true;        // use graphic interface at all?
        int guiScale = 1;          // in case camera resolution exceeds screen one, gui is scaled down
        SDL_Event event;           // store mouse and keyboard events
        int keyNumber = 10000;     // number of keys pressed in the last step       
        Uint8 lastKeys[1000];      // keys pressed in the previous step
        Uint8 *keys = NULL;        // pressed keys
        bool displayHelp = false;  // displays some usage hints
        bool drawCoords = true;    // draws coordinatess at the robot's positions
        int runs = 0;              // number of gui updates/detections performed 
        int evalTime = 0;          // time required to detect the patterns
        int screenWidth = 1920;    // max GUI width
        int screenHeight = 1080;   // max GUI height

        // variables related to (auto) calibration
        const int calibrationSteps = 20;            // how many measurements to average to estimate calibration pattern position (manual calib)
        const int autoCalibrationSteps = 30;        // how many measurements to average to estimate calibration pattern position (automatic calib)  
        const int autoCalibrationPreSteps = 10;     // how many measurements to discard before starting to actually auto-calibrating (automatic calib)  
        int calibNum = 5;                           // number of objects acquired for calibration (5 means calibration winished inactive)
        STrackedObject calib[5];                    // array to store calibration patterns positions
        STrackedObject *calibTmp;                   // array to store several measurements of a given calibration pattern
        int calibStep = calibrationSteps+2;         // actual calibration step (num of measurements of the actual pattern)
        bool autocalibrate = false;                 // is the autocalibration in progress ?
        ETransformType lastTransformType = TRANSFORM_2D;  // pre-calibration transform (used to preserve pre-calibation transform type)
        int wasMarkers = 1;                               // pre-calibration number of makrers to track (used to preserve pre-calibation number of markers to track)

        // marker detection variables
        STrackedObject *objectArray;       // object array (detected objects in metric space)
        SSegment *currInnerSegArray;       // inner segment array
        SSegment *currentSegmentArray;     // segment array (detected objects in image space)
        SSegment *lastSegmentArray;        // segment position in the last step (allows for tracking)
        CCircleDetect **detectorArray;     // detector array (each pattern has its own detector)

        CTransformation *trans;         // allows to transform from image to metric coordinates
        CNecklace *decoder;             // Necklace code decoder

        rclcpp::Subscription<sensor_msgs::msg::CameraInfo>::SharedPtr subInfo;                // camera info subscriber
        rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr subImg;                // image raw subscriber
        rclcpp::Publisher<visualization_msgs::msg::MarkerArray>::SharedPtr markers_pub; // publisher of MarkerArray
        CRawImage *image;                       // encapsulation of image raw data

        std::string fontPath;           // path to GUI font
        std::string calibDefPath;       // path to user defined coordinate calibration

        // intrisic and distortion params from camera_info
        Mat intrinsic = Mat::ones(3,3, CV_32FC1);
        Mat distCoeffs = Mat::ones(1,5, CV_32FC1);

        /*manual calibration can be initiated by pressing 'r' and then clicking circles at four positions (0,0)(fieldLength,0)...*/
        void manualcalibration();

        /*finds four outermost circles and uses them to set-up the coordinate system - [0,0] is left-top, [0,fieldLength] next in clockwise direction*/
        void autocalibration();

        /*process events coming from GUI*/
        void processKeys();

};

#endif
