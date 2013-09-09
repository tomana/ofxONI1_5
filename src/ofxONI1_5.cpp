#include "ofxONI1_5.h"

ofxONI1_5::ofxONI1_5(){
	bInited = false;
	bIsConnected = false;
	bIsFrameNew = false;

	// Defaults for optional features
	bUseTexture = true;
	bUseColorImage = true;
	bUseColorizedDepthImage = true;
	bUseUserTracker = true;
	bUseUserMap = true;
	bUseUserMapImage = true;
	bUseSkeletonTracker = true;
	bUseCalibratedRGBDepth = true;

	bDepthOn = false;
	bColorOn = false;
	bUserTrackerOn = false;
	bSkeletonTrackerOn = false;
}

ofxONI1_5::~ofxONI1_5(){

}

void ofxONI1_5::setUseTexture(bool b) {
	bUseTexture = b;
}

void ofxONI1_5::setUseColorImage(bool b) {
	if(bInited) ofLogWarning("ofxONI1_5") << "setUseColorImage() must be called before init().";
	bUseColorImage = b;
}

void ofxONI1_5::setUseColorizedDepthImage(bool b) {
	if(bInited) ofLogWarning("ofxONI1_5") << "setUseColorizedDepthImage() must be called before init().";
	bUseColorizedDepthImage = b;
}

void ofxONI1_5::setUseCalibratedRGBDepth(bool b) {
	if(bInited) ofLogWarning("ofxONI1_5") << "setUseCalibratedRGBDepth() must be called before init().";
	bUseCalibratedRGBDepth = b;
}

void ofxONI1_5::setUseUserTracker(bool b) {
	if(bInited) ofLogWarning("ofxONI1_5") << "setUseUserTracker() must be called before init().";
	bUseUserTracker = b;
}

void ofxONI1_5::setUseUserMap(bool b) {
	if(bInited) ofLogWarning("ofxONI1_5") << "setUseUserMap() must be called before init().";
	bUseUserMap = b;
}

void ofxONI1_5::setUseUserMapImage(bool b) {
	if(bInited) ofLogWarning("ofxONI1_5") << "setUseUserMapImage() must be called before init().";
	bUseUserMapImage = b;
}

void ofxONI1_5::setUseSkeletonTracker(bool b) {
	if(bInited) ofLogWarning("ofxONI1_5") << "setUseSkeletonTracker() must be called before init().";
	bUseSkeletonTracker = b;
}



XnSkeletonJoint trackedJointsArray[] = { 
	XN_SKEL_HEAD, XN_SKEL_NECK, XN_SKEL_TORSO, XN_SKEL_WAIST,
       	XN_SKEL_LEFT_COLLAR, XN_SKEL_LEFT_SHOULDER, XN_SKEL_LEFT_ELBOW,
       	XN_SKEL_LEFT_WRIST, XN_SKEL_LEFT_HAND, XN_SKEL_LEFT_FINGERTIP,
       	XN_SKEL_RIGHT_COLLAR, XN_SKEL_RIGHT_SHOULDER, XN_SKEL_RIGHT_ELBOW,
       	XN_SKEL_RIGHT_WRIST, XN_SKEL_RIGHT_HAND, XN_SKEL_RIGHT_FINGERTIP,
       	XN_SKEL_LEFT_HIP, XN_SKEL_LEFT_KNEE, XN_SKEL_LEFT_ANKLE,
       	XN_SKEL_LEFT_FOOT, XN_SKEL_RIGHT_HIP, XN_SKEL_RIGHT_KNEE,
       	XN_SKEL_RIGHT_ANKLE, XN_SKEL_RIGHT_FOOT };

bool ofxONI1_5::init() {
	trackedJoints.assign(trackedJointsArray, trackedJointsArray + sizeof(trackedJointsArray)/sizeof(trackedJointsArray[0]));

	XnStatus nRetVal = oniContext.Init();
	if(nRetVal != XN_STATUS_OK) {
		ofLogWarning("ofxONI1_5") << "OpenNI Init failed: " << xnGetStatusString(nRetVal);
		return false;
	} else {
		bInited = true;
		return true;
	}
}

