#pragma once

#include "ofMain.h"
#include "ofxGgmlAgents.h"
#include "ofxGgmlRag.h"
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
		std::string selectedBackend;
		std::string endpointBaseUrl;
		std::string resourcePath;
		std::string toolName;
		std::string toolCallEncoding;
		std::vector<std::string> capabilityStatuses;
		std::string ragQuery;
		std::string ragContext;
		std::vector<std::string> ragReferences;
		std::string finalConfirmation;
		std::vector<std::string> events;
		int elapsedMs = 0;
	};
	struct EndpointCheckResult {
		bool success = false;
		bool modelAvailable = false;
		int httpStatus = 0;
		int elapsedMs = 0;
		std::string selectedBackend;
		std::string endpointBaseUrl;
		std::string requestedModel;
		std::vector<std::string> advertisedModels;
		std::string error;
	};
	static EndpointCheckResult checkEndpointForSmoke(const std::string & baseUrl, const std::string & model,
		const std::string & apiKey, const std::string & selectedBackend = "configured");
	static ToolLoopResult runToolLoopForSmoke(const std::string & baseUrl, const std::string & model,
		const std::string & apiKey, const std::string & ecosystemPath,
		const std::string & selectedBackend = "configured");
	static ToolLoopResult runLocalRagToolLoopForSmoke(const std::string & baseUrl, const std::string & model,
		const std::string & apiKey, const std::string & sourceRoot, const std::string & query,
		const std::string & selectedBackend = "configured");

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
	bool selectedEndpointUsesSharedFallback() const;
	std::string selectedBackendKey() const;
	std::string selectedBackendDisplay() const;
	void drawScenarioList();
	void drawRequestTab() const;
	void drawHandoffTab() const;
	void drawBoundaryTab() const;
	void drawEndpointTab();
	void startEndpointCheck();
	void startToolLoop();
	void invalidateToolLoopResult();
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
	std::string ragSourceRoot;
	std::string ragQuery = "tool orchestration";
	bool endpointApiKeyConfigured = false;
	bool cpuEndpointUsesSharedFallback = false;
	bool cudaEndpointUsesSharedFallback = false;
	std::array<char, 512> cpuEndpointBaseUrlInput{};
	std::array<char, 256> cpuEndpointModelInput{};
	std::array<char, 512> cudaEndpointBaseUrlInput{};
	std::array<char, 256> cudaEndpointModelInput{};
	std::array<char, 512> ragQueryInput{};

	enum class BackendMode { Cpu, Cuda };
	BackendMode backendMode = BackendMode::Cuda;

	enum class ToolMode { EcosystemLanes, LocalRagCorpus };
	ToolMode toolMode = ToolMode::EcosystemLanes;

	enum class ToolLoopState { Idle, Running, Succeeded, Failed };
	ToolLoopState toolLoopState = ToolLoopState::Idle;
	ToolLoopResult toolLoopResult;
	std::future<ToolLoopResult> toolLoopFuture;
	enum class EndpointCheckState { Idle, Running, Reachable, Failed };
	EndpointCheckState endpointCheckState = EndpointCheckState::Idle;
	EndpointCheckResult endpointCheckResult;
	std::future<EndpointCheckResult> endpointCheckFuture;
	ofxImGui::Gui gui;
};
