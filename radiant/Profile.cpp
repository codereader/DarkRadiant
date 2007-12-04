#include "Profile.h"

#include "iregistry.h"
#include <iostream>
#include "map/Map.h"

namespace profile {

	namespace {
		const std::string RKEY_AUTOMATED_TEST_ROOT = "user/debug/automatedTest";
		const std::string RKEY_AUTOMATED_TEST = RKEY_AUTOMATED_TEST_ROOT + "/runTest";
		const std::string RKEY_TESTMAP = RKEY_AUTOMATED_TEST_ROOT + "/testMap";
		const std::string RKEY_TESTCMDS = RKEY_AUTOMATED_TEST_ROOT + "/testCommands//testCommand";
	}

bool CheckAutomatedTestRun() {
	if (GlobalRegistry().get(RKEY_AUTOMATED_TEST) != "1") {
		return false;
	}
	
	std::cout << "Running automated test...\n";
	
	GlobalMap().load(GlobalRegistry().get(RKEY_TESTMAP));
	
	return true;
}

}