bool ofxONI1_5::open(){
	if(!bInited) {
		ofLogWarning("ofxONI1_5") << "Cannot call open() before init() has succeeded.";
		return false;
	}

	XnStatus nRetVal = XN_STATUS_OK;

	//
	// Open depth stream
	//
	nRetVal = oniDepthGenerator.Create(oniContext);
	if(nRetVal != XN_STATUS_OK) {
		ofLogWarning("ofxONI1_5") << "Unable to create depth image context: " << xnGetStatusString(nRetVal);
		return false;
	} else {
		bDepthOn = true;
	}

	//
	// Open user generator stream
	//
	if(bUseUserTracker){
		nRetVal = oniUserGenerator.Create(oniContext);
		if(nRetVal != XN_STATUS_OK){
			ofLogWarning("ofxONI1_5") << "Unable to open user generator stream: " << xnGetStatusString(nRetVal);
		} else {
			bUserTrackerOn = true;
		}
	}

	//
	// Open color/video stream
	//
	if(bUseColorImage){
		nRetVal = oniImageGenerator.Create(oniContext);
		if(nRetVal != XN_STATUS_OK){
			ofLogWarning("ofxONI1_5") << "Unable to open video generator stream: " << xnGetStatusString(nRetVal);
			return false;
		} else {
			bColorOn = true;
		}
	}

	nRetVal = oniContext.StartGeneratingAll();
	if(nRetVal != XN_STATUS_OK){
		ofLogWarning("ofxONI1_5") << "Unable to start all generators: " << xnGetStatusString(nRetVal);
		return false;
	} 


	if(bUseSkeletonTracker){
		if(!oniUserGenerator.IsCapabilitySupported(XN_CAPABILITY_SKELETON)){
			ofLogWarning("ofxONI1_5") << "UserTracker: Skeleton capability not supported.";
		} else {
			XnCallbackHandle hUserCallbacks, hCalibrationCallbacks, hPoseCallbacks;

			oniUserGenerator.RegisterUserCallbacks(
					ofxONI1_5::User_NewUser, 
					ofxONI1_5::User_LostUser, 
					this, hUserCallbacks);

			oniUserGenerator.GetSkeletonCap().RegisterCalibrationCallbacks(
					ofxONI1_5::UserCalibration_CalibrationStart, 
					ofxONI1_5::UserCalibration_CalibrationEnd, 
					this, hCalibrationCallbacks);

			if(oniUserGenerator.GetSkeletonCap().NeedPoseForCalibration()) {
				ofLogWarning("ofxONI1_5") << "UserTracker: NeedPoseForCalibration returned true, update your version of OpenNI.";
				//setUseUserTracker(false);
			}

			// 
			// Since OpenNI update, pose is not needed for skeleton calibration.
			// Will not implement any pose detection as of now.
			//
			// if(oniUserGenerator.GetSkeletonCap().NeedPoseForCalibration()){
			// 	g_bNeedPose = TRUE;
			// 	if(!oniUserGenerator.IsCapabilitySupported(XN_CAPABILITY_POSE_DETECTION)){
			// 		printf("Pose required, but not supported\n");
			// 	}
			// 	oniUserGenerator.GetPoseDetectionCap().RegisterToPoseCallbacks(ofxONI1_5::UserPose_PoseDetected, NULL, this, hPoseCallbacks);
			// 	oniUserGenerator.GetSkeletonCap().GetCalibrationPose(g_strPose);
			// }

			oniUserGenerator.GetSkeletonCap().SetSkeletonProfile(XN_SKEL_PROFILE_ALL);

			bSkeletonTrackerOn = true;
		}
	}

	oniDepthGenerator.GetMetaData(depthMD);

	stream_width = depthMD.XRes();
	stream_height = depthMD.YRes();
	ref_max_depth = -1;

	colorPixels.allocate(stream_width, stream_height, OF_IMAGE_COLOR);
	depthPixelsRaw.allocate(stream_width, stream_height, 1);
	distancePixels.allocate(stream_width, stream_height, 1);

	if(bUseColorizedDepthImage) {
		depthPixels.allocate(stream_width,stream_height, OF_IMAGE_COLOR);
	} else {
		depthPixels.allocate(stream_width,stream_height, 1);
	}
	

	colorPixels.set(0);
	depthPixels.set(0);
	depthPixelsRaw.set(0);
	distancePixels.set(0);

	if(bUseTexture){
		depthTex.allocate(stream_width, stream_height, GL_RGB);
		colorTex.allocate(stream_width, stream_height, GL_RGB);
	}

	if(bUserTrackerOn) {
		userMap.allocate(stream_width, stream_height, 1);
		if(bUseUserMapImage) {
			userMapImageTex.allocate(stream_width, stream_height, GL_RGB);
		}
	}

	if(bUseCalibratedRGBDepth) {
		enableCalibratedRGBDepth();
	}

	bIsConnected = true;

	return true;
}

