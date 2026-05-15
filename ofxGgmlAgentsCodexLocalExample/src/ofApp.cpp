#include "ofApp.h"

#include <cstdlib>
#include <sstream>

namespace {
constexpr const char * LogModule = "ofxGgmlAgentsCodexLocalExample";
}

std::string ofApp::getEnvOrDefault(const std::string & name, const std::string & fallback) const {
	const char * value = std::getenv(name.c_str());
	if (value == nullptr || std::string(value).empty()) {
		return fallback;
	}
	return value;
}

void ofApp::appendWrapped(const std::string & text, std::size_t maxChars) {
	if (text.size() <= maxChars) {
		lines.push_back(text);
		return;
	}

	std::istringstream words(text);
	std::string word;
	std::string line;
	while (words >> word) {
		if (!line.empty() && line.size() + word.size() + 1 > maxChars) {
			lines.push_back(line);
			line.clear();
		}
		if (!line.empty()) {
			line += " ";
		}
		line += word;
	}
	if (!line.empty()) {
		lines.push_back(line);
	}
}

void ofApp::setup() {
	ofSetWindowTitle("ofxGgmlAgents Codex Local Example");
	ofBackground(18);

	baseUrl = getEnvOrDefault("OFXGGML_AGENT_LLM_BASE_URL", "http://127.0.0.1:8001/v1");
	modelAlias = getEnvOrDefault("OFXGGML_AGENT_LLM_MODEL", "unsloth/GLM-4.7-Flash");

	ofxGgmlAgentsRequest request;
	request.goal = "Use Codex with a local llama-server endpoint";
	request.context = "Provider setup is owned by ofxGgmlLlama. Agent orchestration is owned by ofxGgmlAgents.";

	lines.clear();
	lines.push_back("ofxGgmlAgents Codex Local Example");
	lines.push_back("");
	lines.push_back(ofxGgmlAgentsUtils::describe(request));
	lines.push_back("");
	lines.push_back("Endpoint handoff");
	lines.push_back("  base_url: " + baseUrl);
	lines.push_back("  model:    " + modelAlias);
	lines.push_back("");
	lines.push_back("Codex config.toml sketch");
	lines.push_back("[model_providers.local_llama]");
	lines.push_back("name = \"local-llama\"");
	lines.push_back("base_url = \"" + baseUrl + "\"");
	lines.push_back("wire_api = \"responses\"");
	lines.push_back("");
	lines.push_back("[profiles.ofxggml_local]");
	lines.push_back("model = \"" + modelAlias + "\"");
	lines.push_back("model_provider = \"local_llama\"");
	lines.push_back("");
	lines.push_back("Before using local Codex here");
	appendWrapped("1. Start llama-server from ofxGgmlLlama on the endpoint above.", 92);
	appendWrapped("2. Verify the model id with the server's /v1/models response.", 92);
	appendWrapped("3. Run ofxGgmlCore scripts\\plan-local-codex.bat -Endpoint " + baseUrl + " -Json -SummaryOnly.", 92);
	appendWrapped("4. Keep local Codex scoped to planning, docs, validation, and small reviewable repository patches.", 92);
	lines.push_back("");
	lines.push_back("Environment overrides");
	lines.push_back("  OFXGGML_AGENT_LLM_BASE_URL");
	lines.push_back("  OFXGGML_AGENT_LLM_MODEL");

	for (const auto & line : lines) {
		ofLogNotice(LogModule) << line;
	}
}

void ofApp::draw() {
	ofBackground(18);
	ofSetColor(240);
	const int left = 32;
	int y = 44;
	for (const auto & line : lines) {
		ofDrawBitmapString(line, left, y);
		y += 22;
	}
}
