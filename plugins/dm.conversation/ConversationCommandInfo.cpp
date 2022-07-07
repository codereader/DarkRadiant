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

	name = eclass->getAttributeValue("editor_cmdName");
	waitUntilFinishedAllowed = (eclass->getAttributeValue("editor_waitUntilFinishedAllowed") == "1");
	sentence = eclass->getAttributeValue("editor_sentence");

	// Read the arguments
	// Find all attributes matching "argType", this spawnarg is mandatory
    eclass::AttributeList argTypes = eclass::getSpawnargsWithPrefix(
        eclass, "editor_argType"
    );

	for (eclass::AttributeList::const_iterator i = argTypes.begin();
         i != argTypes.end();
         ++i)
    {
		// Cut off the "editor_argType" part and retrieve the number
		std::string argIndex = i->getName().substr(14);

		ArgumentInfo info;

		info.required = (eclass->getAttributeValue("editor_argRequired" + argIndex) != "0");
		info.description = eclass->getAttributeValue("editor_argDesc" + argIndex);
		info.title = eclass->getAttributeValue("editor_argTitle" + argIndex);

		std::string argTypeStr = eclass->getAttributeValue("editor_argType" + argIndex);
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
				argTypeStr << " on entityDef " << eclass->getDeclName() << std::endl;
		}

		// add the argument to the local list
		arguments.push_back(info);
	}
}

} // namespace conversation
