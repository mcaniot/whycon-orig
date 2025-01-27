#include "CWhycon.h"
std::string PKG_NAME = "whycon_ros";
using std::placeholders::_1;

/*manual calibration can be initiated by pressing 'r' and then clicking circles at four positions (0,0)(fieldLength,0)...*/
void CWhycon::manualcalibration(){
    if (currentSegmentArray[0].valid){
        STrackedObject o = objectArray[0];
        moveOne = moveVal;

        //object found - add to a buffer
        if (calibStep < calibrationSteps) calibTmp[calibStep++] = o;

        //does the buffer contain enough data to calculate the object position
        if (calibStep == calibrationSteps){
            o.x = o.y = o.z = 0;
            for (int k = 0;k<calibrationSteps;k++){
                o.x += calibTmp[k].x;
                o.y += calibTmp[k].y;
                o.z += calibTmp[k].z;
            }
            o.x = o.x/calibrationSteps;	
            o.y = o.y/calibrationSteps;	
            o.z = o.z/calibrationSteps;
            if (calibNum < 4){
                calib[calibNum++] = o;
            }

            //was it the last object needed to establish the transform ?
            if (calibNum == 4){
                //calculate and save transforms
                trans->calibrate2D(calib,fieldLength,fieldWidth);
                trans->calibrate3D(calib,fieldLength,fieldWidth);
                calibNum++;
                numMarkers = wasMarkers;
                trans->saveCalibration(calibDefPath.c_str());
                trans->transformType = lastTransformType;
                detectorArray[0]->localSearch = false;
            }
            calibStep++;
        }
    }
}

/*finds four outermost circles and uses them to set-up the coordinate system - [0,0] is left-top, [0,fieldLength] next in clockwise direction*/
void CWhycon::autocalibration(){
    bool saveVals = true;
    for (int i = 0;i<numMarkers;i++){
        if (detectorArray[i]->lastTrackOK == false) saveVals=false;
    }
    if (saveVals){
        int index[] = {0,0,0,0};	
        int maxEval = 0;
        int eval = 0;
        int sX[] = {-1,+1,-1,+1};
        int sY[] = {+1,+1,-1,-1};
        for (int b = 0;b<4;b++){
            maxEval = -10000000;
            for (int i = 0;i<numMarkers;i++){
                eval = 	sX[b]*currentSegmentArray[i].x +sY[b]*currentSegmentArray[i].y;
                if (eval > maxEval){
                    maxEval = eval;
                    index[b] = i;
                }
            }
        }
        printf("INDEX: %i %i %i %i\n",index[0],index[1],index[2],index[3]);
        for (int i = 0;i<4;i++){
            if (calibStep <= autoCalibrationPreSteps) calib[i].x = calib[i].y = calib[i].z = 0;
            calib[i].x+=objectArray[index[i]].x;
            calib[i].y+=objectArray[index[i]].y;
            calib[i].z+=objectArray[index[i]].z;
        }
        calibStep++;
        if (calibStep == autoCalibrationSteps){
            for (int i = 0;i<4;i++){
                calib[i].x = calib[i].x/(autoCalibrationSteps-autoCalibrationPreSteps);
                calib[i].y = calib[i].y/(autoCalibrationSteps-autoCalibrationPreSteps);
                calib[i].z = calib[i].z/(autoCalibrationSteps-autoCalibrationPreSteps);
            }
            trans->calibrate2D(calib,fieldLength,fieldWidth);
            trans->calibrate3D(calib,fieldLength,fieldWidth);
            calibNum++;
            numMarkers = wasMarkers;
            trans->saveCalibration(calibDefPath.c_str());
            trans->transformType = lastTransformType;
            autocalibrate = false;
        }
    }
}

