#include "ofxONI1_5.h"

ofxONI1_5::ofxONI1_5()
{

}

ofxONI1_5::~ofxONI1_5()
{
	g_Context.Shutdown();
}

bool ofxONI1_5::init(bool use_color_image, bool use_texture, bool colorize_depth_image, bool use_players, bool use_skeleton)
{
	XnStatus nRetVal = XN_STATUS_OK;

    bUseTexture = use_texture;
    bGrabVideo = use_color_image;
    bColorizeDepthImage = colorize_depth_image;
	bDrawPlayers = use_players;
	bDrawSkeleton = use_skeleton;

	printf("InitFromXmlFile\n");
	nRetVal = g_Context.InitFromXmlFile(SAMPLE_XML_PATH);
	CHECK_RC(nRetVal, "InitFromXml");

}

bool ofxONI1_5::open()
{
    XnStatus nRetVal = XN_STATUS_OK;

	printf("FindExistingNode\n");
	nRetVal = g_Context.FindExistingNode(XN_NODE_TYPE_DEPTH, g_DepthGenerator);
	CHECK_RC(nRetVal, "Find depth generator");

	if(bDrawPlayers){
	nRetVal = g_Context.FindExistingNode(XN_NODE_TYPE_USER, g_UserGenerator);
	if (nRetVal != XN_STATUS_OK)
	{
		nRetVal = g_UserGenerator.Create(g_Context);
		CHECK_RC(nRetVal, "Find user generator");
	}
	}
	if(bGrabVideo) {
	printf("FindExistingNode\n");
	nRetVal = g_Context.FindExistingNode(XN_NODE_TYPE_IMAGE, g_image);
	CHECK_RC(nRetVal, "Find image generator");
	}

	nRetVal = g_Context.StartGeneratingAll();
	CHECK_RC(nRetVal, "StartGenerating");


	// old
	if(bDrawSkeleton) {
	XnCallbackHandle hUserCBs;

	XnCallbackHandle hUserCallbacks, hCalibrationCallbacks, hPoseCallbacks;
	if (!g_UserGenerator.IsCapabilitySupported(XN_CAPABILITY_SKELETON))
	{
		printf("Supplied user generator doesn't support skeleton\n");
//		return;
	}
	g_UserGenerator.RegisterUserCallbacks(User_NewUser, User_LostUser, NULL, hUserCallbacks);
	g_UserGenerator.GetSkeletonCap().RegisterCalibrationCallbacks(UserCalibration_CalibrationStart, UserCalibration_CalibrationEnd, NULL, hCalibrationCallbacks);

	if (g_UserGenerator.GetSkeletonCap().NeedPoseForCalibration())
	{
		g_bNeedPose = TRUE;
		if (!g_UserGenerator.IsCapabilitySupported(XN_CAPABILITY_POSE_DETECTION))
		{
			printf("Pose required, but not supported\n");
//			return 1;
		}
		g_UserGenerator.GetPoseDetectionCap().RegisterToPoseCallbacks(UserPose_PoseDetected, NULL, NULL, hPoseCallbacks);
		g_UserGenerator.GetSkeletonCap().GetCalibrationPose(g_strPose);
	}

	g_UserGenerator.GetSkeletonCap().SetSkeletonProfile(XN_SKEL_PROFILE_ALL);
	}

	g_DepthGenerator.GetMetaData(depthMD);

    stream_width = depthMD.XRes();
	stream_height = depthMD.YRes();
	ref_max_depth = 0;
	threshold = 3;
    millis = ofGetElapsedTimeMillis();
    counter = 0.0;

    depthPixelsRaw.allocate(stream_width, stream_height, 1);
	depthPixelsRawBack.allocate(stream_width, stream_height, 1);

	videoPixels.allocate(stream_width, stream_height, OF_IMAGE_COLOR);
	videoPixelsBack.allocate(stream_width, stream_height, OF_IMAGE_COLOR);

	depthPixels.allocate(stream_width,stream_height, 1);
	distancePixels.allocate(stream_width, stream_height, 1);
	playersPixels.allocate(stream_width,stream_height, OF_IMAGE_COLOR);

	depthPixelsRaw.set(0);
	depthPixelsRawBack.set(0);

	videoPixels.set(0);
	videoPixelsBack.set(0);

	depthPixels.set(0);
	playersPixels.set(0);
	distancePixels.set(0);

	if(bUseTexture) {
		depthTex.allocate(stream_width, stream_height, GL_RGB);
		videoTex.allocate(stream_width, stream_height, GL_RGB);
        playersTex.allocate(stream_width, stream_height, GL_RGB);
	}

	// MAYBE THIS SHOULD NOT BE HERE
    enableCalibratedRGBDepth();
}

