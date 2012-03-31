#include "ConversationCommand.h"

#include "string/convert.h"
#include <boost/algorithm/string/replace.hpp>

#include "ConversationCommandLibrary.h"

namespace conversation {

ConversationCommand::ConversationCommand() :
	type(-1), // invalid id
	actor(-1),
	waitUntilFinished(true)
{}

std::string ConversationCommand::getArgument(int index) const {
	ArgumentMap::const_iterator i = arguments.find(index);

	return (i != arguments.end()) ? i->second : "";
}

std::string ConversationCommand::getSentence() const {
	// Get the command description for this type
	try {
		const ConversationCommandInfo& cmdInfo =
			ConversationCommandLibrary::Instance().findCommandInfo(type);

		// Get the sentence and fill in the placeholders, if any
		std::string sentence = cmdInfo.sentence;

		int counter = 1;

		for (ConversationCommandInfo::ArgumentInfoList::const_iterator i = cmdInfo.arguments.begin();
			 i != cmdInfo.arguments.end(); ++i, ++counter)
		{
			std::string needle = "[arg" + string::to_string(counter) + "]";
			std::string replacement = getArgument(counter);

			// Check for a bool
			/*if (i->second.type == "b") {
				replacement = (i->second.value.empty()) ? "no" : "yes";
			}*/

			boost::algorithm::replace_all(sentence, needle, replacement);
		}

		return sentence;
	}
	catch (std::runtime_error&) {
		return "Unrecognised command.";
	}
}

} // namespace conversation