/*process events coming from GUI*/
void CWhycon::processKeys(){
    //process mouse - mainly for manual calibration - by clicking four circles at the corners of the operational area 
    while (SDL_PollEvent(&event)){
        if (event.type == SDL_MOUSEBUTTONDOWN){
            if (calibNum < 4 && calibStep > calibrationSteps){
                calibStep = 0;
                trans->transformType = TRANSFORM_NONE;
            }
            if (numMarkers > 0){
                currentSegmentArray[numMarkers-1].x = event.motion.x*guiScale; 
                currentSegmentArray[numMarkers-1].y = event.motion.y*guiScale;
                currentSegmentArray[numMarkers-1].valid = true;
                detectorArray[numMarkers-1]->localSearch = true;
            }
        }
    }

    //process keys 
    keys = SDL_GetKeyState(&keyNumber);

    //program control - (s)top, (p)ause+move one frame and resume
    if (keys[SDLK_ESCAPE]) stop = true;
    if (keys[SDLK_SPACE] && lastKeys[SDLK_SPACE] == false){ moveOne = 100000000; moveVal = 10000000;};
    if (keys[SDLK_p] && lastKeys[SDLK_p] == false) {moveOne = 1; moveVal = 0;}

    if (keys[SDLK_m] && lastKeys[SDLK_m] == false) printf("SAVE %03f %03f %03f %03f %03f %03f\n",objectArray[0].x,objectArray[0].y,objectArray[0].z,objectArray[0].d,currentSegmentArray[0].m0,currentSegmentArray[0].m1);
    if (keys[SDLK_n] && lastKeys[SDLK_n] == false) printf("SEGM %03f %03f %03f %03f\n",currentSegmentArray[0].x,currentSegmentArray[0].y,currentSegmentArray[0].m0,currentSegmentArray[0].m1);
    if (keys[SDLK_s] && lastKeys[SDLK_s] == false) image->saveBmp();

    //initiate autocalibration (searches for 4 outermost circular patterns and uses them to establisht the coordinate system)
    if (keys[SDLK_a] && lastKeys[SDLK_a] == false) { calibStep = 0; lastTransformType=trans->transformType; wasMarkers = numMarkers; autocalibrate = true;trans->transformType=TRANSFORM_NONE;}; 

    //manual calibration (click the 4 calibration circles with mouse)
    if (keys[SDLK_r] && lastKeys[SDLK_r] == false) { calibNum = 0; wasMarkers=numMarkers; numMarkers = 1;}

    //debugging - toggle drawing coordinates and debugging results results
    if (keys[SDLK_l] && lastKeys[SDLK_l] == false) drawCoords = drawCoords == false;
    if (keys[SDLK_d] && lastKeys[SDLK_d] == false){ 
        for (int i = 0;i<numMarkers;i++){
            detectorArray[i]->draw = detectorArray[i]->draw==false;
            detectorArray[i]->debug = detectorArray[i]->debug==false;
            decoder->debugSegment = decoder->debugSegment==false;
        }
    }

    //transformations to use - in our case, the relevant transform is '2D'
    if (keys[SDLK_1] && lastKeys[SDLK_1] == false) trans->transformType = TRANSFORM_NONE;
    if (keys[SDLK_2] && lastKeys[SDLK_2] == false) trans->transformType = TRANSFORM_2D;
    if (keys[SDLK_3] && lastKeys[SDLK_3] == false) trans->transformType = TRANSFORM_3D;

    // TODO camera low-level settings 

    //display help
    if (keys[SDLK_h] && lastKeys[SDLK_h] == false) displayHelp = displayHelp == false; 

    //adjust the number of robots to be searched for
    if (keys[SDLK_PLUS]) numMarkers++;
    if (keys[SDLK_EQUALS]) numMarkers++;
    if (keys[SDLK_MINUS]) numMarkers--;
    if (keys[SDLK_KP_PLUS]) numMarkers++;
    if (keys[SDLK_KP_MINUS]) numMarkers--;

    //store the key states
    memcpy(lastKeys,keys,keyNumber);
}

void CWhycon::cameraInfoCallback(const sensor_msgs::msg::CameraInfo::SharedPtr msg){
    if(msg->k[0] == 0){
        rclcpp::shutdown();
    }
    if(msg->k[0] != intrinsic.at<float>(0,0) || msg->k[2] != intrinsic.at<float>(0,2) || msg->k[4] != intrinsic.at<float>(1,1) ||  msg->k[5] != intrinsic.at<float>(1,2)){
        for(int i = 0; i < 5; i++) distCoeffs.at<float>(i) = msg->d[i];
        int tmpIdx = 0;
        for(int i = 0; i < 3; i++){
            for(int j = 0; j < 3; j++){
                intrinsic.at<float>(i, j) = msg->k[tmpIdx++];
            }
        }
        trans->updateParams(intrinsic, distCoeffs);
    }
}

