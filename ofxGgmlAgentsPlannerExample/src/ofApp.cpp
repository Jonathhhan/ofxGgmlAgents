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

	std::size_t modelRequestTimeoutSeconds() {
		const std::string configured = ofTrim(readEnvironmentValue("OFXGGML_AGENT_LLM_TIMEOUT_SECONDS"));
		if (configured.empty()) return 120;
		try {
			const int value = std::stoi(configured);
			return static_cast<std::size_t>(ofClamp(value, 1, 3600));
		} catch (const std::exception &) {
			ofLogWarning(kLogModule) << "Ignoring invalid OFXGGML_AGENT_LLM_TIMEOUT_SECONDS: " << configured;
			return 120;
		}
	}

	std::string resolveChatEndpoint(const std::string & baseUrl) {
		std::string endpoint = ofTrim(baseUrl);
		while (!endpoint.empty() && endpoint.back() == '/') endpoint.pop_back();
		if (ofToLower(endpoint).size() >= 3 && ofToLower(endpoint).substr(endpoint.size() - 3) == "/v1") {
			return endpoint + "/chat/completions";
		}
		return endpoint + "/v1/chat/completions";
	}

	std::string resolveModelsEndpoint(const std::string & baseUrl) {
		std::string endpoint = ofTrim(baseUrl);
		while (!endpoint.empty() && endpoint.back() == '/') endpoint.pop_back();
		if (ofToLower(endpoint).size() >= 3 && ofToLower(endpoint).substr(endpoint.size() - 3) == "/v1") {
			return endpoint + "/models";
		}
		return endpoint + "/v1/models";
	}

	struct CapabilityStatus {
		std::string lane;
		std::string capability;
		std::string status;
		std::string proof;
	};

	std::vector<CapabilityStatus> readCapabilityStatuses(const std::string & path) {
		ofBuffer buffer = ofBufferFromFile(path);
		if (buffer.size() == 0) throw std::runtime_error("Canonical ecosystem file was not found or was empty: " + path);
		std::vector<CapabilityStatus> capabilities;
		CapabilityStatus current;
		bool inDevelopmentOrder = false;
		const auto flush = [&capabilities, &current]() {
			if (!current.lane.empty() && !current.status.empty()) capabilities.push_back(current);
			current = {};
		};
		for (const auto & rawLine : buffer.getLines()) {
			const std::string line = ofTrim(rawLine);
			if (line == "development_order:") { inDevelopmentOrder = true; continue; }
			if (!inDevelopmentOrder) continue;
			if (!rawLine.empty() && rawLine.front() != ' ' && line != "development_order:") break;
			if (line.rfind("- order:", 0) == 0) flush();
			else if (line.rfind("lane:", 0) == 0) current.lane = ofTrim(line.substr(5));
			else if (line.rfind("capability:", 0) == 0) current.capability = ofTrim(line.substr(11));
			else if (line.rfind("status:", 0) == 0) current.status = ofTrim(line.substr(7));
			else if (line.rfind("proof:", 0) == 0) current.proof = ofTrim(line.substr(6));
		}
		flush();
		return capabilities;
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
		try {
			const ofJson decoded = ofJson::parse(ofTrim(content));
			NormalizedToolCall result;
			result.name = decoded.value("name", "");
			result.arguments = decoded.value("arguments", ofJson::object());
			result.encoding = "json-in-content";
			return result;
		} catch (const ofJson::parse_error &) {}

		const auto extractTag = [&content](const std::string & tag) {
			const std::string opening = "<" + tag + ">";
			const std::string closing = "</" + tag + ">";
			const std::size_t start = content.find(opening);
			if (start == std::string::npos) return std::string();
			const std::size_t valueStart = start + opening.size();
			const std::size_t end = content.find(closing, valueStart);
			if (end == std::string::npos) return std::string();
			return ofTrim(content.substr(valueStart, end - valueStart));
		};
		NormalizedToolCall result;
		result.name = extractTag("name");
		const std::string arguments = extractTag("arguments");
		if (result.name.empty() || arguments.empty()) throw std::runtime_error("Model did not request an allowlisted tool.");
		result.arguments = ofJson::parse(arguments);
		result.encoding = "xml-in-content";
		return result;
	}

	ofHttpResponse postJson(const std::string & endpoint, const ofJson & body, const std::string & apiKey) {
		ofHttpRequest request;
		request.url = endpoint;
		request.method = ofHttpRequest::POST;
		request.body = body.dump();
		request.contentType = "application/json";
		request.verbose = false;
		request.timeoutSeconds = modelRequestTimeoutSeconds();
		if (!apiKey.empty()) request.headers["Authorization"] = "Bearer " + apiKey;
		ofURLFileLoader loader;
		return loader.handleRequest(request);
	}

	ofHttpResponse getJson(const std::string & endpoint, const std::string & apiKey) {
		ofHttpRequest request;
		request.url = endpoint;
		request.method = ofHttpRequest::GET;
		request.contentType = "application/json";
		request.verbose = false;
		request.timeoutSeconds = 5;
		if (!apiKey.empty()) request.headers["Authorization"] = "Bearer " + apiKey;
		ofURLFileLoader loader;
		return loader.handleRequest(request);
	}

	ofApp::ToolLoopResult executeToolLoop(const std::string & baseUrl, const std::string & model,
		const std::string & apiKey, const std::string & toolName, const std::string & ecosystemPath,
		const std::string & ragSourceRoot, const std::string & requestedRagQuery,
		const std::string & selectedBackend) {
		ofApp::ToolLoopResult result;
		result.selectedBackend = ofToLower(ofTrim(selectedBackend));
		if (result.selectedBackend.empty()) result.selectedBackend = "configured";
		result.endpointBaseUrl = ofTrim(baseUrl);
		result.toolName = toolName;
		result.resourcePath = toolName == "search_local_corpus" ? ragSourceRoot : ecosystemPath;
		const auto started = std::chrono::steady_clock::now();
		try {
			if (ofTrim(baseUrl).empty() || ofTrim(model).empty()) throw std::runtime_error("Endpoint base URL and model are required.");
			const bool useRag = toolName == "search_local_corpus";
			if (!useRag && toolName != "get_capability_status") throw std::runtime_error("Unsupported allowlisted tool: " + toolName);
			if (useRag && ofTrim(ragSourceRoot).empty()) throw std::runtime_error("Select a local RAG corpus folder before running the tool loop.");
			const std::string endpoint = resolveChatEndpoint(baseUrl);
			result.events.push_back("Selected backend: " + result.selectedBackend);
			result.events.push_back("Endpoint: " + result.endpointBaseUrl);
			result.events.push_back("Model request: asking " + model + " to call " + toolName);
			const std::string systemPrompt = useRag
				? "You must call search_local_corpus with one query argument. Search for the user's topic. After its result, reply with exactly OFXGGML_AGENTS_TOOL_OK and no extra text."
				: "You must call get_capability_status with no arguments. After its result, reply with exactly OFXGGML_AGENTS_TOOL_OK and no extra text.";
			const std::string userPrompt = useRag
				? "Use search_local_corpus to find cited local evidence about: " + requestedRagQuery
				: "Use the available tool to inspect the current capability status of all model lanes.";
			ofJson toolDefinition;
			if (useRag) {
				ofJson querySchema = {{"type", "string"}, {"description", "The focused search query."}};
				ofJson parameters = {{"type", "object"}, {"properties", {{"query", querySchema}}},
					{"required", ofJson::array({"query"})}, {"additionalProperties", false}};
				toolDefinition = {{"type", "function"}, {"function", {{"name", "search_local_corpus"},
					{"description", "Search the user-selected local text corpus and return cited excerpts."},
					{"parameters", parameters}}}};
			} else {
				ofJson parameters = {{"type", "object"}, {"properties", ofJson::object()}, {"additionalProperties", false}};
				toolDefinition = {{"type", "function"}, {"function", {{"name", "get_capability_status"},
					{"description", "Read each lane's capability, current status, and proof identifier from the canonical ecosystem manifest."}, {"parameters", parameters}}}};
			}
			ofJson payload = {
				{"model", model}, {"temperature", 0}, {"max_tokens", 128}, {"stream", false},
				{"messages", ofJson::array({
					{{"role", "system"}, {"content", systemPrompt}},
					{{"role", "user"}, {"content", userPrompt}}
				})},
				{"tools", ofJson::array({toolDefinition})},
				{"tool_choice", "required"}
			};
			ofHttpResponse first = postJson(endpoint, payload, apiKey);
			if (first.status < 200 || first.status >= 300) throw std::runtime_error("Initial model request failed (HTTP " + ofToString(first.status) + "): " + first.error);
			const ofJson firstJson = ofJson::parse(first.data.getText());
			const ofJson assistantMessage = firstJson.at("choices").at(0).at("message");
			const NormalizedToolCall toolCall = normalizeToolCall(assistantMessage);
			if (toolCall.name != toolName) throw std::runtime_error("Model requested non-allowlisted tool: " + toolCall.name);
			result.toolCallEncoding = toolCall.encoding;
			result.events.push_back("Model requested tool: " + toolName + " (" + toolCall.encoding + ")");
			ofJson toolResult;
			if (useRag) {
				if (!toolCall.arguments.is_object() || toolCall.arguments.size() != 1 || !toolCall.arguments.contains("query") || !toolCall.arguments["query"].is_string()) throw std::runtime_error("search_local_corpus requires exactly one string query.");
				result.ragQuery = ofTrim(toolCall.arguments["query"].get<std::string>());
				if (result.ragQuery.empty() || result.ragQuery.size() > 512) throw std::runtime_error("search_local_corpus query must contain 1 to 512 characters.");
				ofxGgmlRagRequest request;
				request.query = result.ragQuery;
				request.sourceRoot = ragSourceRoot;
				ofxGgmlRagRetrievalOptions options;
				options.search.topK = 3;
				options.context.maxChars = 3000;
				const auto retrieval = ofxGgmlRagUtils::retrieveTextCorpus(request, ofxGgmlRagCorpusOptions(), options);
				if (!retrieval || retrieval.hits.empty()) throw std::runtime_error(retrieval.result.error.empty() ? "Local RAG search returned no cited hits." : retrieval.result.error);
				result.ragContext = retrieval.context.text;
				result.ragReferences = retrieval.result.references;
				toolResult = {{"query", result.ragQuery}, {"context", result.ragContext}, {"references", result.ragReferences}, {"hit_count", retrieval.hits.size()}};
				result.events.push_back("Executed read-only tool: search_local_corpus");
				result.events.push_back("Returned cited hits: " + ofToString(retrieval.hits.size()));
			} else {
				if (!toolCall.arguments.is_object() || !toolCall.arguments.empty()) throw std::runtime_error("get_capability_status does not accept arguments.");
				const auto capabilities = readCapabilityStatuses(ecosystemPath);
				if (capabilities.empty()) throw std::runtime_error("Canonical ecosystem manifest contains no development-order capability statuses.");
				toolResult["capabilities"] = ofJson::array();
				for (const auto & capability : capabilities) {
					toolResult["capabilities"].push_back({{"lane", capability.lane}, {"capability", capability.capability},
						{"status", capability.status}, {"proof", capability.proof}});
					result.capabilityStatuses.push_back(capability.lane + ": " + capability.status + " (" + capability.capability + ")");
				}
				result.events.push_back("Executed read-only tool: get_capability_status");
				result.events.push_back("Returned capability statuses: " + ofToString(capabilities.size()));
			}
			payload["messages"].push_back(assistantMessage);
			ofJson toolMessage = {{"role", "tool"}, {"name", toolName}, {"content", toolResult.dump()}};
			if (!toolCall.id.empty()) toolMessage["tool_call_id"] = toolCall.id;
			payload["messages"].push_back(toolMessage);
			payload.erase("tool_choice");
			payload.erase("tools");
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

ofApp::EndpointCheckResult ofApp::checkEndpointForSmoke(const std::string & baseUrl, const std::string & model,
	const std::string & apiKey, const std::string & selectedBackend) {
	EndpointCheckResult result;
	result.selectedBackend = ofToLower(ofTrim(selectedBackend));
	if (result.selectedBackend.empty()) result.selectedBackend = "configured";
	result.endpointBaseUrl = ofTrim(baseUrl);
	result.requestedModel = ofTrim(model);
	const auto started = std::chrono::steady_clock::now();
	try {
		if (result.endpointBaseUrl.empty() || result.requestedModel.empty()) {
			throw std::runtime_error("Endpoint base URL and model are required.");
		}
		const ofHttpResponse response = getJson(resolveModelsEndpoint(result.endpointBaseUrl), apiKey);
		result.httpStatus = response.status;
		if (response.status < 200 || response.status >= 300) {
			throw std::runtime_error("Model discovery failed (HTTP " + ofToString(response.status) + "): " + response.error);
		}
		const ofJson body = ofJson::parse(response.data.getText());
		if (!body.contains("data") || !body["data"].is_array()) {
			throw std::runtime_error("Model discovery response did not contain a data array.");
		}
		for (const auto & entry : body["data"]) {
			if (!entry.is_object()) continue;
			const std::string id = entry.value("id", "");
			if (!id.empty()) result.advertisedModels.push_back(id);
		}
		result.modelAvailable = std::find(result.advertisedModels.begin(), result.advertisedModels.end(), result.requestedModel) != result.advertisedModels.end();
		if (!result.modelAvailable) {
			throw std::runtime_error("Endpoint is reachable, but the selected model alias is not advertised.");
		}
		result.success = true;
	} catch (const std::exception & error) {
		result.error = error.what();
	}
	result.elapsedMs = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started).count());
	return result;
}

