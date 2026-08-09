#include "ofMain.h"
#include "ofApp.h"

#include <cstdlib>
#include <iostream>

namespace {
	std::string readApiKey() {
#if defined(_MSC_VER)
		char * value = nullptr;
		size_t length = 0;
		if (_dupenv_s(&value, &length, "OFXGGML_AGENT_LLM_API_KEY") != 0 || value == nullptr) return "";
		std::string result(value);
		std::free(value);
		return result;
#else
		const char * value = std::getenv("OFXGGML_AGENT_LLM_API_KEY");
		return value ? value : "";
#endif
	}
}

int main(int argc, char ** argv) {
	if (argc == 5 && std::string(argv[1]) == "--tool-loop-smoke") {
		const auto result = ofApp::runToolLoopForSmoke(argv[2], argv[3], readApiKey(), argv[4]);
		ofJson output = {
			{"success", result.success}, {"tool", "get_proven_lanes"},
			{"tool_call_encoding", result.toolCallEncoding}, {"proven_lanes", result.provenLanes},
			{"final_confirmation", result.finalConfirmation}, {"elapsed_ms", result.elapsedMs},
			{"error", result.error}, {"events", result.events}
		};
		std::cout << output.dump(2) << std::endl;
		return result.success ? 0 : 1;
	}
	ofSetupOpenGL(960, 540, OF_WINDOW);
	ofRunApp(new ofApp());
}
