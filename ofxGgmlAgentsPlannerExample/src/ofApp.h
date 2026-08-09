#pragma once

#include "ofMain.h"
#include "ofxGgmlAgents.h"
#include "ofxImGui.h"

#include <cstddef>
#include <array>
#include <future>
#include <string>
#include <vector>

class ofApp : public ofBaseApp {
public:
	void setup() override;
	void update() override;
	void draw() override;
	struct ToolLoopResult {
		bool success = false;
		std::string error;
		std::string toolCallEncoding;
		std::vector<std::string> provenLanes;
		std::string finalConfirmation;
		std::vector<std::string> events;
		int elapsedMs = 0;
	};
	static ToolLoopResult runToolLoopForSmoke(const std::string & baseUrl, const std::string & model,
		const std::string & apiKey, const std::string & ecosystemPath);

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
	void startToolLoop();
	void logHandoff() const;
	static void drawBullets(const std::vector<std::string> & items);

	std::vector<PlanningScenario> scenarios;
	std::size_t selectedScenario = 0;
	ofxGgmlAgentsRequest request;
	std::string status;
	std::string handoffText;
	std::string endpointBaseUrl;
	std::string endpointModel;
	std::string endpointApiKey;
	std::string ecosystemPath;
	bool endpointApiKeyConfigured = false;
	std::array<char, 512> endpointBaseUrlInput{};
	std::array<char, 256> endpointModelInput{};

	enum class ToolLoopState { Idle, Running, Succeeded, Failed };
	ToolLoopState toolLoopState = ToolLoopState::Idle;
	ToolLoopResult toolLoopResult;
	std::future<ToolLoopResult> toolLoopFuture;
	ofxImGui::Gui gui;
};