void ofxONI1_5::close() {
	if(isThreadRunning()) {
		stopThread();
		ofSleepMillis(10);
		waitForThread(false);
	}

	bIsFrameNew = false;

	g_Context.Shutdown();
}

void ofxONI1_5::update()
{
    counter = counter + 0.029f;
	g_DepthGenerator.GetMetaData(depthMD);
	g_UserGenerator.GetUserPixels(0, sceneMD);
	g_image.GetMetaData(g_imageMD);

	calculateMaps();

	g_Context.WaitNoneUpdateAll();
}

void ofxONI1_5::calculateMaps()
{
	// Calculate the accumulative histogram

	unsigned int nValue = 0;
	unsigned int nHistValue = 0;
	unsigned int nIndex = 0;
	unsigned int nX = 0;
	unsigned int nY = 0;
	unsigned int nNumberOfPoints = 0;
	const XnDepthPixel* pDepth = depthMD.Data();
	const XnUInt8* pImage = g_imageMD.Data();

	//memset(depthHist, 0, MAX_DEPTH*sizeof(float));
	int n = 0;
	for (nY=0; nY < stream_height; nY++)
	{
		for (nX=0; nX < stream_width; nX++, nIndex++)
		{
			nValue = pDepth[nIndex];

			if (nValue != 0)
			{
				depthHist[nValue]++;
				nNumberOfPoints++;
			}
		}
	}

	for (nIndex=1; nIndex < MAX_DEPTH; nIndex++)
	{
		depthHist[nIndex] += depthHist[nIndex-1];
	}

	if (nNumberOfPoints)
	{
		for (nIndex=1; nIndex < MAX_DEPTH; nIndex++)
		{
			depthHist[nIndex] = (unsigned int)(256 * (1.0f - (depthHist[nIndex] / nNumberOfPoints)));
		}
	}

	const XnLabel* pLabels = sceneMD.Data();
	XnLabel label;
    unsigned char hue;
    ofColor color = ofColor(0,0,0);
    unsigned short *rawpixel = depthPixelsRaw.getPixels();
	unsigned char  *depthpixel = depthPixels.getPixels();
	unsigned char  *playerspixel = playersPixels.getPixels();
	float	       *floatpixel = distancePixels.getPixels();

	for (int i = 0; i < stream_width * stream_height; i++)
	{
		nValue = pDepth[i];
		label = pLabels[i];
		XnUInt32 nColorID = label % nColors;
		if (label == 0)
		{
			nColorID = nColors;
		}

		if (nValue != 0)
		{
            if(ref_max_depth == 0) {
				if(nValue > ref_max_depth) ref_max_depth = nValue;
				cout << "Max depth establised to " << ref_max_depth << endl;
			}

			playerspixel[i * 3 + 0] = 255 * oniColors[nColorID][0];
			playerspixel[i * 3 + 1] = 255 * oniColors[nColorID][1];
			playerspixel[i * 3 + 2] = 255 * oniColors[nColorID][2];

            depthpixel[3*i + 0] = (nValue/256/256)%256;
            depthpixel[3*i + 1] = (nValue/256) %256;
            depthpixel[3*i + 2] = nValue%256;

            floatpixel[i] = depthHist[nValue];

            rawpixel[i] = nValue;

		}
		else
		{

			playerspixel[i * 3 + 0] = 0;
			playerspixel[i * 3 + 1] = 0;
			playerspixel[i * 3 + 2] = 0;

            depthpixel[3*i + 0] = 0;
            depthpixel[3*i + 1] = 0;
            depthpixel[3*i + 2] = 0;

            floatpixel[i] = 0;

            rawpixel[i] = 0;
		}
	}


	const XnRGB24Pixel* pImageRow = g_imageMD.RGB24Data(); // - g_imageMD.YOffset();

	for (XnUInt y = 0; y < stream_height; ++y)
	{
		const XnRGB24Pixel* pImage = pImageRow; // + g_imageMD.XOffset();

		for (XnUInt x = 0; x < stream_width; ++x, ++pImage)
		{
			int index = (y*stream_width + x)*3;
			gColorBuffer[index + 2] = (unsigned char) pImage->nBlue;
			gColorBuffer[index + 1] = (unsigned char) pImage->nGreen;
			gColorBuffer[index + 0] = (unsigned char) pImage->nRed;
		}
		pImageRow += stream_width;
	}

    videoPixels.setFromPixels(gColorBuffer, stream_width, stream_height, OF_IMAGE_COLOR);
    //videoPixels.setFromPixels(gColorBuffer, width, height);

    if(bUseTexture) {
			depthTex.loadData(depthPixels.getPixels(), stream_width, stream_height, GL_RGB);
			videoTex.loadData(videoPixels.getPixels(), stream_width, stream_height, GL_RGB);
			playersTex.loadData(playersPixels.getPixels(), stream_width, stream_height, GL_RGB);
    }

}

