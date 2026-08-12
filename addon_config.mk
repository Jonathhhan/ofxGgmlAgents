meta:
	ADDON_NAME = ofxGgmlAgents
	ADDON_DESCRIPTION = Standalone addon for local agent orchestration workflows
	ADDON_AUTHOR = Jonathan Frank
	ADDON_TAGS = "ggml,ai,agents,tools,orchestration"
	ADDON_URL = https://github.com/Jonathhhan/ofxGgmlAgents

common:
	ADDON_INCLUDES += src
	ADDON_SOURCES_EXCLUDE += build/%
	ADDON_SOURCES_EXCLUDE += libs/*/build/%
	ADDON_SOURCES_EXCLUDE += libs/*/build*/%
	ADDON_INCLUDES_EXCLUDE += build/%
	ADDON_INCLUDES_EXCLUDE += libs/*/build/%
	ADDON_INCLUDES_EXCLUDE += libs/*/build*/%
