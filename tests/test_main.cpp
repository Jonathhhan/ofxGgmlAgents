#include "ofxGgmlAgents.h"

#include <iostream>
#include <string>

namespace {
	bool expect(bool condition, const std::string & message) {
		if (!condition) {
			std::cerr << message << "\n";
			return false;
		}
		return true;
	}

	bool contains(const std::string & value, const std::string & needle) {
		return value.find(needle) != std::string::npos;
	}

	bool testVersionMetadata() {
		if (OFXGGML_AGENTS_VERSION_MAJOR != 1 ||
			OFXGGML_AGENTS_VERSION_MINOR != 0 ||
			OFXGGML_AGENTS_VERSION_PATCH != 1 ||
			std::string(OFXGGML_AGENTS_VERSION_STRING) != "1.0.1" ||
			std::string(ofxGgmlAgentsGetVersionString()) != "1.0.1") {
			std::cerr << "unexpected Agents addon version metadata\n";
			return false;
		}
		return true;
	}

	bool testRequestInputHelpers() {
		ofxGgmlAgentsRequest request;
		if (!expect(!ofxGgmlAgentsUtils::hasInput(request), "empty request reported as configured")) {
			return false;
		}
		if (!expect(ofxGgmlAgentsUtils::describe(request) == "agents: empty request", "empty request description changed")) {
			return false;
		}

		request.goal = " \t";
		request.prompt = "\n";
		request.context = " ";
		request.tools = {"", "  "};
		if (!expect(!ofxGgmlAgentsUtils::hasInput(request), "whitespace-only request reported as configured")) {
			return false;
		}

		request = {};
		request.goal = "plan a local creative coding assistant";
		if (!expect(ofxGgmlAgentsUtils::hasInput(request), "configured goal request reported as empty")) {
			return false;
		}
		if (!expect(contains(ofxGgmlAgentsUtils::describe(request), request.goal), "description did not include request goal")) {
			return false;
		}

		request = {};
		request.prompt = "draft the workflow boundary";
		if (!expect(ofxGgmlAgentsUtils::hasInput(request), "configured prompt request reported as empty")) {
			return false;
		}
		if (!expect(contains(ofxGgmlAgentsUtils::describe(request), request.prompt), "description did not include request prompt")) {
			return false;
		}

		request = {};
		request.context = "addon handoff context";
		if (!expect(ofxGgmlAgentsUtils::hasInput(request), "configured context request reported as empty")) {
			return false;
		}
		if (!expect(contains(ofxGgmlAgentsUtils::describe(request), request.context), "description did not include request context")) {
			return false;
		}

		request = {};
		request.tools = {"", "ofxGgmlLlama endpoint handoff"};
		if (!expect(ofxGgmlAgentsUtils::hasInput(request), "configured tool request reported as empty")) {
			return false;
		}
		if (!expect(contains(ofxGgmlAgentsUtils::describe(request), request.tools[1]), "description did not include request tool")) {
			return false;
		}

		return true;
	}

	bool testResultTruthiness() {
		ofxGgmlAgentsResult result;
		if (!expect(!result, "default result reported success")) {
			return false;
		}
		result.success = true;
		result.text = "planned";
		if (!expect(static_cast<bool>(result), "successful result reported failure")) {
			return false;
		}
		result.success = false;
		result.error = "missing endpoint";
		if (!expect(!result, "failed result reported success")) {
			return false;
		}
		return true;
	}
}

int main() {
	if (!testVersionMetadata() ||
		!testRequestInputHelpers() ||
		!testResultTruthiness()) {
		return 1;
	}

	return 0;
}