ofApp::ToolLoopResult ofApp::runToolLoopForSmoke(const std::string & baseUrl, const std::string & model,
	const std::string & apiKey, const std::string & ecosystemPath, const std::string & selectedBackend) {
	return executeToolLoop(baseUrl, model, apiKey, "get_capability_status", ecosystemPath, "", "", selectedBackend);
}

ofApp::ToolLoopResult ofApp::runLocalRagToolLoopForSmoke(const std::string & baseUrl, const std::string & model,
	const std::string & apiKey, const std::string & sourceRoot, const std::string & query,
	const std::string & selectedBackend) {
	return executeToolLoop(baseUrl, model, apiKey, "search_local_corpus", "", sourceRoot, query, selectedBackend);
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
	if (endpointCheckState == EndpointCheckState::Running && endpointCheckFuture.valid()
		&& endpointCheckFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
		endpointCheckResult = endpointCheckFuture.get();
		endpointCheckState = endpointCheckResult.success ? EndpointCheckState::Reachable : EndpointCheckState::Failed;
		if (endpointCheckResult.success) {
			ofLogNotice(kLogModule) << "Endpoint ready for " << endpointCheckResult.selectedBackend << " at "
			                        << endpointCheckResult.endpointBaseUrl << " in " << endpointCheckResult.elapsedMs << " ms";
		} else {
			ofLogWarning(kLogModule) << "Endpoint check failed for " << endpointCheckResult.selectedBackend << ": " << endpointCheckResult.error;
		}
		refreshHandoffText();
	}
	if (toolLoopState == ToolLoopState::Running && toolLoopFuture.valid()
		&& toolLoopFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
		toolLoopResult = toolLoopFuture.get();
		toolLoopState = toolLoopResult.success ? ToolLoopState::Succeeded : ToolLoopState::Failed;
		for (const auto & event : toolLoopResult.events) ofLogNotice(kLogModule) << event;
		ofLogNotice(kLogModule) << "Tool loop elapsed: " << toolLoopResult.elapsedMs << " ms";
	}
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
		stream << "Selected backend profile: " << selectedBackendDisplay() << "\n";
		stream << "Endpoint configured: " << configuredText(endpointConfigured) << "\n";
		stream << "Endpoint base URL: " << displayValue(endpointBaseUrl) << "\n";
		stream << "Model alias: " << displayValue(endpointModel) << "\n";
		stream << "Provider owner: ofxGgmlLlama\n";
		const char * checkText = endpointCheckState == EndpointCheckState::Idle ? "not checked"
			: endpointCheckState == EndpointCheckState::Running ? "checking"
			: endpointCheckState == EndpointCheckState::Reachable ? "ready"
			: "failed";
		stream << "Selected endpoint readiness: " << checkText << "\n";
		if (endpointCheckState == EndpointCheckState::Reachable || endpointCheckState == EndpointCheckState::Failed) {
			stream << "Endpoint check HTTP status: " << endpointCheckResult.httpStatus << "\n";
			stream << "Endpoint check elapsed: " << endpointCheckResult.elapsedMs << " ms\n";
			if (!endpointCheckResult.error.empty()) stream << "Endpoint check error: " << endpointCheckResult.error << "\n";
		}
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
	const std::string fallbackUrl = readEnvironmentValue("OFXGGML_AGENT_LLM_BASE_URL");
	const std::string fallbackModel = readEnvironmentValue("OFXGGML_AGENT_LLM_MODEL");
	std::string cpuUrl = readEnvironmentValue("OFXGGML_AGENT_CPU_LLM_BASE_URL");
	std::string cpuModel = readEnvironmentValue("OFXGGML_AGENT_CPU_LLM_MODEL");
	std::string cudaUrl = readEnvironmentValue("OFXGGML_AGENT_CUDA_LLM_BASE_URL");
	std::string cudaModel = readEnvironmentValue("OFXGGML_AGENT_CUDA_LLM_MODEL");
	cpuEndpointUsesSharedFallback = cpuUrl.empty() && !fallbackUrl.empty();
	cudaEndpointUsesSharedFallback = cudaUrl.empty() && !fallbackUrl.empty();
	if (cpuUrl.empty()) cpuUrl = fallbackUrl.empty() ? "http://127.0.0.1:8082" : fallbackUrl;
	if (cpuModel.empty()) cpuModel = fallbackModel;
	if (cudaUrl.empty()) cudaUrl = fallbackUrl.empty() ? "http://127.0.0.1:8080" : fallbackUrl;
	if (cudaModel.empty()) cudaModel = fallbackModel;
	endpointApiKey = readEnvironmentValue("OFXGGML_AGENT_LLM_API_KEY");
	endpointApiKeyConfigured = !endpointApiKey.empty();
	ecosystemPath = readEnvironmentValue("OFXGGML_AGENT_ECOSYSTEM_PATH");
	if (ecosystemPath.empty()) ecosystemPath = ofToDataPath("../../../../ofxGgmlWorkflows/ecosystem.yaml", true);
	ragSourceRoot = readEnvironmentValue("OFXGGML_AGENT_RAG_SOURCE_ROOT");
	std::snprintf(cpuEndpointBaseUrlInput.data(), cpuEndpointBaseUrlInput.size(), "%s", cpuUrl.c_str());
	std::snprintf(cpuEndpointModelInput.data(), cpuEndpointModelInput.size(), "%s", cpuModel.c_str());
	std::snprintf(cudaEndpointBaseUrlInput.data(), cudaEndpointBaseUrlInput.size(), "%s", cudaUrl.c_str());
	std::snprintf(cudaEndpointModelInput.data(), cudaEndpointModelInput.size(), "%s", cudaModel.c_str());
	endpointBaseUrl = backendMode == BackendMode::Cpu ? cpuEndpointBaseUrlInput.data() : cudaEndpointBaseUrlInput.data();
	endpointModel = backendMode == BackendMode::Cpu ? cpuEndpointModelInput.data() : cudaEndpointModelInput.data();
	std::snprintf(ragQueryInput.data(), ragQueryInput.size(), "%s", ragQuery.c_str());
	if (!scenarios.empty()) {
		refreshHandoffText();
	}
}

