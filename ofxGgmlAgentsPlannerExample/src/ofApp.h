#pragma once

#include "ofMain.h"
#include "ofxGgmlAgents.h"
#include "ofxImGui.h"

class ofApp : public ofBaseApp {
public:
	void setup() override;
	void draw() override;

private:
	ofxGgmlAgentsRequest request;
	std::string status;
	ofxImGui::Gui gui;
};
