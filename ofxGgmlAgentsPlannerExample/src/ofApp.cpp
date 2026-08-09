#include "ofApp.h"

#include <cstdlib>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <sstream>
#include <stdexcept>

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

	std::string resolveChatEndpoint(const std::string & baseUrl) {
		std::string endpoint = ofTrim(baseUrl);
		while (!endpoint.empty() && endpoint.back() == '/') endpoint.pop_back();
		if (ofToLower(endpoint).size() >= 3 && ofToLower(endpoint).substr(endpoint.size() - 3) == "/v1") {
			return endpoint + "/chat/completions";
		}
		return endpoint + "/v1/chat/completions";
	}

	std::vector<std::string> readProvenLanes(const std::string & path) {
		ofBuffer buffer = ofBufferFromFile(path);
		if (buffer.size() == 0) throw std::runtime_error("Canonical ecosystem file was not found or was empty: " + path);
		std::vector<std::string> lanes;
		std::string currentLane;
		for (const auto & rawLine : buffer.getLines()) {
			const std::string line = ofTrim(rawLine);
			if (line.rfind("lane:", 0) == 0) currentLane = ofTrim(line.substr(5));
			else if (line == "status: proven" && !currentLane.empty()) {
				if (std::find(lanes.begin(), lanes.end(), currentLane) == lanes.end()) lanes.push_back(currentLane);
				currentLane.clear();
			}
		}
		return lanes;
	}

	struct NormalizedToolCall {
		std::string id;
		std::string name;
		std::string encoding;
		ofJson arguments;
	};

	NormalizedToolCall normalizeToolCall(const ofJson & message) {
		if (message.contains("tool_calls") && message["tool_calls"].is_array() && !message["tool_calls"].empty()) {
			const auto & call = message["tool_calls"][0];
			NormalizedToolCall result;
			result.id = call.value("id", "");
			result.name = call.at("function").value("name", "");
			result.encoding = "structured-tool-calls";
			const std::string arguments = call.at("function").value("arguments", "{}");
			result.arguments = ofJson::parse(arguments.empty() ? "{}" : arguments);
			return result;
		}
		const std::string content = message.value("content", "");
		const ofJson decoded = ofJson::parse(ofTrim(content));
		NormalizedToolCall result;
		result.name = decoded.value("name", "");
		result.arguments = decoded.value("arguments", ofJson::object());
		result.encoding = "json-in-content";
		return result;
	}

	ofHttpResponse postJson(const std::string & endpoint, const ofJson & body, const std::string & apiKey) {
		ofHttpRequest request;
		request.url = endpoint;
		request.method = ofHttpRequest::POST;
		request.body = body.dump();
		request.contentType = "application/json";
		request.verbose = false;
		if (!apiKey.empty()) request.headers["Authorization"] = "Bearer " + apiKey;
		ofURLFileLoader loader;
		return loader.handleRequest(request);
	}

	ofApp::ToolLoopResult executeToolLoop(const std::string & baseUrl, const std::string & model,
		const std::string & apiKey, const std::string & ecosystemPath) {
		ofApp::ToolLoopResult result;
		const auto started = std::chrono::steady_clock::now();
		try {
			if (ofTrim(baseUrl).empty() || ofTrim(model).empty()) throw std::runtime_error("Endpoint base URL and model are required.");
			const std::string endpoint = resolveChatEndpoint(baseUrl);
			result.events.push_back("Model request: asking " + model + " to call get_proven_lanes");
			ofJson payload = {
				{"model", model}, {"temperature", 0}, {"max_tokens", 128}, {"stream", false},
				{"messages", ofJson::array({
					{{"role", "system"}, {"content", "You must call get_proven_lanes with no arguments. After its result, reply with exactly OFXGGML_AGENTS_TOOL_OK and no extra text."}},
					{{"role", "user"}, {"content", "Use the available tool to identify the proven model lanes."}}
				})},
				{"tools", ofJson::array({{{"type", "function"}, {"function", {{"name", "get_proven_lanes"}, {"description", "Read proven lanes from the canonical ecosystem manifest."}, {"parameters", {{"type", "object"}, {"properties", ofJson::object()}, {"additionalProperties", false}}}}}}})}
			};
			ofHttpResponse first = postJson(endpoint, payload, apiKey);
			if (first.status < 200 || first.status >= 300) throw std::runtime_error("Initial model request failed (HTTP " + ofToString(first.status) + "): " + first.error);
			const ofJson firstJson = ofJson::parse(first.data.getText());
			const ofJson assistantMessage = firstJson.at("choices").at(0).at("message");
			const NormalizedToolCall toolCall = normalizeToolCall(assistantMessage);
			if (toolCall.name != "get_proven_lanes") throw std::runtime_error("Model requested non-allowlisted tool: " + toolCall.name);
			if (!toolCall.arguments.is_object() || !toolCall.arguments.empty()) throw std::runtime_error("get_proven_lanes does not accept arguments.");
			result.toolCallEncoding = toolCall.encoding;
			result.events.push_back("Model requested tool: get_proven_lanes (" + toolCall.encoding + ")");
			result.provenLanes = readProvenLanes(ecosystemPath);
			result.events.push_back("Executed read-only tool: get_proven_lanes");
			result.events.push_back("Returned lanes: " + ofJoinString(result.provenLanes, ", "));
			ofJson toolResult = {{"proven_lanes", result.provenLanes}};
			payload["messages"].push_back(assistantMessage);
			ofJson toolMessage = {{"role", "tool"}, {"name", "get_proven_lanes"}, {"content", toolResult.dump()}};
			if (!toolCall.id.empty()) toolMessage["tool_call_id"] = toolCall.id;
			payload["messages"].push_back(toolMessage);
			ofHttpResponse second = postJson(endpoint, payload, apiKey);
			if (second.status < 200 || second.status >= 300) throw std::runtime_error("Final model request failed (HTTP " + ofToString(second.status) + "): " + second.error);
			const ofJson secondJson = ofJson::parse(second.data.getText());
			result.finalConfirmation = ofTrim(secondJson.at("choices").at(0).at("message").value("content", ""));
			if (result.finalConfirmation != "OFXGGML_AGENTS_TOOL_OK") throw std::runtime_error("Model did not return OFXGGML_AGENTS_TOOL_OK; received: " + result.finalConfirmation);
			result.events.push_back("Final confirmation: " + result.finalConfirmation);
			result.success = true;
		} catch (const std::exception & error) {
			result.error = error.what();
			result.events.push_back("Error: " + result.error);
		}
		result.elapsedMs = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started).count());
		return result;
	}
}