void ofxONI1_5::drawPlayers(float x, float y, float w, float h)
{
	playersTex.draw(x, y, w, h);

	XnUserID aUsers[15];
	XnUInt16 nUsers = 15;
	g_UserGenerator.GetUsers(aUsers, nUsers);
	for (int i = 0; i < nUsers; ++i)
	{
		XnPoint3D com;
		g_UserGenerator.GetCoM(aUsers[i], com);
		g_DepthGenerator.ConvertRealWorldToProjective(1, &com, &com);

		ofSetColor(255, 255, 255, 255);
		ofDrawBitmapString(ofToString((int)aUsers[i]), com.X, com.Y);
	}
}

// DRAW SKELETON
void ofxONI1_5::drawSkeletonPt(XnUserID player, XnSkeletonJoint eJoint, int x, int y)
{

	if (!g_UserGenerator.GetSkeletonCap().IsTracking(player))
	{
		printf("not tracked!\n");
		return;
	}

	XnSkeletonJointPosition joint;
	g_UserGenerator.GetSkeletonCap().GetSkeletonJointPosition(player, eJoint, joint);

	if (joint.fConfidence < 0.5)
	{
		return;
	}

	XnPoint3D pt;
	pt = joint.position;
	float ptz = pt.Z;

	float radZ = 25 - ptz/100;
	if(radZ < 3) radZ=3;

	g_DepthGenerator.ConvertRealWorldToProjective(1, &pt, &pt);

	ofPushMatrix();
	ofSetColor(255, 0, 0);
    ofTranslate(x, y);
    //ofTranslate(-width/2, -height/2);
	//ofTranslate(0,0,-pt.Z);
	ofCircle(pt.X, pt.Y, -3);
	ofPopMatrix();

}
void ofxONI1_5::drawSkeletons(int x, int y)
{
	XnUserID aUsers[15];
	XnUInt16 nUsers = 15;
    ofPushMatrix();
	g_UserGenerator.GetUsers(aUsers, nUsers);
	for (int i = 0; i < nUsers; ++i)
	{
		if (g_UserGenerator.GetSkeletonCap().IsTracking(aUsers[i]))
		{
			drawSkeletonPt(aUsers[i], XN_SKEL_HEAD, x, y);
			drawSkeletonPt(aUsers[i], XN_SKEL_NECK, x, y);
			drawSkeletonPt(aUsers[i], XN_SKEL_LEFT_SHOULDER, x, y);
			drawSkeletonPt(aUsers[i], XN_SKEL_LEFT_ELBOW, x, y);
			drawSkeletonPt(aUsers[i], XN_SKEL_LEFT_HAND, x, y);
			drawSkeletonPt(aUsers[i], XN_SKEL_RIGHT_SHOULDER, x, y);
			drawSkeletonPt(aUsers[i], XN_SKEL_RIGHT_ELBOW, x, y);
			drawSkeletonPt(aUsers[i], XN_SKEL_RIGHT_HAND, x, y);
			drawSkeletonPt(aUsers[i], XN_SKEL_TORSO, x, y);
			drawSkeletonPt(aUsers[i], XN_SKEL_LEFT_HIP, x, y);
			drawSkeletonPt(aUsers[i], XN_SKEL_LEFT_KNEE, x, y);
			drawSkeletonPt(aUsers[i], XN_SKEL_LEFT_FOOT, x, y);
			drawSkeletonPt(aUsers[i], XN_SKEL_RIGHT_HIP, x, y);
			drawSkeletonPt(aUsers[i], XN_SKEL_RIGHT_KNEE, x, y);
			drawSkeletonPt(aUsers[i], XN_SKEL_RIGHT_FOOT, x, y);
//			DrawLimb(aUsers[i], XN_SKEL_LEFT_HIP, XN_SKEL_RIGHT_HIP);

		}
	}
	ofPopMatrix();
}

