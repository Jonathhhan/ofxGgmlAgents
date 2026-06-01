#include "ofApp.h"

#include <cstdlib>
#include <sstream>

namespace {
	const char * kLogModule = "ofxGgmlAgentsPlannerExample";

	void appendList(std::ostringstream & stream, const std::vector<std::string> & items) {
		if (items.empty()) {
			stream << "none\n";
			return;
		}

		for (std::size_t i = 0; i < items.size(); ++i) {
			if (i > 0) {
				stream << "; ";
			}
			stream << items[i];
		}
		stream << "\n";
	}

	std::string readEnvironmentValue(const char * name) {
#if defined(_MSC_VER)
		char * value = nullptr;
		size_t length = 0;
		if (_dupenv_s(&value, &length, name) != 0 || value == nullptr) {
			return "";
		}
		std::string result(value);
		std::free(value);
		return result;
#else
		const char * value = std::getenv(name);
		if (value == nullptr) {
			return "";
		}
		return value;
#endif
	}

	const char * configuredText(bool configured) {
		return configured ? "yes" : "no";
	}

	const char * displayValue(const std::string & value) {
		return value.empty() ? "(not set)" : value.c_str();
	}
}

void ofApp::setup() {
	ofSetWindowTitle("ofxGgmlAgents planner example");
	gui.setup(nullptr, false);
	refreshEndpointStatus();
	buildScenarios();
	selectScenario(0);
	logHandoff();
}

