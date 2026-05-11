cmake_minimum_required(VERSION 3.18)
project(ofxGgmlAgents_tests CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ofxGgmlAgents_tests
	test_main.cpp
	../src/ofxGgmlAgents/ofxGgmlAgentsUtils.cpp)

target_include_directories(ofxGgmlAgents_tests PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/../src)

enable_testing()
add_test(NAME ofxGgmlAgents_tests COMMAND ofxGgmlAgents_tests)