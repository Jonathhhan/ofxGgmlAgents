#include "ofxGgmlAgents.h"

#include <iostream>

int main() {
	ofxGgmlAgentsRequest request;
	if (ofxGgmlAgentsUtils::hasInput(request)) {
		std::cerr << "empty request reported as configured\n";
		return 1;
	}

	request.goal = "plan a local creative coding assistant";
	if (!ofxGgmlAgentsUtils::hasInput(request)) {
		std::cerr << "configured request reported as empty\n";
		return 1;
	}

	const auto description = ofxGgmlAgentsUtils::describe(request);
	if (description.find(request.goal) == std::string::npos) {
		std::cerr << "description did not include request input\n";
		return 1;
	}

	return 0;
}