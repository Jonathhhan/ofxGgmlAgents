#pragma once

#include "ofxGgmlAgentsTypes.h"

#include <string>

namespace ofxGgmlAgentsUtils {
	// True when any request field carries meaningful planning input.
	bool hasInput(const ofxGgmlAgentsRequest & request);

	// Returns a deterministic, human-readable summary for smoke tests and logs.
	std::string describe(const ofxGgmlAgentsRequest & request);
}