void ofxONI1_5::close(){

	oniContext.StopGeneratingAll();
	oniDepthGenerator.Release();
	oniUserGenerator.Release();
	oniImageGenerator.Release();
	oniContext.Release();
	
	bIsConnected = false;
}

void ofxONI1_5::clear(){
	if(isConnected()){
		ofLogWarning("ofxONI1_5") << "clear(): do not call clear while ofxONI1_5 is running!";
		return;
	}

	depthPixelsRaw.clear();

	colorPixels.clear();
	depthPixels.clear();

	distancePixels.clear();

	depthTex.clear();
	colorTex.clear();

	bInited = false;
}

void ofxONI1_5::update(){

	if(bDepthOn && oniDepthGenerator.IsNewDataAvailable()) {
		bIsFrameNew = true;
		oniDepthGenerator.WaitAndUpdateData();
		oniDepthGenerator.GetMetaData(depthMD);
		updateDepth();
	}

	if(bColorOn && oniImageGenerator.IsNewDataAvailable()) {
		bIsFrameNew = true;
		oniImageGenerator.WaitAndUpdateData();
		oniImageGenerator.GetMetaData(oniImageGeneratorMD);
		updateColor();
	}

	if(bUserTrackerOn && oniUserGenerator.IsNewDataAvailable()) {
		bIsFrameNew = true;
		oniUserGenerator.WaitAndUpdateData();
		updateUserTracker();
	}

}

void ofxONI1_5::updateDepth() {
	// Data has type XnDepthPixel, should be typedef for uint16.
	uint16_t *pDepth = (uint16_t*) depthMD.Data();

	if(ref_max_depth == -1) {
		for(int i = 0; i < stream_width*stream_height; i++) {
			if(pDepth[i] > ref_max_depth) {
				ref_max_depth = pDepth[i];
			}
		}

		ofLogVerbose("ofxONI1_5") << "Max depth established to " << ref_max_depth;
	}

	unsigned short *rawpixel = depthPixelsRaw.getPixels();
	unsigned char  *depthpixel = depthPixels.getPixels();
	float          *floatpixel = distancePixels.getPixels();

	ofColor c;
	for(int i = 0; i < stream_width*stream_height; i++) {
		rawpixel[i] = pDepth[i];
		floatpixel[i] = rawpixel[i];

		unsigned char hue = (unsigned char)(255.0 * (floatpixel[i] / ref_max_depth));

		if(bUseColorizedDepthImage) {
			if(rawpixel[i] > 0) {
				c = ofColor::fromHsb(hue,255,255);
			} else {
				c = ofColor::black;
			}

			depthpixel[3*i +0] = c.r;
			depthpixel[3*i +1] = c.g;
			depthpixel[3*i +2] = c.b;
		} else {
			depthpixel[i] = hue;
		}
	}

	if(bUseTexture) {
		if(bUseColorizedDepthImage) {
			depthTex.loadData(depthPixels.getPixels(), stream_width, stream_height, GL_RGB);
		} else {
			depthTex.loadData(depthPixels.getPixels(), stream_width, stream_height, GL_LUMINANCE);
		}
	}

}

void ofxONI1_5::updateColor() {
	// Color image is assumed to be 24 bit RGB.
	colorPixels.setFromPixels( (unsigned char* ) oniImageGeneratorMD.Data(), 
			oniImageGeneratorMD.XRes(), oniImageGeneratorMD.YRes(), OF_IMAGE_COLOR);

	if(bUseTexture) {
		colorTex.loadData(colorPixels);
	}
}