ofApp::ToolLoopResult ofApp::runToolLoopForSmoke(const std::string & baseUrl, const std::string & model,
	const std::string & apiKey, const std::string & ecosystemPath) {
	return executeToolLoop(baseUrl, model, apiKey, ecosystemPath);
}

void ofApp::setup() {
	ofSetWindowTitle("ofxGgmlAgents planner example");
	gui.setup(nullptr, false);
	refreshEndpointStatus();
	buildScenarios();
	selectScenario(0);
	logHandoff();
}

void ofApp::update() {
	if (toolLoopState != ToolLoopState::Running || !toolLoopFuture.valid()) return;
	if (toolLoopFuture.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) return;
	toolLoopResult = toolLoopFuture.get();
	toolLoopState = toolLoopResult.success ? ToolLoopState::Succeeded : ToolLoopState::Failed;
	for (const auto & event : toolLoopResult.events) ofLogNotice(kLogModule) << event;
	ofLogNotice(kLogModule) << "Tool loop elapsed: " << toolLoopResult.elapsedMs << " ms";
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
	endpointApiKey = readEnvironmentValue("OFXGGML_AGENT_LLM_API_KEY");
	endpointApiKeyConfigured = !endpointApiKey.empty();
	ecosystemPath = readEnvironmentValue("OFXGGML_AGENT_ECOSYSTEM_PATH");
	if (ecosystemPath.empty()) ecosystemPath = ofToDataPath("../../../../ofxGgmlWorkflows/ecosystem.yaml", true);
	std::snprintf(endpointBaseUrlInput.data(), endpointBaseUrlInput.size(), "%s", endpointBaseUrl.c_str());
	std::snprintf(endpointModelInput.data(), endpointModelInput.size(), "%s", endpointModel.c_str());
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
	endpointBaseUrl = endpointBaseUrlInput.data();
	endpointModel = endpointModelInput.data();
	const bool endpointConfigured = !ofTrim(endpointBaseUrl).empty() && !ofTrim(endpointModel).empty();
	if (ImGui::Button("Refresh environment")) {
		refreshEndpointStatus();
	}
	ImGui::Spacing();
	ImGui::InputText("Endpoint base URL", endpointBaseUrlInput.data(), endpointBaseUrlInput.size());
	ImGui::InputText("Model alias", endpointModelInput.data(), endpointModelInput.size());
	ImGui::TextWrapped("Manifest: %s", ecosystemPath.c_str());
	if (toolLoopState == ToolLoopState::Running) ImGui::BeginDisabled();
	if (ImGui::Button("Run allowlisted tool loop")) startToolLoop();
	if (toolLoopState == ToolLoopState::Running) ImGui::EndDisabled();
	ImGui::SameLine();
	const char * stateText = toolLoopState == ToolLoopState::Idle ? "idle" : toolLoopState == ToolLoopState::Running ? "running" : toolLoopState == ToolLoopState::Succeeded ? "completed" : "failed";
	ImGui::Text("State: %s", stateText);
	ImGui::Separator();
	ImGui::TextUnformatted("Endpoint configured");
	ImGui::TextWrapped("%s", endpointConfigured ? "yes" : "no");
	ImGui::Spacing();
	ImGui::TextUnformatted("OFXGGML_AGENT_LLM_API_KEY");
	ImGui::TextWrapped("%s", endpointApiKeyConfigured ? "(configured, value hidden)" : "(not set)");
	ImGui::Spacing();
	ImGui::Text("Elapsed: %d ms", toolLoopResult.elapsedMs);
	for (const auto & event : toolLoopResult.events) ImGui::BulletText("%s", event.c_str());
	ImGui::Separator();
	ImGui::TextWrapped("Offline by default: network and tool execution begin only when Run is clicked. This example consumes an already-running endpoint; it never starts or configures the provider.");
}

void ofApp::startToolLoop() {
	if (toolLoopState == ToolLoopState::Running) return;
	endpointBaseUrl = endpointBaseUrlInput.data();
	endpointModel = endpointModelInput.data();
	toolLoopResult = {};
	toolLoopState = ToolLoopState::Running;
	ofLogNotice(kLogModule) << "Starting explicit allowlisted get_proven_lanes loop";
	toolLoopFuture = std::async(std::launch::async, executeToolLoop, endpointBaseUrl, endpointModel, endpointApiKey, ecosystemPath);
}

void ofApp::logHandoff() const {
	ofLogNotice(kLogModule) << "\n" << handoffText;
}

void ofApp::drawBullets(const std::vector<std::string> & items) {
	for (const auto & item : items) {
		ImGui::BulletText("%s", item.c_str());
	}
}
