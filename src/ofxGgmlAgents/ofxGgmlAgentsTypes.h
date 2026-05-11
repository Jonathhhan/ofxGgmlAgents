#pragma once

#include <string>
#include <vector>

struct ofxGgmlAgentsRequest {
	std::string goal;
	std::string prompt;
	std::vector<std::string> tools;
	std::string context;
};

struct ofxGgmlAgentsResult {
	bool success = false;
	std::string text;
	std::string error;
	std::vector<std::string> references;

	explicit operator bool() const {
		return success;
	}
};