void ofxONI1_5::updateUserTracker() {
	userData.clear();
	ostringstream debugString;

	unsigned short numUsers = oniUserGenerator.GetNumberOfUsers();
	XnUserID* userArray = new XnUserID[numUsers];
	oniUserGenerator.GetUsers(userArray, numUsers);

	debugString << "Number of users found: " << numUsers << endl;
	for(int i = 0; i < numUsers; i++) {
		UserData d;
		d.id = userArray[i];

		XnPoint3D com;
		oniUserGenerator.GetCoM(d.id, com);
		d.centerOfMass = toOf(com);
		debugString << "User #" << d.id << ", center of mass: " << d.centerOfMass << endl;

		if(bUseSkeletonTracker) {
			xn::SkeletonCapability skeleton = oniUserGenerator.GetSkeletonCap();

			d.isSkeletonAvailable = skeleton.IsTracking(d.id); // Is this correct?
			debugString << "\tSkeleton available: " << (d.isSkeletonAvailable ? "yes" : "no") << endl;

			for(int i = 0; i < trackedJoints.size(); i++) {
				XnSkeletonJoint joint = trackedJoints[i];
				XnSkeletonJointTransformation jointdata;
				skeleton.GetSkeletonJoint(d.id, joint, jointdata);
				d.skeletonPoints[joint] = toOf(jointdata.position.position);

				debugString << "\tJoint " << joint << " at " << d.skeletonPoints[joint] << endl;
			}
		} else {
			d.isSkeletonAvailable = false;
		}

		userData.push_back(d);
	}

	stringUserTrackerDebug = debugString.str();

	if(bUseUserMap) {
		oniUserGenerator.GetUserPixels(0,sceneMD);
		unsigned short* userMapPixels = (unsigned short*) sceneMD.Data();
		userMap.setFromPixels( userMapPixels,
				stream_width, stream_height, 1);

		if(bUseUserMapImage) {
			unsigned char* p = new unsigned char[3*stream_width*stream_height];
			for(int i = 0; i < stream_width*stream_height; i++) {
				short id = userMapPixels[i];
				p[3*i + 0] = 255.0*oniColors[id%nColors][0];
				p[3*i + 1] = 255.0*oniColors[id%nColors][1];
				p[3*i + 2] = 255.0*oniColors[id%nColors][2];
			}

			userMapImageTex.loadData(p,stream_width,stream_height,GL_RGB);
			delete p;
		}
	}

}

bool ofxONI1_5::isConnected(){
	return bIsConnected;
}

bool ofxONI1_5::isFrameNew(){
	if(bIsFrameNew) {
		bIsFrameNew = false;
		return true;
	} else {
		return false;
	}
}

void ofxONI1_5::enableCalibratedRGBDepth(){
	if(!oniImageGenerator.IsValid()){
		ofLogWarning("ofxONI1_5") << "useCalibratedRGBDepth: No Image generator found: cannot register viewport";
	}

	// Register view point to image map
	if(oniDepthGenerator.IsCapabilitySupported(XN_CAPABILITY_ALTERNATIVE_VIEW_POINT)) {
		XnStatus result = oniDepthGenerator.GetAlternativeViewPointCap().SetViewPoint(oniImageGenerator);
		if(result != XN_STATUS_OK) {
			ofLogWarning("ofxONI1_5") << "useCalibratedRGBDepth failed.";
		}
	} else {
		ofLogWarning("ofxONI1_5") << "Can't enable calibrated RGB depth, alternative viewport capability not supported";
	}
}

void ofxONI1_5::draw(float x, float y, float w, float h){
	if(bUseTexture && bUseColorImage && bColorOn){
		colorTex.draw(x, y, w, h);
	}
}

void ofxONI1_5::drawUsers(float x, float y, float w, float h){
	if(bUseUserTracker && bUseUserMapImage){
		userMapImageTex.draw(x, y, w, h);
	}
}

void ofxONI1_5::drawSkeletonOverlay(float x, float y, float w, float h) {
	if(bUseUserTracker && bUseSkeletonTracker) {
		ofPushStyle();
		ofFill();
		ofSetColor(ofColor::red,255);
		for(int i = 0; i < userData.size(); i++) {
			if(userData[i].isSkeletonAvailable) {
				for(int j = 0; j < trackedJoints.size(); j++) {
					ofVec3f p = userData[i].skeletonPoints[trackedJoints[j]];
					p = coordsRealToProjective(p);
					p.x = ofMap(p.x, 0, stream_width, 0, w)   + x;
					p.y = ofMap(p.y, 0, stream_height, 0, h)  + y;
					ofCircle(ofPoint(p.x,p.y),5);
				}
			}
		}
		ofPopStyle();
		
	}
}