void CWhycon::imageCallback(const sensor_msgs::msg::Image::SharedPtr msg){
    //setup timers to assess system performance
    CTimer timer;
    timer.reset();
    timer.start();

    CTimer globalTimer;
    globalTimer.reset();
    globalTimer.start();

    // check if readjusting of camera is needed
    if (image->bpp != int(msg->step/msg->width) || image->width != int(msg->width) || image->height != int(msg->height)){
        delete image;
        RCLCPP_INFO(get_logger(),"Readjusting image format from %ix%i %ibpp, to %ix%i %ibpp.",
                image->width, image->height, image->bpp, msg->width, msg->height, msg->step/msg->width);
        image = new CRawImage(msg->width,msg->height,msg->step/msg->width);
        if(useGui){
            while(image->height/guiScale > screenHeight || image->height/guiScale > screenWidth) guiScale = guiScale*2;
            if(gui == NULL){
                gui = new CGui(msg->width, msg->height, guiScale, fontPath.c_str());
            }else{
                delete gui;
                gui = new CGui(msg->width, msg->height, guiScale, fontPath.c_str());
            }
        }
    }

    memcpy(image->data,(void*)&msg->data[0],msg->step*msg->height);

    numFound = numStatic = 0;
    timer.reset();

    // track the robots found in the last attempt 
    for (int i = 0;i<numMarkers;i++){
        if (currentSegmentArray[i].valid){
            lastSegmentArray[i] = currentSegmentArray[i];
            currentSegmentArray[i] = detectorArray[i]->findSegment(image,lastSegmentArray[i]);
            currInnerSegArray[i] = detectorArray[i]->getInnerSegment();
        }
    }

    // search for untracked (not detected in the last frame) robots 
    for (int i = 0;i<numMarkers;i++){
        if (currentSegmentArray[i].valid == false){
            lastSegmentArray[i].valid = false;
            currentSegmentArray[i] = detectorArray[i]->findSegment(image,lastSegmentArray[i]);
            currInnerSegArray[i] = detectorArray[i]->getInnerSegment();
        }
        if (currentSegmentArray[i].valid == false) break;		//does not make sense to search for more patterns if the last one was not found
    }

    // perform transformations from camera to world coordinates
    for (int i = 0;i<numMarkers;i++){
        if (currentSegmentArray[i].valid){
            int step = image->bpp;
            int pos;
            pos = ((int)currentSegmentArray[i].x+((int)currentSegmentArray[i].y)*image->width);
            image->data[step*pos+0] = 255;
            image->data[step*pos+1] = 0;
            image->data[step*pos+2] = 0;
            pos = ((int)currInnerSegArray[i].x+((int)currInnerSegArray[i].y)*image->width);
            image->data[step*pos+0] = 0;
            image->data[step*pos+1] = 255;
            image->data[step*pos+2] = 0;

            objectArray[i] = trans->transform(currentSegmentArray[i]);

            if(identify){
                int segmentID = decoder->identifySegment(&currentSegmentArray[i], &objectArray[i], image) + 1;
                // if (debug) printf("SEGMENT ID: %i\n", segmentID);
                if (segmentID > -1){
                    // objectArray[i].yaw = currentSegmentArray[i].angle;
                    currentSegmentArray[i].ID = segmentID;
                }else{
                    currentSegmentArray[i].angle = lastSegmentArray[i].angle;
                    currentSegmentArray[i].ID = lastSegmentArray[i].ID;
                }
            }else{
                float dist1 = sqrt((currInnerSegArray[i].x-objectArray[i].segX1)*(currInnerSegArray[i].x-objectArray[i].segX1)+(currInnerSegArray[i].y-objectArray[i].segY1)*(currInnerSegArray[i].y-objectArray[i].segY1));
                float dist2 = sqrt((currInnerSegArray[i].x-objectArray[i].segX2)*(currInnerSegArray[i].x-objectArray[i].segX2)+(currInnerSegArray[i].y-objectArray[i].segY2)*(currInnerSegArray[i].y-objectArray[i].segY2));
                if(dist1 < dist2){
                    currentSegmentArray[i].x = objectArray[i].segX1;
                    currentSegmentArray[i].y = objectArray[i].segY1;
                    objectArray[i].x = objectArray[i].x1;
                    objectArray[i].y = objectArray[i].y1;
                    objectArray[i].z = objectArray[i].z1;
                    objectArray[i].pitch = objectArray[i].pitch1;
                    objectArray[i].roll = objectArray[i].roll1;
                    objectArray[i].yaw = objectArray[i].yaw1;
                }else{
                    currentSegmentArray[i].x = objectArray[i].segX2;
                    currentSegmentArray[i].y = objectArray[i].segY2;
                    objectArray[i].x = objectArray[i].x2;
                    objectArray[i].y = objectArray[i].y2;
                    objectArray[i].z = objectArray[i].z2;
                    objectArray[i].pitch = objectArray[i].pitch2;
                    objectArray[i].roll = objectArray[i].roll2;
                    objectArray[i].yaw = objectArray[i].yaw2;
                }
                currentSegmentArray[i].angle = 0;
            }

            numFound++;
            if (currentSegmentArray[i].x == lastSegmentArray[i].x) numStatic++;

        }
    }
    // if(numFound > 0) ROS_INFO("Pattern detection time: %i us. Found: %i Static: %i.",globalTimer.getTime(),numFound,numStatic);
    evalTime = timer.getTime();

    // publishing information about tags 
    visualization_msgs::msg::MarkerArray markerArray;

    for (int i = 0;i<numMarkers;i++){
        if (currentSegmentArray[i].valid){
            // printf("ID %d\n", currentSegmentArray[i].ID);
            visualization_msgs::msg::Marker marker;
            marker.header = msg->header;
            marker.id = currentSegmentArray[i].ID;
            marker.type = 0;

            marker.scale.x = currentSegmentArray[i].size;
            marker.scale.y = currentSegmentArray[i].size;
            marker.scale.z = currentSegmentArray[i].size;

            // Convert to ROS standard Coordinate System
            marker.pose.position.x = -objectArray[i].y;
            marker.pose.position.y = -objectArray[i].z;
            marker.pose.position.z = objectArray[i].x;

            // Convert YPR to Quaternion

            tf2::Vector3 axis_vector(objectArray[i].pitch, objectArray[i].roll, objectArray[i].yaw);
            tf2::Vector3 up_vector(0.0, 0.0, 1.0);
            tf2::Vector3 right_vector = axis_vector.cross(up_vector);
            right_vector.normalized();
            tf2::Quaternion quat(right_vector, -1.0*acos(axis_vector.dot(up_vector)));
            quat.normalize();

            marker.pose.orientation.x = quat.x();
            marker.pose.orientation.y = quat.y();
            marker.pose.orientation.z = quat.z();
            marker.pose.orientation.w = quat.w();

            markerArray.markers.push_back(marker);
        }
    }

    if(markerArray.markers.size() > 0) markers_pub->publish(markerArray);

    //draw stuff on the GUI 
    if (useGui){
        gui->drawImage(image);
        gui->drawTimeStats(evalTime,numMarkers);
        gui->displayHelp(displayHelp);
        gui->guideCalibration(calibNum,fieldLength,fieldWidth);
    }
    for (int i = 0;i<numMarkers && useGui && drawCoords;i++){
        if (currentSegmentArray[i].valid) gui->drawStats(currentSegmentArray[i].minx-30,currentSegmentArray[i].maxy,objectArray[i],trans->transformType == TRANSFORM_2D);
    }

    //establishing the coordinate system by manual or autocalibration
    if (autocalibrate && numFound == numMarkers) autocalibration();
    if (calibNum < 4) manualcalibration();

    /* empty for-cycle that isn't used even in master orig version
       for (int i = 0;i<numMarkers;i++){
    //if (currentSegmentArray[i].valid) printf("Object %i %03f %03f %03f %03f %03f\n",i,objectArray[i].x,objectArray[i].y,objectArray[i].z,objectArray[i].error,objectArray[i].esterror);
    }*/

    //gui->saveScreen(runs);
    if (useGui) gui->update();
    if (useGui) processKeys();
}

