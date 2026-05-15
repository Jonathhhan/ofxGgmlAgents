#pragma once

#include "ofMain.h"
#include "ofxGgmlAgents.h"

#include <string>
#include <vector>

class ofApp : public ofBaseApp {
public:
	void setup() override;
	void draw() override;

private:
	std::string baseUrl;
	std::string modelAlias;
	std::vector<std::string> lines;

	std::string getEnvOrDefault(const std::string & name, const std::string & fallback) const;
	void appendWrapped(const std::string & text, std::size_t maxChars);
};