void ofApp::draw() {
	ofBackground(18, 20, 22);
	gui.begin();
	ImGui::SetNextWindowPos(ImVec2(24.0f, 24.0f), ImGuiCond_Once);
	float windowWidth = static_cast<float>(ofGetWidth()) - 48.0f;
	float windowHeight = static_cast<float>(ofGetHeight()) - 48.0f;
	if (windowWidth < 720.0f) {
		windowWidth = 720.0f;
	}
	if (windowHeight < 420.0f) {
		windowHeight = 420.0f;
	}
	ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight), ImGuiCond_Always);
	if (ImGui::Begin("ofxGgmlAgents Planner Example")) {
		ImGui::TextWrapped("%s", status.c_str());
		ImGui::Separator();

		drawScenarioList();
		ImGui::SameLine();

		ImGui::BeginChild("planning-detail", ImVec2(0.0f, 0.0f), true);
		if (ImGui::BeginTabBar("PlanningTabs")) {
			if (ImGui::BeginTabItem("Request")) {
				drawRequestTab();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Handoff")) {
				drawHandoffTab();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Boundary")) {
				drawBoundaryTab();
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Endpoint")) {
				drawEndpointTab();
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
		ImGui::EndChild();
	}
	ImGui::End();
	gui.end();
	gui.draw();
}

void ofApp::buildScenarios() {
	PlanningScenario assistant;
	assistant.name = "Creative coding assistant";
	assistant.request.goal = "plan a local creative coding assistant";
	assistant.request.prompt = "Draft the workflow boundary before adding model-backed execution.";
	assistant.request.context = "The user wants an assistant loop that can inspect an openFrameworks addon, propose scoped edits, and hand work to companion addons for model-specific capabilities.";
	assistant.request.tools = {
		"ofxGgmlLlama: OpenAI-compatible local text endpoint handoff",
		"ofxGgmlCore: shared runtime primitives only after they are stable",
		"ofxGgmlAgents: planning, tool registry shape, and validation records"
	};
	assistant.outOfScope = {
		"Starting llama.cpp or downloading GGUF models",
		"Writing memory indexes, generated media, or runtime caches",
		"Adding reverse dependencies from ofxGgmlCore to companion addons"
	};
	assistant.validation = {
		"scripts\\doctor-agents.bat",
		"scripts\\run-agents-runtime-smoke.bat -Json -SummaryOnly",
		"scripts\\validate-local.bat"
	};
	scenarios.push_back(assistant);

	PlanningScenario toolRegistry;
	toolRegistry.name = "Companion tool registry";
	toolRegistry.request.goal = "describe how companion addons expose tools to an agent";
	toolRegistry.request.prompt = "Sketch the registry contract without executing external tools.";
	toolRegistry.request.context = "A companion addon owns model UX and local setup. Agents records the callable boundary, expected inputs, output summary, cleanup rule, and validation command.";
	toolRegistry.request.tools = {
		"Tool descriptor: name, owning addon, inputs, outputs, and cleanup policy",
		"Planner record: requested tool, reason, expected side effects, and validation",
		"Doctor script: confirms layout readiness without reading private sessions"
	};
	toolRegistry.outOfScope = {
		"Shell execution loops",
		"Provider-specific authentication",
		"Reusable Actions policy that belongs in ofxGgmlWorkflows"
	};
	toolRegistry.validation = {
		"scripts\\test-addon.bat",
		"scripts\\validate-local.bat"
	};
	scenarios.push_back(toolRegistry);

	PlanningScenario endpoint;
	endpoint.name = "Local LLM endpoint handoff";
	endpoint.request.goal = "record a user-provided local LLM endpoint for agent planning";
	endpoint.request.prompt = "Use an already-running OpenAI-compatible server; do not own server lifecycle here.";
	endpoint.request.context = "ofxGgmlAgents can carry OFXGGML_AGENT_LLM_BASE_URL and OFXGGML_AGENT_LLM_MODEL into smoke validation, while ofxGgmlLlama remains responsible for llama.cpp setup.";
	endpoint.request.tools = {
		"OFXGGML_AGENT_LLM_BASE_URL",
		"OFXGGML_AGENT_LLM_MODEL",
		"ofxGgmlLlama/ofxGgmlLlamaCodexLocalExample"
	};
	endpoint.outOfScope = {
		"llama-server startup",
		"Model discovery and downloads",
		"Client-specific Codex or OpenCode config snippets"
	};
	endpoint.validation = {
		"scripts\\run-agents-runtime-smoke.bat -Json -SummaryOnly",
		"scripts\\validate-local.bat"
	};
	scenarios.push_back(endpoint);
}

void ofApp::selectScenario(std::size_t index) {
	if (index >= scenarios.size()) {
		return;
	}
	selectedScenario = index;
	request = scenarios[selectedScenario].request;
	status = ofxGgmlAgentsUtils::describe(request);
	refreshHandoffText();
	ofLogNotice(kLogModule) << status;
}

void ofApp::refreshHandoffText() {
	const PlanningScenario & scenario = scenarios[selectedScenario];
	std::ostringstream stream;

	stream << "Workflow: " << scenario.name << "\n";
	stream << "User goal: " << request.goal << "\n";
	stream << "Repository touched: ofxGgmlAgents\n";
	if (scenario.name == "Local LLM endpoint handoff") {
		const bool endpointConfigured = !endpointBaseUrl.empty() && !endpointModel.empty();
		stream << "Assistant client: OpenAI-compatible local client\n";
		stream << "Endpoint configured: " << configuredText(endpointConfigured) << "\n";
		stream << "Endpoint base URL: " << displayValue(endpointBaseUrl) << "\n";
		stream << "Model alias: " << displayValue(endpointModel) << "\n";
		stream << "Provider owner: ofxGgmlLlama\n";
		stream << "Server health checked: no, this example does not call the provider\n";
		stream << "API key: " << (endpointApiKeyConfigured ? "configured, value hidden" : "not set") << "\n";
	}
	stream << "Companion tools needed: ";
	appendList(stream, request.tools);
	stream << "Out of scope: ";
	appendList(stream, scenario.outOfScope);
	stream << "Validation: ";
	appendList(stream, scenario.validation);

	handoffText = stream.str();
}

void ofApp::refreshEndpointStatus() {
	endpointBaseUrl = readEnvironmentValue("OFXGGML_AGENT_LLM_BASE_URL");
	endpointModel = readEnvironmentValue("OFXGGML_AGENT_LLM_MODEL");
	endpointApiKeyConfigured = !readEnvironmentValue("OFXGGML_AGENT_LLM_API_KEY").empty();
	if (!scenarios.empty()) {
		refreshHandoffText();
	}
}

void ofApp::drawScenarioList() {
	ImGui::BeginChild("planning-scenarios", ImVec2(240.0f, 0.0f), true);
	ImGui::TextUnformatted("Scenarios");
	ImGui::Separator();
	for (std::size_t i = 0; i < scenarios.size(); ++i) {
		const bool selected = i == selectedScenario;
		if (ImGui::Selectable(scenarios[i].name.c_str(), selected)) {
			selectScenario(i);
		}
	}
	ImGui::EndChild();
}

void ofApp::drawRequestTab() const {
	ImGui::TextUnformatted("Goal");
	ImGui::TextWrapped("%s", request.goal.c_str());
	ImGui::Spacing();
	ImGui::TextUnformatted("Prompt");
	ImGui::TextWrapped("%s", request.prompt.c_str());
	ImGui::Spacing();
	ImGui::TextUnformatted("Context");
	ImGui::TextWrapped("%s", request.context.c_str());
	ImGui::Spacing();
	ImGui::TextUnformatted("Companion tools");
	drawBullets(request.tools);
}

void ofApp::drawHandoffTab() const {
	const PlanningScenario & scenario = scenarios[selectedScenario];
	if (ImGui::Button("Log handoff")) {
		logHandoff();
	}
	ImGui::SameLine();
	if (ImGui::Button("Copy handoff")) {
		ImGui::SetClipboardText(handoffText.c_str());
	}
	ImGui::Spacing();
	ImGui::TextUnformatted("Workflow");
	ImGui::TextWrapped("%s", scenario.name.c_str());
	ImGui::Spacing();
	ImGui::TextUnformatted("User goal");
	ImGui::TextWrapped("%s", request.goal.c_str());
	ImGui::Spacing();
	ImGui::TextUnformatted("Repository touched");
	ImGui::TextWrapped("ofxGgmlAgents");
	ImGui::Spacing();
	ImGui::TextUnformatted("Companion tools needed");
	drawBullets(request.tools);
	ImGui::Spacing();
	ImGui::TextUnformatted("Validation");
	drawBullets(scenario.validation);
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::TextUnformatted("Template record");
	ImGui::BeginChild("handoff-record", ImVec2(0.0f, 132.0f), true);
	ImGui::TextUnformatted(handoffText.c_str());
	ImGui::EndChild();
}

void ofApp::drawBoundaryTab() const {
	const PlanningScenario & scenario = scenarios[selectedScenario];
	ImGui::TextUnformatted("Out of scope");
	drawBullets(scenario.outOfScope);
	ImGui::Spacing();
	ImGui::TextUnformatted("Validation commands");
	drawBullets(scenario.validation);
}

void ofApp::drawEndpointTab() {
	const bool endpointConfigured = !endpointBaseUrl.empty() && !endpointModel.empty();
	if (ImGui::Button("Refresh environment")) {
		refreshEndpointStatus();
	}
	ImGui::Spacing();
	ImGui::TextUnformatted("Endpoint configured");
	ImGui::TextWrapped("%s", endpointConfigured ? "yes" : "no");
	ImGui::Spacing();
	ImGui::TextUnformatted("OFXGGML_AGENT_LLM_BASE_URL");
	ImGui::TextWrapped("%s", displayValue(endpointBaseUrl));
	ImGui::Spacing();
	ImGui::TextUnformatted("OFXGGML_AGENT_LLM_MODEL");
	ImGui::TextWrapped("%s", displayValue(endpointModel));
	ImGui::Spacing();
	ImGui::TextUnformatted("OFXGGML_AGENT_LLM_API_KEY");
	ImGui::TextWrapped("%s", endpointApiKeyConfigured ? "(configured, value hidden)" : "(not set)");
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::TextWrapped("This example only records endpoint handoff state. It does not start llama.cpp, download models, call a server, or expose API key values.");
}

void ofApp::logHandoff() const {
	ofLogNotice(kLogModule) << "\n" << handoffText;
}

void ofApp::drawBullets(const std::vector<std::string> & items) {
	for (const auto & item : items) {
		ImGui::BulletText("%s", item.c_str());
	}
}
