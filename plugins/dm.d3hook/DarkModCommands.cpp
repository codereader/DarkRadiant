#include "DarkModCommands.h"

#include "gtkutil/dialog.h"

#include "iscenegraph.h"
#include "ieventmanager.h"
#include "nameable.h"
#include "DarkModRCFClient.h"

#include <boost/algorithm/string/predicate.hpp>

std::string DarkModCommands::getMapName() {
	// Get the map name
	// Note: this is a temporary way to retrieve the map name 
	// until a proper IMap interface is in place
	NameablePtr rootNameable = 
		boost::dynamic_pointer_cast<Nameable>(GlobalSceneGraph().root());
	
	std::string fullName("");
	
	if (rootNameable != NULL && rootNameable->name().size() > 0) {
		fullName = rootNameable->name();
	
		// Substract the maps/ prefix
		if (boost::algorithm::istarts_with(fullName, "maps/")) {
			fullName = fullName.substr(5, fullName.size());
		}
	}
	
	return fullName;
}

// Compiles the map (including or excluding AAS)
void DarkModCommands::compileMap(bool noAAS) {
	// Prevent re-entering this method
	static bool mutex(false);
	
	if (mutex) {
		return;
	}
	
	mutex = true;
	
	std::string mapName = getMapName();
	
	if (!mapName.empty()) {
		// Show the console during compile
		IEventPtr toggleConsoleEvent = GlobalEventManager().findEvent("ToggleConsole");
		toggleConsoleEvent->keyDown();

		// Instantiate a client and issue the command
		DarkModRCFClient client;
		
		// Assemble the command
		std::string command = "dmap ";
		command += noAAS ? "noaas " : "";
		command += mapName;
		
		client.executeCommand(command);
	}
	else {
		gtkutil::errorDialog("Cannot compile empty or unnamed map.", GlobalRadiant().getMainWindow());
	}
	
	mutex = false;
}

void DarkModCommands::compileMap() {
	// Compile including AAS
	compileMap(false);
}

void DarkModCommands::compileMapNoAAS() {
	// Compile excluding AAS
	compileMap(true);
}

void DarkModCommands::runAAS() {
	// Prevent re-entering this method
	static bool mutex(false);
	
	if (mutex) {
		return;
	}
	
	mutex = true;
	
	std::string mapName = getMapName();
	
	if (!mapName.empty()) {
		// Show the console during compile
		IEventPtr toggleConsoleEvent = GlobalEventManager().findEvent("ToggleConsole");
		toggleConsoleEvent->keyDown();

		// Instantiate a client and issue the command
		DarkModRCFClient client;
		
		// Assemble the command
		std::string command = "runaas " + mapName;
		client.executeCommand(command);
	}
	else {
		gtkutil::errorDialog("Cannot compile AAS of empty or unnamed map.", GlobalRadiant().getMainWindow());
	}
	
	mutex = false;
}
