#include "ofxONI1_5.h"

// Declare static members in ofxONI1_5
xn::Context ofxONI1_5::g_Context;
xn::DepthGenerator ofxONI1_5::g_DepthGenerator;
xn::UserGenerator ofxONI1_5::g_UserGenerator;
xn::ImageGenerator ofxONI1_5::g_image;

ofxONI1_5::ofxONI1_5(){
	bNeedsUpdateDepth = false;
	bNeedsUpdateColor = false;
	bIsConnected = false;
	bUseUserMap = true;
}

ofxONI1_5::~ofxONI1_5(){

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

bool ofxONI1_5::init(bool use_color_image, bool use_texture, bool colorize_depth_image, bool use_players, bool use_skeleton){
	XnStatus nRetVal = XN_STATUS_OK;

	bUseTexture = use_texture;
	bGrabVideo = use_color_image;
	bColorizeDepthImage = colorize_depth_image;
	bDrawPlayers = use_players;
	bDrawSkeleton = use_skeleton;

	trackedJoints.assign(trackedJointsArray, trackedJointsArray + sizeof(trackedJointsArray)/sizeof(trackedJointsArray[0]));


	// printf("InitFromXmlFile\n");
	// nRetVal = g_Context.InitFromXmlFile(SAMPLE_XML_PATH);
	nRetVal = g_Context.Init();
	CHECK_RC(nRetVal, "InitFromXml");
	return true;
}

bool ofxONI1_5::open(){
	XnStatus nRetVal = XN_STATUS_OK;

	printf("FindExistingNode\n");
	//nRetVal = g_Context.FindExistingNode(XN_NODE_TYPE_DEPTH, g_DepthGenerator);
	nRetVal = g_DepthGenerator.Create(g_Context);
	CHECK_RC(nRetVal, "Find depth generator");
	bDepthOn = true;

	if(bDrawPlayers){
		//nRetVal = g_Context.FindExistingNode(XN_NODE_TYPE_USER, g_UserGenerator);
		nRetVal = g_UserGenerator.Create(g_Context);
		if(nRetVal != XN_STATUS_OK){
			nRetVal = g_UserGenerator.Create(g_Context);
			CHECK_RC(nRetVal, "Find user generator");
		}
		bUserTrackerOn = true;
	}
	if(bGrabVideo){
		//printf("FindExistingNode\n");
		//nRetVal = g_Context.FindExistingNode(XN_NODE_TYPE_IMAGE, g_image);
		nRetVal = g_image.Create(g_Context);
		CHECK_RC(nRetVal, "Find image generator");
		bColorOn = true;
	}

	nRetVal = g_Context.StartGeneratingAll();
	CHECK_RC(nRetVal, "StartGenerating");


	// old
	if(bDrawSkeleton){
		if(!g_UserGenerator.IsCapabilitySupported(XN_CAPABILITY_SKELETON)){
			printf("Supplied user generator doesn't support skeleton\n");
		} else {
			XnCallbackHandle hUserCallbacks, hCalibrationCallbacks, hPoseCallbacks;

			g_UserGenerator.RegisterUserCallbacks(
					ofxONI1_5::User_NewUser, 
					ofxONI1_5::User_LostUser, 
					this, hUserCallbacks);

			g_UserGenerator.GetSkeletonCap().RegisterCalibrationCallbacks(
					ofxONI1_5::UserCalibration_CalibrationStart, 
					ofxONI1_5::UserCalibration_CalibrationEnd, 
					this, hCalibrationCallbacks);

			if(g_UserGenerator.GetSkeletonCap().NeedPoseForCalibration()) {
				ofLogWarning("ofxONI1_5") << "UserTracker: NeedPoseForCalibration returned true, update your version of OpenNI.";
				//setUseUserTracker(false);
			}

			// 
			// Since OpenNI update, pose is not needed for skeleton calibration.
			// Will not implement any pose detection as of now.
			//
			// if(g_UserGenerator.GetSkeletonCap().NeedPoseForCalibration()){
			// 	g_bNeedPose = TRUE;
			// 	if(!g_UserGenerator.IsCapabilitySupported(XN_CAPABILITY_POSE_DETECTION)){
			// 		printf("Pose required, but not supported\n");
			// 	}
			// 	g_UserGenerator.GetPoseDetectionCap().RegisterToPoseCallbacks(ofxONI1_5::UserPose_PoseDetected, NULL, this, hPoseCallbacks);
			// 	g_UserGenerator.GetSkeletonCap().GetCalibrationPose(g_strPose);
			// }

			g_UserGenerator.GetSkeletonCap().SetSkeletonProfile(XN_SKEL_PROFILE_ALL);
		}
	}

	g_DepthGenerator.GetMetaData(depthMD);

	stream_width = depthMD.XRes();
	stream_height = depthMD.YRes();
	ref_max_depth = -1;

	depthPixelsRaw.allocate(stream_width, stream_height, 1);

	videoPixels.allocate(stream_width, stream_height, OF_IMAGE_COLOR);

	if(bColorizeDepthImage) {
		depthPixels.allocate(stream_width,stream_height, OF_IMAGE_COLOR);
	} else {
		depthPixels.allocate(stream_width,stream_height, 1);
	}
	distancePixels.allocate(stream_width, stream_height, 1);
	playersPixels.allocate(stream_width, stream_height, OF_IMAGE_COLOR_ALPHA);
	grayPixels.allocate(stream_width, stream_height, 1);

	depthPixelsRaw.set(0);

	videoPixels.set(0);

	depthPixels.set(0);
	playersPixels.set(0);
	distancePixels.set(0);
	grayPixels.set(0);

	if(bUseTexture){
		depthTex.allocate(stream_width, stream_height, GL_RGB);
		videoTex.allocate(stream_width, stream_height, GL_RGB);
		playersTex.allocate(stream_width, stream_height, GL_RGBA);
		grayTex.allocate(stream_width, stream_height, GL_LUMINANCE);
	}

	if(bUserTrackerOn) {
		userMap.allocate(stream_width, stream_height, 1);
	}

	// MAYBE THIS SHOULD NOT BE HERE
	enableCalibratedRGBDepth();

	//startThread(true, false);
	
	bIsConnected = true;

	return true;
}

void ofxONI1_5::close(){

// NOT WORKING PROPERLY
	stopGenerators();
	g_Context.StopGeneratingAll();
	g_Context.Release();
	bNeedsUpdateDepth = false;
	//stopThread();
	
	bIsConnected = false;
}

//--------------------------------------------------------------
void ofxONI1_5::stopGenerators(){

	g_DepthGenerator.StopGenerating();
	g_DepthGenerator.Release();

	g_UserGenerator.StopGenerating();
	g_UserGenerator.Release();

	g_image.StopGenerating();
	g_image.Release();

}

void ofxONI1_5::clear(){
	if(isConnected()){
		ofLogWarning("ofxONI1_5") << "clear(): do not call clear while ofxONI1_5 is running!";
		return;
	}

	depthPixelsRaw.clear();

	playersPixels.clear();
	videoPixels.clear();
	depthPixels.clear();

	distancePixels.clear();

	depthTex.clear();
	videoTex.clear();
	playersTex.clear();

	bGrabberInited = false;
}

void ofxONI1_5::update(){

	if(bDepthOn && g_DepthGenerator.IsNewDataAvailable()) {
		bIsFrameNew = true;
		g_DepthGenerator.WaitAndUpdateData();
		g_DepthGenerator.GetMetaData(depthMD);
		updateDepth();
	}

	if(bColorOn && g_image.IsNewDataAvailable()) {
		bIsFrameNew = true;
		g_image.WaitAndUpdateData();
		g_image.GetMetaData(g_imageMD);
		updateColor();
	}

	if(bUserTrackerOn && g_UserGenerator.IsNewDataAvailable()) {
		bIsFrameNew = true;
		g_UserGenerator.WaitAndUpdateData();
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

		if(bColorizeDepthImage) {
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
		if(bColorizeDepthImage) {
			depthTex.loadData(depthPixels.getPixels(), stream_width, stream_height, GL_RGB);
		} else {
			depthTex.loadData(depthPixels.getPixels(), stream_width, stream_height, GL_LUMINANCE);
		}
	}

}

void ofxONI1_5::updateColor() {
	// Color image is assumed to be 24 bit RGB.
	videoPixels.setFromPixels( (unsigned char* ) g_imageMD.Data(), 
			g_imageMD.XRes(), g_imageMD.YRes(), OF_IMAGE_COLOR);

	if(bUseTexture) {
		videoTex.loadData(videoPixels);
	}
}



void ofxONI1_5::updateUserTracker() {
	if(bUseUserMap) {
		g_UserGenerator.GetUserPixels(0,sceneMD);
		userMap.setFromPixels( (unsigned short*) sceneMD.Data(), 
				stream_width, stream_height, 1);
	}

	userData.clear();

	unsigned short numUsers = g_UserGenerator.GetNumberOfUsers();
	XnUserID* userArray = new XnUserID[numUsers];
	g_UserGenerator.GetUsers(userArray, numUsers);
	for(int i = 0; i < numUsers; i++) {
		UserData d;
		d.id = userArray[i];

		XnPoint3D com;
		g_UserGenerator.GetCoM(d.id, com);
		d.centerOfMass = toOf(com);

		xn::SkeletonCapability skeleton = g_UserGenerator.GetSkeletonCap();

		d.isSkeletonAvailable = skeleton.IsTracking(d.id); // Is this correct?

		for(int i = 0; i < trackedJoints.size(); i++) {
			XnSkeletonJoint joint = trackedJoints[i];
			XnSkeletonJointPosition jointdata;
			skeleton.GetSkeletonJointPosition(d.id, joint, jointdata);
			d.skeletonPoints[joint] = toOf(jointdata.position);
		}

		userData.push_back(d);
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

unsigned char * ofxONI1_5::getPixels(){
	return videoPixels.getPixels();
}

unsigned char * ofxONI1_5::getDepthPixels(){
	return depthPixels.getPixels();
}

unsigned short * ofxONI1_5::getRawDepthPixels(){
	return depthPixelsRaw.getPixels();
}

float * ofxONI1_5::getDistancePixels(){
	return distancePixels.getPixels();
}

unsigned char * ofxONI1_5::getPlayersPixels(){
	return playersPixels.getPixels();
}


ofPixels & ofxONI1_5::getPixelsRef(){
	return videoPixels;
}

ofPixels & ofxONI1_5::getDepthPixelsRef(){
	return depthPixels;
}

ofShortPixels & ofxONI1_5::getRawDepthPixelsRef(){
	return depthPixelsRaw;
}

ofFloatPixels & ofxONI1_5::getDistancePixelsRef(){
	return distancePixels;
}

ofTexture & ofxONI1_5::getTextureReference(){
	if(!videoTex.bAllocated()){
		ofLogWarning("ofxONI2") << "getTextureReference video texture not allocated";
	}

	return videoTex;
}

ofTexture & ofxONI1_5::getDepthTextureReference(){
	if(!depthTex.bAllocated()){
		ofLogWarning("ofxONI2") << "getDepthTextureReference depth texture not allocated";
	}

	return depthTex;
}

ofTexture & ofxONI1_5::getPlayersTextureReference(){
	return playersTex;
}

ofTexture & ofxONI1_5::getGrayTextureReference(){
	return grayTex;
}

void ofxONI1_5::setUseTexture(bool use_texture){
	bUseTexture = use_texture;
}

void ofxONI1_5::draw(float x, float y, float w, float h){
	if(bUseTexture && bGrabVideo){
		videoTex.draw(x, y, w, h);
	}
}

void ofxONI1_5::drawDepth(float x, float y, float w, float h){
	if(bUseTexture){
		depthTex.draw(x, y, w, h);
	}
}

void ofxONI1_5::drawGrayDepth(float x, float y, float w, float h){
	if(bUseTexture){
		grayTex.draw(x, y, w, h);
	}
}


void ofxONI1_5::draw3D(){
	// not implemented yet
}

float ofxONI1_5::getWidth(){
	return stream_width;
}
float ofxONI1_5::getHeight(){
	return stream_height;
}


bool ofxONI1_5::enableCalibratedRGBDepth(){
	if(!g_image.IsValid()){
		printf("No Image generator found: cannot register viewport");
		return false;
	}

	// Register view point to image map
	if(g_DepthGenerator.IsCapabilitySupported(XN_CAPABILITY_ALTERNATIVE_VIEW_POINT)){

		XnStatus result = g_DepthGenerator.GetAlternativeViewPointCap().SetViewPoint(g_image);
		cout << ofToString((int)result) +  "Register viewport" << endl;
		if(result != XN_STATUS_OK){
			return false;
		}
	}
	else{
		printf("Can't enable calibrated RGB depth, alternative viewport capability not supported");
		return false;
	}

	return true;
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

	g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
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
		g_UserGenerator.GetSkeletonCap().StartTracking(nId);
	}
	else{
		// Calibration failed
		ofLogVerbose("ofxONI1_5") << "UserTracker: calibration failed for user #" << nId;
		g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
	}
}


// Callback for pose detection. Not used.
//
// void XN_CALLBACK_TYPE ofxONI1_5::UserPose_PoseDetected(xn::PoseDetectionCapability & capability, const XnChar * strPose, XnUserID nId, void * pCookie){
// 	printf("Pose %s detected for user %d\n", strPose, nId);
// 	g_UserGenerator.GetPoseDetectionCap().StopPoseDetection(nId);
// 	g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
// }

