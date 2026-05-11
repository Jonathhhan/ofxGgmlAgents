#pragma once

#include "ofxGgmlAgentsTypes.h"

#include <string>

namespace ofxGgmlAgentsUtils {
	bool hasInput(const ofxGgmlAgentsRequest & request);
	std::string describe(const ofxGgmlAgentsRequest & request);
}