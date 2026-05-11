#include "ofApp.h"

void ofApp::setup() {
	ofSetWindowTitle("ofxGgmlAgents smoke example");
	request.goal = "plan a local creative coding assistant";
	status = ofxGgmlAgentsUtils::describe(request);
	ofLogNotice("ofxGgmlAgentsPlannerExample") << status;
}

void ofApp::draw() {
	ofBackground(18);
	ofSetColor(240);
	ofDrawBitmapString("ofxGgmlAgents", 32, 48);
	ofDrawBitmapString(status, 32, 78);
}