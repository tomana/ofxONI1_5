#include "ofMain.h"
#undef Status
#undef STATUS
#include "XnOpenNI.h"
#include "XnCodecIDs.h"
#include "XnCppWrapper.h"

#include "ofxBase3DVideo.h"

class ofxONI1_5 : public ofxBase3DVideo {
	public:
		ofxONI1_5();
		~ofxONI1_5();
		
		// Enable/disable features. These should be run before init()
		void setUseTexture(bool b); // Defaults to true
		void setUseColorImage(bool b); // Defaults to true
		void setUseColorizedDepthImage(bool b); // Defaults to true
		void setUseCalibratedRGBDepth(bool b); // Defaults to true
		void setUseUserTracker(bool b); // Defaults to true
		void setUseUserMap(bool b); // Defaults to true
		void setUseUserMapImage(bool b); // Defaults to true
		void setUseSkeletonTracker(bool b); // Defaults to true

		bool init();
		bool open();
		void update();
		void close();
		void clear();

		bool isConnected();

		// is the current frame new? This is positive when
		// either the depth frame or the color image is updated.
		bool isFrameNew();


		// Raw pixel pointer for the color image:
		unsigned char* getPixels() { return colorPixels.getPixels(); }

		// Raw pixel pointer for the (hue colorized or grayscale) depth image:
		unsigned char* getDepthPixels() { return depthPixels.getPixels(); }

		// Raw pixel pointer for the raw 16 bit depth image (in mm)
		// OpenNI uses short for depth data.
		unsigned short* getRawDepthPixels() { return depthPixelsRaw.getPixels(); }

		// Raw pixel pointer to float millimeter distance image:
		float* getDistancePixels() { return distancePixels.getPixels(); }

		// ofPixel objects
		ofPixels& getPixelsRef() { return colorPixels; }
		ofPixels& getDepthPixelsRef() { return depthPixels; }
		ofShortPixels& getRawDepthPixelsRef() { return depthPixelsRaw; }
		ofFloatPixels& getDistancePixelsRef() { return distancePixels; }
		ofShortPixels& getUserMapRef() { return userMap; }

		// Textures:
		ofTexture& getTextureReference() { return colorTex; }
		ofTexture& getDepthTextureReference() { return depthTex; }
		ofTexture& getUserMapImageTextureReference() { return userMapImageTex; }

		// Draw the color texture
		void draw(float x, float y, float w, float h);
		void draw(float x, float y){ draw(x, y, stream_width, stream_height); }
		void draw(const ofPoint & point){ draw(point.x, point.y); }
		void draw(const ofRectangle & rect){ draw(rect.x, rect.y, rect.width, rect.height); }

		// Draw the depth texture
		void drawDepth(float x, float y, float w, float h);
		void drawDepth(float x, float y){ drawDepth(x, y, stream_width, stream_height); }
		void drawDepth(const ofPoint & point){ drawDepth(point.x, point.y); }
		void drawDepth(const ofRectangle & rect){ drawDepth(rect.x, rect.y, rect.width, rect.height); }

		// Draw user image
		void drawUsers(float x, float y, float w, float h);
		void drawUsers(float x, float y){ drawUsers(x, y, stream_width, stream_height); }
		void drawUsers(const ofPoint & point){ drawUsers(point.x, point.y); }
		void drawUsers(const ofRectangle & rect){ drawUsers(rect.x, rect.y, rect.width, rect.height); }

		// Draw skeleton overlay image
		void drawSkeletonOverlay(float x, float y, float w, float h);
		void drawSkeletonOverlay(float x, float y){ drawSkeletonOverlay(x, y, stream_width, stream_height); }
		void drawSkeletonOverlay(const ofPoint & point){ drawSkeletonOverlay(point.x, point.y); }
		void drawSkeletonOverlay(const ofRectangle & rect){ drawSkeletonOverlay(rect.x, rect.y, rect.width, rect.height); }

		string& getUserTrackerDebugString() { return stringUserTrackerDebug; }

		float getWidth();
		float getHeight();

