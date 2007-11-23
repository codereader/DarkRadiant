#include "DarkModCommands.h"

#include "gtkutil/dialog.h"

#include "iscenegraph.h"
#include "ieventmanager.h"
#include "nameable.h"
#include "DarkModRCFClient.h"

#include <boost/algorithm/string/predicate.hpp>

namespace darkmodcommands {

// Compiles the map (including or excluding AAS)
void CompileMap(bool noAAS) {
	// Get the map name
	// Note: this is a temporary way to retrieve the map name 
	// until a proper IMap interface is in place
	NameablePtr rootNameable = 
		boost::dynamic_pointer_cast<Nameable>(GlobalSceneGraph().root());

	if (rootNameable != NULL && rootNameable->name().size() > 0) {
		std::string fullName(rootNameable->name());

		if (boost::algorithm::istarts_with(fullName, "maps/")) {
			fullName = fullName.substr(5, fullName.size());
		}
		
		// Show the console during compile
		IEventPtr toggleConsoleEvent = GlobalEventManager().findEvent("ToggleConsole");
		toggleConsoleEvent->keyDown();

		// Instantiate a client and issue the command
		DarkModRCFClient client;
		
		// Assemble the command
		std::string command = "dmap ";
		command += noAAS ? "noaas " : "";
		command += fullName;
		
		client.executeCommand(command);
	}
	else {
		gtkutil::errorDialog("Cannot compile empty or unnamed map.", GlobalRadiant().getMainWindow());
	}
}

} // namespace darkmodcommands
