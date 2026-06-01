#pragma once

#include "ofMain.h"
#include "ofxGgmlAgents.h"
#include "ofxImGui.h"

#include <cstddef>
#include <string>
#include <vector>

class ofApp : public ofBaseApp {
public:
	void setup() override;
	void draw() override;

private:
	struct PlanningScenario {
		std::string name;
		ofxGgmlAgentsRequest request;
		std::vector<std::string> outOfScope;
		std::vector<std::string> validation;
	};

	void buildScenarios();
	void selectScenario(std::size_t index);
	void refreshHandoffText();
	void refreshEndpointStatus();
	void drawScenarioList();
	void drawRequestTab() const;
	void drawHandoffTab() const;
	void drawBoundaryTab() const;
	void drawEndpointTab();
	void logHandoff() const;
	static void drawBullets(const std::vector<std::string> & items);

	std::vector<PlanningScenario> scenarios;
	std::size_t selectedScenario = 0;
	ofxGgmlAgentsRequest request;
	std::string status;
	std::string handoffText;
	std::string endpointBaseUrl;
	std::string endpointModel;
	bool endpointApiKeyConfigured = false;
	ofxImGui::Gui gui;
};
