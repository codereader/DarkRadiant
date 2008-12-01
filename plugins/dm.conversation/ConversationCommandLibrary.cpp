#include "ConversationCommandLibrary.h"

#include <gtk/gtkliststore.h>
#include "ieclass.h"
#include "iregistry.h"
#include "string/string.h"
#include <boost/algorithm/string/predicate.hpp>

namespace conversation {

/** greebo: The visitor class that stores all the relevant eclassptrs
 * 			into the given target map if the prefix matches.
 */
class ConversationCommandInfoLoader :
	public EntityClassVisitor
{
	// The target map to populate
	ConversationCommandInfoMap& _map;
	
	// The entityDef prefix (e.g. "atdm:conversation_command_")
	std::string _prefix;

public:
	/** 
	 * greebo: Pass the target map where all the eclassptrs should be stored into. 
	 */
	ConversationCommandInfoLoader(ConversationCommandInfoMap& map) :
		_map(map),
		_prefix(GlobalRegistry().get(RKEY_CONVERSATION_COMMAND_INFO_PREFIX))
	{}
	
	void visit(IEntityClassPtr eclass) {
		if (boost::algorithm::starts_with(eclass->getName(), _prefix)) {
			// We have a match, create a new structure
			ConversationCommandInfoPtr commandInfo(new ConversationCommandInfo);

			// Fill the values from the found entityDef
			commandInfo->parseFromEntityClass(eclass);

			// Store the structure to the target map
			_map[commandInfo->name] = commandInfo;
		}
	}
};

ConversationCommandLibrary::ConversationCommandLibrary() {
	loadConversationCommands();
}

const ConversationCommandInfo& ConversationCommandLibrary::findCommandInfo(const std::string& name) {
	ConversationCommandInfoMap::const_iterator i = _commandInfo.find(name);

	if (i == _commandInfo.end()) {
		throw std::runtime_error(std::string("Could not find command info with the given name: ") + name);
	}

	return *(i->second);
}

const ConversationCommandInfo& ConversationCommandLibrary::findCommandInfo(int id) {
	
	for (ConversationCommandInfoMap::const_iterator i = _commandInfo.begin(); 
		 i != _commandInfo.end(); 
		 ++i)
	{
		if (i->second->id == id) {
			return *(i->second);
		}
	}

	throw std::runtime_error(std::string("Could not find command info with the given ID: ") + intToStr(id));
}

void ConversationCommandLibrary::loadConversationCommands() {
	// Load the possible command types
	ConversationCommandInfoLoader loader(_commandInfo);
	GlobalEntityClassManager().forEach(loader);
}

void ConversationCommandLibrary::populateListStore(GtkListStore* store) {
	// Iterate over everything and push the data into the liststore
	for (ConversationCommandInfoMap::const_iterator i = _commandInfo.begin(); 
		 i != _commandInfo.end(); 
		 ++i)
	{
		GtkTreeIter iter;
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 
						   0, i->second->id, 
						   1, i->second->name.c_str(),
						   -1);
	}
}

// Static accessor
ConversationCommandLibrary& ConversationCommandLibrary::Instance() {
	static ConversationCommandLibrary _instance;
	return _instance;
}

} // namespace conversation