		ofVec3f toOf(XnVector3D p) { return ofVec3f(p.X, p.Y, p.Z); }

		// Convert OpenNI "real" coordinates to camera pixel coordinates and the reverse
		ofVec3f coordsRealToProjective(ofVec3f v);
		ofVec3f coordsProjectiveToReal(ofVec3f v);


		ofEvent<short> newUserEvent;
		ofEvent<short> lostUserEvent;

		vector<UserData>& getUserData() { return userData; }
		struct UserData {
			short id;
			ofVec3f centerOfMass;
			bool isSkeletonAvailable;
			map<XnSkeletonJoint,ofVec3f> skeletonPoints;

		};

	protected:

		xn::Context oniContext;
		xn::DepthGenerator oniDepthGenerator;
		xn::UserGenerator oniUserGenerator;
		xn::ImageGenerator oniImageGenerator;

		xn::SceneMetaData sceneMD;
		xn::SceneMetaData playerMD;
		xn::DepthMetaData depthMD;
		xn::ImageMetaData oniImageGeneratorMD;

		int stream_width, stream_height;
		float ref_max_depth;

		bool bInited;
		bool bIsConnected;
		bool bIsFrameNew;

		bool bUseTexture;
		bool bUseColorImage;
		bool bUseColorizedDepthImage;
		bool bUseCalibratedRGBDepth;
		bool bUseUserTracker;
		bool bUseUserMap;
		bool bUseUserMapImage;
		bool bUseSkeletonTracker;

		bool bDepthOn;
		bool bColorOn;
		bool bUserTrackerOn;
		bool bSkeletonTrackerOn;

		string stringUserTrackerDebug;

		ofTexture colorTex;
		ofTexture depthTex;
		ofTexture userMapImageTex;

		ofPixels colorPixels;
		ofPixels depthPixels;

		ofShortPixels depthPixelsRaw;
		ofFloatPixels distancePixels;
		ofShortPixels userMap;

		void updateDepth();
		void updateColor();
		void updateUserTracker();

		void enableCalibratedRGBDepth();

		vector<UserData> userData;
		vector<XnSkeletonJoint> trackedJoints;


		//
		// OpenNI callback functions must be static, but pCookie can be used
		// to send a pointer to the current instance. This is used in turn to 
		// call the cb...() funcions.
		//
		static void XN_CALLBACK_TYPE User_NewUser(xn::UserGenerator & generator, XnUserID nId, void * pCookie);
		void cbNewUser(xn::UserGenerator & generator, XnUserID nId);

		static void XN_CALLBACK_TYPE User_LostUser(xn::UserGenerator & generator, XnUserID nId, void * pCookie);
		void cbLostUser(xn::UserGenerator & generator, XnUserID nId);

		static void XN_CALLBACK_TYPE UserCalibration_CalibrationStart(xn::SkeletonCapability & capability, XnUserID nId, void * pCookie);
		void cbUserCalibrationStart(xn::SkeletonCapability & capability, XnUserID nId);

		static void XN_CALLBACK_TYPE UserCalibration_CalibrationEnd(xn::SkeletonCapability & capability, XnUserID nId, XnBool bSuccess, void * pCookie);
		void cbUserCalibrationEnd(xn::SkeletonCapability & capability, XnUserID nId, XnBool bSuccess);

		// static void XN_CALLBACK_TYPE UserPose_PoseDetected(xn::PoseDetectionCapability & capability, const XnChar * strPose, XnUserID nId, void * pCookie);
		// void cbUserPoseDetected(xn::PoseDetectionCapability & capability, const XnChar * strPose, XnUserID nId);

};


static XnFloat oniColors[][3] = {
	{0, 0, 0},
	{0, 1, 1},
	{0, 1, 0},
	{0, 0, 1},
	{1, 1, 0},
	{1, 0, 0},
	{1, .5, 0},
	{.5, 1, 0},
	{0, .5, 1},
	{.5, 0, 1},
	{1, 1, .5},
};

static XnUInt32 nColors = 10;
