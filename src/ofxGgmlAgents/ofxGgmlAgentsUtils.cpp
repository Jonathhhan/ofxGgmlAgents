#include "ofxGgmlAgentsUtils.h"

namespace ofxGgmlAgentsUtils {
	namespace {
		bool hasText(const std::string & value) {
			return value.find_first_not_of(" \t\r\n") != std::string::npos;
		}

		const std::string * firstToolInput(const ofxGgmlAgentsRequest & request) {
			for (const auto & tool : request.tools) {
				if (hasText(tool)) {
					return &tool;
				}
			}
			return nullptr;
		}
	}

	bool hasInput(const ofxGgmlAgentsRequest & request) {
		return hasText(request.goal) ||
			hasText(request.prompt) ||
			hasText(request.context) ||
			firstToolInput(request) != nullptr;
	}

	std::string describe(const ofxGgmlAgentsRequest & request) {
		if (!hasInput(request)) {
			return "agents: empty request";
		}
		if (hasText(request.goal)) {
			return "agents: " + request.goal;
		}
		if (hasText(request.prompt)) {
			return "agents: " + request.prompt;
		}
		if (hasText(request.context)) {
			return "agents: " + request.context;
		}
		const std::string * tool = firstToolInput(request);
		if (tool != nullptr) {
			return "agents: tool handoff: " + *tool;
		}
		return "agents: empty request";
	}
}
