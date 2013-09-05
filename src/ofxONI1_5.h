#include "ofMain.h"
#undef Status
#undef STATUS
#include "XnOpenNI.h"
#include "XnCodecIDs.h"
#include "XnCppWrapper.h"


#define CHECK_RC(nRetVal, what)										\
	if(nRetVal != XN_STATUS_OK)									   \
	{																\
		printf("%s failed: %s\n", what, xnGetStatusString(nRetVal)); \
	}

static XnFloat oniColors[][3] = {
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
	{0, 0, 0}
};

static XnUInt32 nColors = 10;

#include "ofxBase3DVideo.h"

class ofxONI1_5 : public ofxBase3DVideo {
	public:
		ofxONI1_5();
		~ofxONI1_5();

		bool init(bool use_color_image = true, bool use_texture = true, bool colorize_depth_image = true, bool use_players = true, bool use_skeleton = true);
		bool open();
		void update();

		// close connection and stop grabbing images
		void close();
		// close generators
		void stopGenerators();

		// is the connection currently open?
		bool isConnected();

		// is the current frame new? This is positive when
		// either the depth frame or the color image is updated.
		bool isFrameNew();
		// Raw pixel pointer for the color image:
		unsigned char * getPixels();

		// Raw pixel pointer for the (hue colorized or grayscale) depth image:
		unsigned char * getDepthPixels();

		// Raw pixel pointer for the raw 16 bit depth image (in mm)
		// OpenNI 2.2 uses typedef uint16_t openni::DepthPixel
		unsigned short * getRawDepthPixels();
		//
		unsigned char * getPlayersPixels();
		// Raw pixel pointer to float millimeter distance image:
		float * getDistancePixels();

		// ofPixel objects
		ofPixels & getPixelsRef();
		ofPixels & getDepthPixelsRef();

		ofShortPixels & getRawDepthPixelsRef();
		ofFloatPixels & getDistancePixelsRef();

		// Textures:
		// Color image as texture
		ofTexture & getTextureReference();
		// Depth image as texture
		ofTexture & getDepthTextureReference();
		//
		ofTexture & getPlayersTextureReference();
		// Get gray reference
		ofTexture & getGrayTextureReference();

		// Enable/disable texture updates
		void setUseTexture(bool use_texture);

		// Draw the video texture
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


		// Functions for skeleton parts.
		// void drawSkeletonPt(XnUserID player, XnSkeletonJoint eJoint, int x, int y);
		// void drawSkeletons(int x, int y);

		xn::SceneMetaData sceneMD;
		xn::SceneMetaData playerMD;
		xn::DepthMetaData depthMD;
		xn::ImageMetaData g_imageMD;

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

	protected:

		static xn::Context g_Context;
		static xn::DepthGenerator g_DepthGenerator;
		static xn::UserGenerator g_UserGenerator;
		static xn::ImageGenerator g_image;

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

		static XnBool g_bNeedPose;
		static XnChar g_strPose[20];

		// Players
		XnUserID * aUsers;
		XnUInt16 nUsers;
		XnPoint3D * com;
		//

		bool bUseTexture;
		bool bGrabVideo;
		bool bColorizeDepthImage;
		bool bDrawSkeleton;
		bool bDrawPlayers;

		bool bDepthOn;
		bool bColorOn;
		bool bUserTrackerOn;
		bool bIsConnected;

		bool bIsFrameNew;
		bool bNeedsUpdateColor;
		bool bNeedsUpdateDepth;
		bool bUpdateTex;

		ofTexture videoTex;
		ofTexture depthTex;
		ofTexture playersTex;
		ofTexture grayTex;

		bool bGrabberInited;


		ofPixels videoPixels;
		ofPixels depthPixels;
		ofPixels playersPixels;
		ofPixels grayPixels;

		ofShortPixels depthPixelsRaw;
		ofFloatPixels distancePixels;

		void updateDepth();
		void updateColor();
		void updateUserTracker();

};