bool ofxONI1_5::isConnected() {
	return isThreadRunning();
}

bool ofxONI1_5::isFrameNew() {
	if(isThreadRunning() && bIsFrameNew) {
		bIsFrameNew = false;
		return true;
	} else {
		return false;
	}
}

unsigned char* ofxONI1_5::getPixels() {
	return videoPixels.getPixels();
}

unsigned char* ofxONI1_5::getDepthPixels() {
	return depthPixels.getPixels();
}

unsigned short* ofxONI1_5::getRawDepthPixels() {
	return depthPixelsRaw.getPixels();
}

float* ofxONI1_5::getDistancePixels() {
	return distancePixels.getPixels();
}

ofPixels& ofxONI1_5::getPixelsRef() {
	return videoPixels;
}

ofPixels& ofxONI1_5::getDepthPixelsRef() {
	return depthPixels;
}

ofShortPixels& ofxONI1_5::getRawDepthPixelsRef() {
	return depthPixelsRaw;
}

ofFloatPixels& ofxONI1_5::getDistancePixelsRef() {
	return distancePixels;
}

ofTexture& ofxONI1_5::getTextureReference() {
	if(!videoTex.bAllocated()) {
		ofLogWarning("ofxONI2") << "getTextureReference video texture not allocated";
	}

	return videoTex;
}

ofTexture& ofxONI1_5::getDepthTextureReference() {
	if(!depthTex.bAllocated()) {
		ofLogWarning("ofxONI2") << "getDepthTextureReference depth texture not allocated";
	}

	return depthTex;
}

void ofxONI1_5::setUseTexture(bool use_texture) {
	bUseTexture = use_texture;
}

void ofxONI1_5::draw(float x, float y, float w, float h) {
	if(bUseTexture && bGrabVideo) {
		videoTex.draw(x,y,w,h);
	}
}

void ofxONI1_5::drawDepth(float x, float y, float w, float h) {
	if(bUseTexture) {
		depthTex.draw(x,y,w,h);
	}
}

void ofxONI1_5::draw3D() {
	// not implemented yet
	draw(0,0);
}

float ofxONI1_5::getWidth() { return stream_width; }
float ofxONI1_5::getHeight() { return stream_height; }


bool ofxONI1_5::enableCalibratedRGBDepth(){
	if (!g_image.IsValid()) {
		printf("No Image generator found: cannot register viewport");
		return false;
	}

	// Register view point to image map
	if(g_DepthGenerator.IsCapabilitySupported(XN_CAPABILITY_ALTERNATIVE_VIEW_POINT)){

		XnStatus result = g_DepthGenerator.GetAlternativeViewPointCap().SetViewPoint(g_image);
		cout << ofToString((int) result) +  "Register viewport" << endl;
		if(result!=XN_STATUS_OK){
			return false;
		}
	}else{
		printf("Can't enable calibrated RGB depth, alternative viewport capability not supported");
		return false;
	}

	return true;
}