bool ofApp::selectedEndpointUsesSharedFallback() const {
	return backendMode == BackendMode::Cpu ? cpuEndpointUsesSharedFallback : cudaEndpointUsesSharedFallback;
}

std::string ofApp::selectedBackendKey() const {
	if (selectedEndpointUsesSharedFallback()) return "shared";
	return backendMode == BackendMode::Cpu ? "cpu" : "cuda";
}

std::string ofApp::selectedBackendDisplay() const {
	if (selectedEndpointUsesSharedFallback()) {
		return backendMode == BackendMode::Cpu
			? "CPU profile (shared legacy endpoint; offload unknown)"
			: "CUDA profile (shared legacy endpoint; offload unknown)";
	}
	return backendMode == BackendMode::Cpu ? "CPU (0 GPU layers)" : "CUDA (GPU offload)";
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
	const bool requestRunning = endpointCheckState == EndpointCheckState::Running || toolLoopState == ToolLoopState::Running;
	int selectedBackend = backendMode == BackendMode::Cpu ? 0 : 1;
	const std::string cpuChoice = cpuEndpointUsesSharedFallback
		? "CPU profile (shared endpoint; offload unknown)" : "CPU (0 GPU layers)";
	const std::string cudaChoice = cudaEndpointUsesSharedFallback
		? "CUDA profile (shared endpoint; offload unknown)" : "CUDA (GPU offload)";
	const char * backendChoices[] = {cpuChoice.c_str(), cudaChoice.c_str()};
	bool backendChanged = false;
	if (requestRunning) ImGui::BeginDisabled();
	if (ImGui::Combo("Backend profile", &selectedBackend, backendChoices, 2)) {
		backendMode = selectedBackend == 0 ? BackendMode::Cpu : BackendMode::Cuda;
		backendChanged = true;
		invalidateToolLoopResult();
		endpointCheckResult = {};
		endpointCheckState = EndpointCheckState::Idle;
	}
	auto & activeUrlInput = backendMode == BackendMode::Cpu ? cpuEndpointBaseUrlInput : cudaEndpointBaseUrlInput;
	auto & activeModelInput = backendMode == BackendMode::Cpu ? cpuEndpointModelInput : cudaEndpointModelInput;
	endpointBaseUrl = activeUrlInput.data();
	endpointModel = activeModelInput.data();
	const bool endpointConfigured = !ofTrim(endpointBaseUrl).empty() && !ofTrim(endpointModel).empty();
	if (requestRunning) ImGui::BeginDisabled();
	if (ImGui::Button("Refresh environment")) {
		endpointCheckResult = {};
		endpointCheckState = EndpointCheckState::Idle;
		invalidateToolLoopResult();
		refreshEndpointStatus();
	}
	if (requestRunning) ImGui::EndDisabled();
	ImGui::Spacing();
	const std::string backendDisplay = selectedBackendDisplay();
	ImGui::Text("Selected backend: %s", backendDisplay.c_str());
	ImGui::TextWrapped("Backend evidence: configured profile; llama-server does not report GPU layer offload through /health, /v1/models, or /props.");
	const bool urlChanged = ImGui::InputText("Endpoint base URL", activeUrlInput.data(), activeUrlInput.size());
	const bool modelChanged = ImGui::InputText("Model alias", activeModelInput.data(), activeModelInput.size());
	if (urlChanged || modelChanged) {
		endpointCheckResult = {};
		endpointCheckState = EndpointCheckState::Idle;
		invalidateToolLoopResult();
	}
	if (urlChanged) {
		if (backendMode == BackendMode::Cpu) cpuEndpointUsesSharedFallback = false;
		else cudaEndpointUsesSharedFallback = false;
	}
	endpointBaseUrl = activeUrlInput.data();
	endpointModel = activeModelInput.data();
	if (backendChanged || urlChanged || modelChanged) refreshHandoffText();
	if (requestRunning) ImGui::EndDisabled();
	int selectedTool = toolMode == ToolMode::EcosystemLanes ? 0 : 1;
	const char * toolChoices[] = {"Ecosystem lanes", "Local RAG corpus"};
	if (requestRunning) ImGui::BeginDisabled();
	if (ImGui::Combo("Allowlisted tool", &selectedTool, toolChoices, 2)) {
		toolMode = selectedTool == 0 ? ToolMode::EcosystemLanes : ToolMode::LocalRagCorpus;
		invalidateToolLoopResult();
	}
	if (toolMode == ToolMode::EcosystemLanes) {
		ImGui::TextWrapped("Manifest: %s", ecosystemPath.c_str());
		if (ImGui::Button("Choose ecosystem manifest")) {
			auto selection = ofSystemLoadDialog("Select ecosystem.yaml", false, ecosystemPath);
			if (selection.bSuccess) {
				ecosystemPath = selection.getPath();
				invalidateToolLoopResult();
				status = "Selected ecosystem manifest: " + ecosystemPath;
			}
		}
	} else {
		if (ImGui::InputText("RAG query", ragQueryInput.data(), ragQueryInput.size())) {
			invalidateToolLoopResult();
		}
		ragQuery = ragQueryInput.data();
		ImGui::TextWrapped("Corpus: %s", displayValue(ragSourceRoot));
		if (ImGui::Button("Choose corpus folder")) {
			auto selection = ofSystemLoadDialog("Select local RAG corpus", true, ragSourceRoot);
			if (selection.bSuccess) {
				ragSourceRoot = selection.getPath();
				invalidateToolLoopResult();
			}
		}
	}
	if (requestRunning) ImGui::EndDisabled();
	if (requestRunning) ImGui::BeginDisabled();
	if (ImGui::Button("Check selected endpoint")) startEndpointCheck();
	ImGui::SameLine();
	if (ImGui::Button("Run allowlisted tool loop")) startToolLoop();
	if (requestRunning) ImGui::EndDisabled();
	ImGui::SameLine();
	const char * stateText = toolLoopState == ToolLoopState::Idle ? "idle" : toolLoopState == ToolLoopState::Running ? "running" : toolLoopState == ToolLoopState::Succeeded ? "completed" : "failed";
	ImGui::Text("State: %s", stateText);
	const char * endpointStateText = endpointCheckState == EndpointCheckState::Idle ? "not checked"
		: endpointCheckState == EndpointCheckState::Running ? "checking"
		: endpointCheckState == EndpointCheckState::Reachable ? "ready"
		: "failed";
	ImGui::Text("Selected endpoint: %s", endpointStateText);
	if (endpointCheckState == EndpointCheckState::Reachable || endpointCheckState == EndpointCheckState::Failed) {
		ImGui::Text("Endpoint check: HTTP %d, %d ms", endpointCheckResult.httpStatus, endpointCheckResult.elapsedMs);
		ImGui::TextWrapped("Requested model: %s", displayValue(endpointCheckResult.requestedModel));
		if (!endpointCheckResult.advertisedModels.empty()) {
			ImGui::TextUnformatted("Advertised models");
			drawBullets(endpointCheckResult.advertisedModels);
		}
		if (!endpointCheckResult.error.empty()) ImGui::TextWrapped("Endpoint error: %s", endpointCheckResult.error.c_str());
		if (!endpointCheckResult.modelAvailable && !endpointCheckResult.advertisedModels.empty()) {
			if (requestRunning) ImGui::BeginDisabled();
			if (ImGui::Button("Use first advertised model")) {
				std::snprintf(activeModelInput.data(), activeModelInput.size(), "%s", endpointCheckResult.advertisedModels.front().c_str());
				endpointModel = activeModelInput.data();
				endpointCheckResult = {};
				endpointCheckState = EndpointCheckState::Idle;
				invalidateToolLoopResult();
				refreshHandoffText();
			}
			if (requestRunning) ImGui::EndDisabled();
		}
	}
	ImGui::Separator();
	ImGui::TextUnformatted("Endpoint configured");
	ImGui::TextWrapped("%s", endpointConfigured ? "yes" : "no");
	ImGui::Spacing();
	ImGui::TextUnformatted("OFXGGML_AGENT_LLM_API_KEY");
	ImGui::TextWrapped("%s", endpointApiKeyConfigured ? "(configured, value hidden)" : "(not set)");
	if (toolLoopState != ToolLoopState::Idle) {
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::TextUnformatted("Tool result");
		if (!toolLoopResult.toolName.empty()) ImGui::Text("Tool: %s", toolLoopResult.toolName.c_str());
		if (!toolLoopResult.toolCallEncoding.empty()) ImGui::Text("Call encoding: %s", toolLoopResult.toolCallEncoding.c_str());
		if (!toolLoopResult.selectedBackend.empty()) ImGui::Text("Last executed backend: %s", toolLoopResult.selectedBackend.c_str());
		if (!toolLoopResult.endpointBaseUrl.empty()) ImGui::TextWrapped("Endpoint: %s", toolLoopResult.endpointBaseUrl.c_str());
		if (!toolLoopResult.resourcePath.empty()) ImGui::TextWrapped("Resource: %s", toolLoopResult.resourcePath.c_str());
		ImGui::Text("Elapsed: %d ms", toolLoopResult.elapsedMs);
		if (!toolLoopResult.error.empty()) {
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.45f, 0.38f, 1.0f));
			ImGui::TextWrapped("Error: %s", toolLoopResult.error.c_str());
			ImGui::PopStyleColor();
		}
		if (!toolLoopResult.capabilityStatuses.empty()) {
			ImGui::TextUnformatted("Ecosystem capability status");
			drawBullets(toolLoopResult.capabilityStatuses);
		}
		if (!toolLoopResult.finalConfirmation.empty()) {
			ImGui::TextWrapped("Model confirmation: %s", toolLoopResult.finalConfirmation.c_str());
		}
		if (!toolLoopResult.events.empty() && ImGui::TreeNode("Execution trace")) {
			for (const auto & event : toolLoopResult.events) ImGui::BulletText("%s", event.c_str());
			ImGui::TreePop();
		}
	}
	if (!toolLoopResult.ragReferences.empty()) {
		ImGui::TextWrapped("Executed RAG query: %s", displayValue(toolLoopResult.ragQuery));
		if (!toolLoopResult.ragContext.empty()) {
			ImGui::TextUnformatted("Retrieved cited excerpts");
			ImGui::BeginChild("rag-context", ImVec2(0.0f, 140.0f), true);
			ImGui::TextWrapped("%s", toolLoopResult.ragContext.c_str());
			ImGui::EndChild();
		}
		ImGui::TextUnformatted("Cited sources");
		drawBullets(toolLoopResult.ragReferences);
	}
	ImGui::Separator();
	ImGui::TextWrapped("Offline by default: network access begins only when Check or Run is clicked. This example consumes an already-running endpoint; it never starts or configures the provider.");
}

