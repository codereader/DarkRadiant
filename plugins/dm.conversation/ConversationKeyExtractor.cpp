#include "ConversationKeyExtractor.h"

#include "itextstream.h"

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/classification.hpp>

#include "string/convert.h"
#include "ConversationCommand.h"
#include "ConversationCommandLibrary.h"

namespace conversation {

// Shortcut for boost::algorithm::split
typedef std::vector<std::string> StringParts;

// Required entity visit function
void ConversationKeyExtractor::visit(const std::string& key, const std::string& value) {
	// Quick discard of any non-conversation keys
	if (key.substr(0, 4) != "conv") return;

	// Extract the objective number
	static const boost::regex reConvNum("conv_(\\d+)_(.*)");
	boost::smatch results;
	int iNum;

	if (!boost::regex_match(key, results, reConvNum)) {
		// No match, abort
		return;
	}

	// Get the conversation number
	iNum = string::convert<int>(results[1]);

	// We now have the conversation number and the substring (everything after
	// "conv_<n>_" which applies to this conversation.
	std::string convSubString = results[2];

	// Switch on the substring
	if (convSubString == "name") {
		_convMap[iNum].name = value;
	}
	else if (convSubString == "talk_distance") {
		_convMap[iNum].talkDistance = string::convert<float>(value, 60);
	}
	else if (convSubString == "actors_must_be_within_talkdistance") {
		_convMap[iNum].actorsMustBeWithinTalkdistance = (value == "1");
	}
	else if (convSubString == "actors_always_face_each_other_while_talking") {
		_convMap[iNum].actorsAlwaysFaceEachOther = (value == "1");
	}
	else if (convSubString == "max_play_count") {
		_convMap[iNum].maxPlayCount = string::convert<int>(value, -1);
	}
	else if (convSubString.substr(0, 6) == "actor_") {
		// This is an actor definition, extract the number
		int actorNum = string::convert<int>(convSubString.substr(6), -1);

		if (actorNum == -1) {
			return;
		}

		// Store the actor in the map
		_convMap[iNum].actors.insert(Conversation::ActorMap::value_type(actorNum, value));
	}
	else if (convSubString.substr(0, 4) == "cmd_") {
		// This is a conversation command, form a new regex
		static const boost::regex reCommand("cmd_(\\d+)_(.*)");
		boost::smatch results;

		if (!boost::regex_match(convSubString, results, reCommand)) {
			return; // not matching
		}

		int cmdIndex = string::convert<int>(results[1]);
		std::string cmdSubStr = results[2];

		ConversationCommandPtr command;
		Conversation::CommandMap::iterator found = _convMap[iNum].commands.find(cmdIndex);

		if (found != _convMap[iNum].commands.end()) {
			// Command already exists
			command = found->second;
		}
		else {
			// Command with the given index does not exist yet, create it
			command = ConversationCommandPtr(new ConversationCommand);

			// Insert this into the map
			_convMap[iNum].commands[cmdIndex] = command;
		}

		if (cmdSubStr == "type") {
			try {
				command->type = ConversationCommandLibrary::Instance().findCommandInfo(value).id;
			}
			catch (std::runtime_error& e) {
				rError() << e.what() << std::endl;
				return;
			}
		}
		else if (cmdSubStr == "actor") {
			command->actor = string::convert<int>(value);
		}
		else if (cmdSubStr == "wait_until_finished") {
			command->waitUntilFinished = (value == "1");
		}
		else if (cmdSubStr.substr(0,4) == "arg_") {
			int cmdArgIndex = string::convert<int>(cmdSubStr.substr(4));

			command->arguments[cmdArgIndex] = value;
		}
	}
}

} // namespace conversation