void ofxONI1_5::drawDepth(float x, float y, float w, float h){
	if(bUseTexture){
		depthTex.draw(x, y, w, h);
	}
}

float ofxONI1_5::getWidth(){
	return stream_width;
}

float ofxONI1_5::getHeight(){
	return stream_height;
}

ofVec3f ofxONI1_5::coordsRealToProjective(ofVec3f v) {
	XnVector3D p = {v.x, v.y, v.z};
	oniDepthGenerator.ConvertRealWorldToProjective(1,&p,&p);
	return ofVec3f(p.X, p.Y, p.Z);
}

ofVec3f ofxONI1_5::coordsProjectiveToReal(ofVec3f v) {
	XnVector3D p = {v.x, v.y, v.z};
	oniDepthGenerator.ConvertProjectiveToRealWorld(1,&p,&p);
	return ofVec3f(p.X, p.Y, p.Z);
}


//
//
// User tracker callbacks. Must be static.
// *pCookie is set to pointer to ofxONI1_5 object (this), 
// so the static method can call the non-static method.
//
//

// Callback for new user.
void XN_CALLBACK_TYPE ofxONI1_5::User_NewUser(xn::UserGenerator & generator, XnUserID nId, void * pCookie){
	((ofxONI1_5*)pCookie)->cbNewUser(generator,nId);
}

void ofxONI1_5::cbNewUser(xn::UserGenerator & generator, XnUserID nId) {
	ofLogVerbose("ofxONI1_5") << "UserTracker: new user #" << nId;
	short userid = nId;
	ofNotifyEvent(newUserEvent, userid);

	oniUserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
}

// Callback for lost user.
void XN_CALLBACK_TYPE ofxONI1_5::User_LostUser(xn::UserGenerator & generator, XnUserID nId, void * pCookie){
	((ofxONI1_5*)pCookie)->cbLostUser(generator,nId);
}

void ofxONI1_5::cbLostUser(xn::UserGenerator & generator, XnUserID nId) {
	ofLogVerbose("ofxONI1_5") << "UserTracker: lost user #" << nId;
	short userid = nId;
	ofNotifyEvent(lostUserEvent, userid);
}

// Callback for skeleton calibration start.
void XN_CALLBACK_TYPE ofxONI1_5::UserCalibration_CalibrationStart(xn::SkeletonCapability & capability, XnUserID nId, void * pCookie){
	((ofxONI1_5*)pCookie)->cbUserCalibrationStart(capability, nId);
}

void ofxONI1_5::cbUserCalibrationStart(xn::SkeletonCapability& capability, XnUserID nId) {
	ofLogVerbose("ofxONI1_5") << "UserTracker: starting skeleton calibration for user #" << nId;
}

// Callback for skeleton calibration end.
void XN_CALLBACK_TYPE ofxONI1_5::UserCalibration_CalibrationEnd(xn::SkeletonCapability & capability, XnUserID nId, XnBool bSuccess, void * pCookie){
	((ofxONI1_5*)pCookie)->cbUserCalibrationEnd(capability,nId,bSuccess);
}

void ofxONI1_5::cbUserCalibrationEnd(xn::SkeletonCapability & capability, XnUserID nId, XnBool bSuccess) {
	if(bSuccess){
		// Calibration succeeded
		ofLogVerbose("ofxONI1_5") << "UserTracker: calibration complete for user #" << nId;
		oniUserGenerator.GetSkeletonCap().StartTracking(nId);
	}
	else{
		// Calibration failed
		ofLogVerbose("ofxONI1_5") << "UserTracker: calibration failed for user #" << nId;
		oniUserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
	}
}


// Callback for pose detection. Not used.
//
// void XN_CALLBACK_TYPE ofxONI1_5::UserPose_PoseDetected(xn::PoseDetectionCapability & capability, const XnChar * strPose, XnUserID nId, void * pCookie){
// 	printf("Pose %s detected for user %d\n", strPose, nId);
// 	oniUserGenerator.GetPoseDetectionCap().StopPoseDetection(nId);
// 	oniUserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
// }

