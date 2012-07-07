#include "ConversationCommandInfo.h"

#include "string/string.h"
#include "itextstream.h"
#include "eclass.h"

namespace conversation {

// initialise the static member variable
int ConversationCommandInfo::_highestId = 0;

void ConversationCommandInfo::parseFromEntityClass(const IEntityClassPtr& eclass)
{
	assert(eclass != NULL); // don't accept NULL pointers

	name = eclass->getAttribute("editor_cmdName").getValue();
	waitUntilFinishedAllowed = (eclass->getAttribute("editor_waitUntilFinishedAllowed").getValue() == "1");
	sentence = eclass->getAttribute("editor_sentence").getValue();

	// Read the arguments
	// Find all attributes matching "argType", this spawnarg is mandatory
    eclass::AttributeList argTypes = eclass::getSpawnargsWithPrefix(
        *eclass, "editor_argType"
    );

	for (eclass::AttributeList::const_iterator i = argTypes.begin();
         i != argTypes.end();
         ++i)
    {
		// Cut off the "editor_argType" part and retrieve the number
		std::string argIndex = i->getName().substr(14);

		ArgumentInfo info;

		info.required = (eclass->getAttribute("editor_argRequired" + argIndex).getValue() != "0");
		info.description = eclass->getAttribute("editor_argDesc" + argIndex).getValue();
		info.title = eclass->getAttribute("editor_argTitle" + argIndex).getValue();

		std::string argTypeStr = eclass->getAttribute("editor_argType" + argIndex).getValue();
		if (argTypeStr == "float") {
			info.type = ArgumentInfo::ARGTYPE_FLOAT;
		}
		else if (argTypeStr == "int") {
			info.type = ArgumentInfo::ARGTYPE_INT;
		}
		else if (argTypeStr == "string") {
			info.type = ArgumentInfo::ARGTYPE_STRING;
		}
		else if (argTypeStr == "vector") {
			info.type = ArgumentInfo::ARGTYPE_VECTOR;
		}
		else if (argTypeStr == "soundshader") {
			info.type = ArgumentInfo::ARGTYPE_SOUNDSHADER;
		}
		else if (argTypeStr == "actor") {
			info.type = ArgumentInfo::ARGTYPE_ACTOR;
		}
		else if (argTypeStr == "entity") {
			info.type = ArgumentInfo::ARGTYPE_ENTITY;
		}
		else if (argTypeStr == "bool") {
			info.type = ArgumentInfo::ARGTYPE_BOOL;
		}
		else {
			rError() << "Could not determine Conversation Command Argument type: " <<
				argTypeStr << " on entityDef " << eclass->getName() << std::endl;
		}

		// add the argument to the local list
		arguments.push_back(info);
	}
}

} // namespace conversation
