#include "ofxGgmlAgentsUtils.h"

namespace ofxGgmlAgentsUtils {
	bool hasInput(const ofxGgmlAgentsRequest & request) {
		return !request.goal.empty();
	}

	std::string describe(const ofxGgmlAgentsRequest & request) {
		if (!hasInput(request)) {
			return "agents: empty request";
		}
		return "agents: " + request.goal;
	}
}