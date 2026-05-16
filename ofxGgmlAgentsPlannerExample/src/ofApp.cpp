#include "ofApp.h"

void ofApp::setup() {
	ofSetWindowTitle("ofxGgmlAgents smoke example");
	gui.setup(nullptr, false);
	request.goal = "plan a local creative coding assistant";
	status = ofxGgmlAgentsUtils::describe(request);
	ofLogNotice("ofxGgmlAgentsPlannerExample") << status;
}

void ofApp::draw() {
	ofBackground(18);
	gui.begin();
	ImGui::SetNextWindowPos(ImVec2(24.0f, 24.0f), ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(560.0f, 220.0f), ImGuiCond_Once);
	if (ImGui::Begin("ofxGgmlAgents Planner Example")) {
		ImGui::TextUnformatted("Planning Request");
		ImGui::Separator();
		ImGui::TextWrapped("%s", status.c_str());
	}
	ImGui::End();
	gui.end();
	gui.draw();
}
