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
    depthcam.drawUsers(640+20,20);
    depthcam.drawSkeletonOverlay(20,20);
    depthcam.drawSkeletonOverlay(640+20,20);

    ofSetColor(0,0,0,255);
    ofDrawBitmapString(depthcam.getUserTrackerDebugString(),20,480+20+20);
}


void testApp::keyPressed(int key) {
}

void testApp::exit() {
}


