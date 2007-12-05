#include "Profile.h"

#include "iregistry.h"
#include "ieventmanager.h"
#include <iostream>
#include "map/Map.h"
#include "debugging/ScopedDebugTimer.h"
#include "string/string.h"
#include <gtk/gtkmain.h>

namespace profile {

	namespace {
		const std::string RKEY_AUTOMATED_TEST_ROOT = "debug/automatedTest";
		const std::string RKEY_AUTOMATED_TEST = RKEY_AUTOMATED_TEST_ROOT + "/runTest";
		const std::string RKEY_TESTMAP = RKEY_AUTOMATED_TEST_ROOT + "/testMap";
		const std::string RKEY_TESTCMDS = RKEY_AUTOMATED_TEST_ROOT + "/testCommands//testCommand";
	}

bool CheckAutomatedTestRun() {
	if (GlobalRegistry().get(RKEY_AUTOMATED_TEST) != "1") {
		return false;
	}
	
	std::cout << "Running automated test...\n";
	
	std::string testMap = GlobalRegistry().get(RKEY_TESTMAP);
	
	if (!testMap.empty()) {
		std::cout << "Loading map: " << testMap << "\n";
		ScopedDebugTimer mapLoadTimer("Loaded map " + testMap);
		GlobalMap().load(testMap);
	}
	
	ScopedDebugTimer timer("Executed all commands");
	
	xml::NodeList list = GlobalRegistry().findXPath(RKEY_TESTCMDS);
	
	for (std::size_t i = 0; i < list.size(); i++) {
		std::string eventName = list[i].getAttributeValue("value");
		int count = strToInt(list[i].getAttributeValue("count"));
		
		ScopedDebugTimer overallTimer("---Executed command " + eventName + " " + intToStr(count) + " times");
		
		std::cout << "--- Executing command " << eventName << " " << count << " times\n";
		for (int c = 0; c < count; c++) {
			IEventPtr event = GlobalEventManager().findEvent(eventName);
			if (event != NULL) {
				ScopedDebugTimer timer("Executing command " + eventName);
				
				// Fire the event
				event->keyDown();
			}
			else {
				std::cout << "Event not found: " << event << "\n";
				break;
			}
			
			// Process the GUI events
			while (gtk_events_pending()) {
				gtk_main_iteration();
			}
		}
	}
	
	return true;
}

}
