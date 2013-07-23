#include "testApp.h"

void testApp::setup() {
	ofSetLogLevel(OF_LOG_VERBOSE);
	ofSetVerticalSync(false);

	depthcam.init();
	depthcam.open();

	ofEnableAlphaBlending();
}

void testApp::update() {
	depthcam.update();
}

void testApp::draw() {
    ofSetColor(0,0,0,255);
    ofDrawBitmapString(ofToString(ofGetFrameRate()),20,20);
    ofSetColor(255,255,255,255);
	depthcam.draw(20,20);
	ofSetColor(255,255,255,100);
	depthcam.drawDepth(20, 20);

	ofSetColor(255,255,255,100);
	depthcam.drawPlayers(20,20);
	depthcam.drawSkeletons(20,20);
}

void testApp::keyPressed(int key) {
}

void testApp::exit()
{

    // On some systems the XnSensorServer does not shut down properly. Being mean and killing it doesn't seem to hurt.
    cout << "killing stuff that won't close" << endl;
    system("killall -9 XnSensorServer");
    system("killall -9 XnVSceneServer_1_5_2");
}
