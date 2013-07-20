#pragma once

#include "ofMain.h"
#include "XnOpenNI.h"
#include "XnCodecIDs.h"
#include "XnCppWrapper.h"
#include "ofxOpenCv.h"

static xn::Context g_Context;
static xn::DepthGenerator g_DepthGenerator;
static xn::UserGenerator g_UserGenerator;
static xn::ImageGenerator g_image;

#define CHECK_RC(nRetVal, what)										\
	if (nRetVal != XN_STATUS_OK)									\
	{																\
		printf("%s failed: %s\n", what, xnGetStatusString(nRetVal));\
	}

static XnBool g_bNeedPose = FALSE;
static XnChar g_strPose[20] = "";

static void XN_CALLBACK_TYPE User_NewUser(xn::UserGenerator& generator, XnUserID nId, void* pCookie)
{
	printf("New User %d\n", nId);
	// New user found
	if (g_bNeedPose)
	{
		g_UserGenerator.GetPoseDetectionCap().StartPoseDetection(g_strPose, nId);
	}
	else
	{
		g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
	}
};
static void XN_CALLBACK_TYPE User_LostUser(xn::UserGenerator& generator, XnUserID nId, void* pCookie)
{
	printf("Lost User id: %i\n", (unsigned int)nId);
};

// Callback: Detected a pose
static void XN_CALLBACK_TYPE UserPose_PoseDetected(xn::PoseDetectionCapability& capability, const XnChar* strPose, XnUserID nId, void* pCookie)
{
	printf("Pose %s detected for user %d\n", strPose, nId);
	g_UserGenerator.GetPoseDetectionCap().StopPoseDetection(nId);
	g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
};
// Callback: Started calibration
static void XN_CALLBACK_TYPE UserCalibration_CalibrationStart(xn::SkeletonCapability& capability, XnUserID nId, void* pCookie)
{
	printf("Calibration started for user %d\n", nId);
};
// Callback: Finished calibration
static void XN_CALLBACK_TYPE UserCalibration_CalibrationEnd(xn::SkeletonCapability& capability, XnUserID nId, XnBool bSuccess, void* pCookie)
{
	if (bSuccess)
	{
		// Calibration succeeded
		printf("Calibration complete, start tracking user %d\n", nId);
		g_UserGenerator.GetSkeletonCap().StartTracking(nId);
	}
	else
	{
		// Calibration failed
		printf("Calibration failed for user %d\n", nId);
		if (g_bNeedPose)
		{
			g_UserGenerator.GetPoseDetectionCap().StartPoseDetection(g_strPose, nId);
		}
		else
		{
			g_UserGenerator.GetSkeletonCap().RequestCalibration(nId, TRUE);
		}
	}
};


//#define SAMPLE_XML_PATH "data/Sample-User.xml"
#define SAMPLE_XML_PATH "../bin/data/SamplesConfig.xml"

#define MAX_DEPTH 10000

static XnFloat oniColors[][3] =
{
	{0,1,1},
	{0,0,1},
	{0,1,0},
	{1,1,0},
	{1,0,0},
	{1,.5,0},
	{.5,1,0},
	{0,.5,1},
	{.5,0,1},
	{1,1,.5},
	{0,0,0}
};
static XnUInt32 nColors = 10;

#include "ofxBase3DVideo.h"

class ofxONI1_5 : public ofxBase3DVideo, protected ofThread {
    public:
        ofxONI1_5();
        ~ofxONI1_5();

        bool init(bool use_color_image = true, bool use_texture = true, bool colorize_depth_image = true, bool use_players = true, bool use_skeleton = true);
        bool open();
		void update();

		// close connection and stop grabbing images
		void close();

		// is the connection currently open?
		bool isConnected();

		// is the current frame new? This is positive when
		// either the depth frame or the color image is updated.
		bool isFrameNew();
		// Raw pixel pointer for the color image:
		unsigned char* getPixels();

		// Raw pixel pointer for the (hue colorized or grayscale) depth image:
		unsigned char* getDepthPixels();