void ofApp::startEndpointCheck() {
	if (endpointCheckState == EndpointCheckState::Running || toolLoopState == ToolLoopState::Running) return;
	endpointBaseUrl = backendMode == BackendMode::Cpu ? cpuEndpointBaseUrlInput.data() : cudaEndpointBaseUrlInput.data();
	endpointModel = backendMode == BackendMode::Cpu ? cpuEndpointModelInput.data() : cudaEndpointModelInput.data();
	endpointCheckResult = {};
	endpointCheckState = EndpointCheckState::Running;
	const std::string selectedBackend = selectedBackendKey();
	ofLogNotice(kLogModule) << "Checking selected " << selectedBackend << " endpoint at " << endpointBaseUrl;
	endpointCheckFuture = std::async(std::launch::async, checkEndpointForSmoke,
		endpointBaseUrl, endpointModel, endpointApiKey, selectedBackend);
	refreshHandoffText();
}

void ofApp::startToolLoop() {
	if (toolLoopState == ToolLoopState::Running || endpointCheckState == EndpointCheckState::Running) return;
	endpointBaseUrl = backendMode == BackendMode::Cpu ? cpuEndpointBaseUrlInput.data() : cudaEndpointBaseUrlInput.data();
	endpointModel = backendMode == BackendMode::Cpu ? cpuEndpointModelInput.data() : cudaEndpointModelInput.data();
	toolLoopResult = {};
	toolLoopState = ToolLoopState::Running;
	const std::string selectedTool = toolMode == ToolMode::EcosystemLanes ? "get_capability_status" : "search_local_corpus";
	const std::string selectedBackend = selectedBackendKey();
	const std::string ragSourceRootSnapshot = ragSourceRoot;
	const std::string ragQuerySnapshot = ragQueryInput.data();
	ofLogNotice(kLogModule) << "Starting explicit allowlisted " << selectedTool << " loop on " << selectedBackend;
	toolLoopFuture = std::async(std::launch::async, executeToolLoop, endpointBaseUrl, endpointModel, endpointApiKey,
		selectedTool, ecosystemPath, ragSourceRootSnapshot, ragQuerySnapshot, selectedBackend);
}

void ofApp::invalidateToolLoopResult() {
	if (toolLoopState == ToolLoopState::Running) return;
	toolLoopResult = {};
	toolLoopState = ToolLoopState::Idle;
}

void ofApp::logHandoff() const {
	ofLogNotice(kLogModule) << "\n" << handoffText;
}

void ofApp::drawBullets(const std::vector<std::string> & items) {
	for (const auto & item : items) {
		ImGui::BulletText("%s", item.c_str());
	}
}