// cleaning up
CWhycon::~CWhycon(){
    RCLCPP_DEBUG(get_logger(),"Releasing memory.");
    free(calibTmp);
    free(objectArray);
    free(currInnerSegArray);
    free(currentSegmentArray);
    free(lastSegmentArray);

    delete image;
    if (useGui) delete gui;
    for (int i = 0;i<maxMarkers;i++) delete detectorArray[i];
    free(detectorArray);
    delete trans;
    delete decoder;
    delete this;
}

CWhycon::CWhycon()
    : Node(PKG_NAME)
{
    image = new CRawImage(imageWidth,imageHeight, 3);

    // loading params and args from launch file
    std::string package_share_directory = ament_index_cpp::get_package_share_directory(PKG_NAME);
    fontPath = package_share_directory + "/etc/font.ttf";
    calibDefPath = package_share_directory + "/etc/default.cal";
    // Declare param
    this->declare_parameter("identify");
    this->declare_parameter("circleDiameter");
    this->declare_parameter("fieldLength");
    this->declare_parameter("fieldWidth");
    this->declare_parameter("useGui");
    this->declare_parameter("idBits");
    this->declare_parameter("idSamples");
    this->declare_parameter("hammingDist");
    this->declare_parameter("maxMarkers");

    // get Param
    this->get_parameter("identify", identify);
    this->get_parameter("circleDiameter", circleDiameter);
    this->get_parameter("fieldLength", fieldLength);
    this->get_parameter("fieldWidth", fieldWidth);
    this->get_parameter("useGui", useGui);
    this->get_parameter("idBits", idBits);
    this->get_parameter("idSamples", idSamples);
    this->get_parameter("hammingDist", hammingDist);
    this->get_parameter("maxMarkers", maxMarkers);
    numMarkers = maxMarkers;


    moveOne = moveVal;
    moveOne  = 0;
    calibTmp = (STrackedObject*) malloc(calibrationSteps * sizeof(STrackedObject));

    objectArray = (STrackedObject*) malloc(maxMarkers * sizeof(STrackedObject));
    currInnerSegArray = (SSegment*) malloc(maxMarkers * sizeof(SSegment));
    currentSegmentArray = (SSegment*) malloc(maxMarkers * sizeof(SSegment));
    lastSegmentArray = (SSegment*) malloc(maxMarkers * sizeof(SSegment));

    // determine gui size so that it fits the screen
    while (imageHeight/guiScale > screenHeight || imageHeight/guiScale > screenWidth) guiScale = guiScale*2;

    // initialize GUI, image structures, coordinate transformation modules
    if (useGui) gui = new CGui(imageWidth,imageHeight,guiScale, fontPath.c_str());
    trans = new CTransformation(imageWidth,imageHeight,circleDiameter, calibDefPath.c_str());
    trans->transformType = TRANSFORM_NONE;		//in our case, 2D is the default

    detectorArray = (CCircleDetect**) malloc(maxMarkers * sizeof(CCircleDetect*));

    // initialize the circle detectors - each circle has its own detector instance 
    for (int i = 0;i<maxMarkers;i++) detectorArray[i] = new CCircleDetect(imageWidth,imageHeight,identify, idBits, idSamples, hammingDist);
    image->getSaveNumber();

    decoder = new CNecklace(idBits,idSamples,hammingDist);

    // subscribe to camera topic, publish topis with card position, rotation and ID
    subInfo = this->create_subscription<sensor_msgs::msg::CameraInfo>(
        "/camera/color/camera_info", 10,
        std::bind(&CWhycon::cameraInfoCallback, this, _1));
    subImg = this->create_subscription<sensor_msgs::msg::Image>(
        "/camera/color/image_raw", 10,
        std::bind(&CWhycon::imageCallback, this, _1));
    markers_pub = this->create_publisher<visualization_msgs::msg::MarkerArray>("markers", 1);
}

int main(int argc, char *argv[]) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<CWhycon>());
  rclcpp::shutdown();
  return 0;
}
