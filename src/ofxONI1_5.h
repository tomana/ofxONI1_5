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

		// Draw player image
		void drawPlayers(float x, float y, float w, float h);
		void drawPlayers(float x, float y){ drawPlayers(x, y, stream_width, stream_height); }
		void drawPlayers(const ofPoint & point){ drawPlayers(point.x, point.y); }
		void drawPlayers(const ofRectangle & rect){ drawPlayers(rect.x, rect.y, rect.width, rect.height); }

		// Draw gray depth texture
		void drawGrayDepth(float x, float y, float w, float h);
		void drawGrayDepth(float x, float y){ drawGrayDepth(x, y, stream_width, stream_height); }
		void drawGrayDepth(const ofPoint & point){ drawGrayDepth(point.x, point.y); }
		void drawGrayDepth(const ofRectangle & rect){ drawGrayDepth(rect.x, rect.y, rect.width, rect.height); }

		// Not implemented. Should probably not be here.
		void draw3D();

		float getWidth();
		float getHeight();

		ofVec3f toOf(XnVector3D p) { return ofVec3f(p.X, p.Y, p.Z); }

		// Functions for skeleton parts.
		// void drawSkeletonPt(XnUserID player, XnSkeletonJoint eJoint, int x, int y);
		// void drawSkeletons(int x, int y);

		xn::SceneMetaData sceneMD;
		xn::SceneMetaData playerMD;
		xn::DepthMetaData depthMD;
		xn::ImageMetaData oniImageGeneratorMD;

		int stream_width, stream_height;

		float ref_max_depth;

		bool enableCalibratedRGBDepth();

		//Grid for shader
		ofVboMesh kinect_grid;

		void generate_grid();

		// clear resources
		void clear();

		ofEvent<short> newUserEvent;
		ofEvent<short> lostUserEvent;

		struct UserData {
			// ofVec3f boundingBoxMin;
			// ofVec3f boundingBoxMax;
			ofVec3f centerOfMass;
			short id;
			// bool isVisible;
			bool isSkeletonAvailable;
			map<XnSkeletonJoint,ofVec3f> skeletonPoints;

		};

	protected:

		static xn::Context oniContext;
		static xn::DepthGenerator oniDepthGenerator;
		static xn::UserGenerator oniUserGenerator;
		static xn::ImageGenerator oniImageGenerator;

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

		vector<UserData> userData;
		vector<XnSkeletonJoint> trackedJoints;
};