		// Raw pixel pointer for the raw 16 bit depth image (in mm)
		// OpenNI 2.2 uses typedef uint16_t openni::DepthPixel
		unsigned short* getRawDepthPixels();

		// Raw pixel pointer to float millimeter distance image:
		float* getDistancePixels();

		// ofPixel objects
		ofPixels& getPixelsRef();
		ofPixels& getDepthPixelsRef();
		ofShortPixels& getRawDepthPixelsRef();
		ofFloatPixels& getDistancePixelsRef();

        // Textures:
		// Color image as texture
		ofTexture& getTextureReference();
		// Depth image as texture
		ofTexture& getDepthTextureReference();

        // Enable/disable texture updates
		void setUseTexture(bool use_texture);

        void draw(float x, float y, float w, float h);
		void draw(float x, float y);
		void draw(const ofPoint& point);
		void draw(const ofRectangle& rect);

        // Draw the depth texture
		void drawDepth(float x, float y, float w, float h);
		void drawDepth(float x, float y);
		void drawDepth(const ofPoint& point);
		void drawDepth(const ofRectangle& rect);

        void draw3D();

        float getWidth();
		float getHeight();

	//	void drawDepth(int x, int y) {drawDepth(x, y, width, height);};
	//	void drawDepth(int x, int y, int w, int h);
		void drawPlayers(int x, int y) {drawPlayers(x, y, width, height);};
		void drawPlayers(int x, int y, int w, int h);;
		void drawCam(int x, int y) {drawCam(x, y, width, height);};
		void drawCam(int x, int y, int w, int h);

		// EGET
		void drawContour(int x, int y, int threshold) {drawContour(x, y, width, height, threshold);};
		void drawContour(int x, int y, int w, int h, int threshold);
        void drawPointCloud(int x, int y, float depthCoeff, float differenceCoeff, int stepx, int stepy);

		void drawSkeletonPt(XnUserID player, XnSkeletonJoint eJoint, int x, int y);
		void skeletonTracking(int x, int y);

		void calculateMaps();

		xn::SceneMetaData sceneMD;
		// EGET
		xn::SceneMetaData playerMD;
		xn::DepthMetaData depthMD;
		xn::ImageMetaData g_imageMD;

		ofxCvGrayscaleImage depth;
		// eget
		ofxCvContourFinder 	contourFinder;
		ofxCvGrayscaleImage grayImage;
		ofxCvGrayscaleImage grayDiff;

        ofImage shaderdepth;
        ofImage refcolordepth;
        ofxCvColorImage colordepth;
		ofImage players;
		ofxCvColorImage imgCam;
        int millis;

		float depthHist[MAX_DEPTH];
		unsigned char gColorBuffer[640*480*3];         // BGRA

		unsigned char * tmpGrayPixels;
		unsigned char * tmpColorPixels;
		unsigned char * tmpRefColorPixels;
		unsigned char * tmpShaderPixels;
		unsigned char * tmpCamColorPixels;

        bool color_depth_bool;

		XnPoint3D LHandPoint;
		XnPoint3D RHandPoint;

        float counter;

		int width, height;
		int stream_width, stream_height;
		int threshold;

		// maxdepth
		float ref_max_depth;

		//Calib

		bool enableCalibratedRGBDepth();

		//Grid for shader
		ofVboMesh kinect_grid;

		void generate_grid();


    protected:
        bool bUseTexture;
        bool bGrabVideo;
        bool bColorizeDepthImage;
        bool bDrawSkeleton;
        bool bDrawPlayers;

        ofTexture videoTex;
		ofTexture depthTex;
		bool bGrabberInited;

		bool bIsFrameNew;

		ofPixels videoPixels;
		ofPixels depthPixels;
		ofPixels usersPixels;

		ofShortPixels depthPixelsRaw;
		ofFloatPixels distancePixels;

        ofShortPixels depthPixelsRawBack;
		ofPixels videoPixelsBack;

};

