meta:
	ADDON_NAME = ofxGgmlAgents
	ADDON_DESCRIPTION = Companion addon for local agent orchestration workflows on top of ofxGgmlCore
	ADDON_AUTHOR = Jonathan Frank
	ADDON_TAGS = "ggml,ai,agents,tools,orchestration"
	ADDON_URL = https://github.com/Jonathhhan/ofxGgmlAgents

common:
	ADDON_DEPENDENCIES += ofxGgmlCore
	ADDON_INCLUDES += src
	ADDON_SOURCES_EXCLUDE += build/%
	ADDON_SOURCES_EXCLUDE += libs/*/build/%
	ADDON_SOURCES_EXCLUDE += libs/*/build*/%
	ADDON_INCLUDES_EXCLUDE += build/%
	ADDON_INCLUDES_EXCLUDE += libs/*/build/%
	ADDON_INCLUDES_EXCLUDE += libs/*/build*/%