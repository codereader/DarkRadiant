#include "ConversationKeyExtractor.h"

#include "itextstream.h"

#include <regex>

#include "string/convert.h"
#include "string/predicate.h"
#include "ConversationCommand.h"
#include "ConversationCommandLibrary.h"

namespace conversation 
{

ConversationKeyExtractor::ConversationKeyExtractor(ConversationMap& map) :
	_convMap(map),
	_regexConvNum("conv_(\\d+)_(.*)"), 
	_regexConvCmd("cmd_(\\d+)_(.*)")
{
	assert(_convMap.empty());
}

void ConversationKeyExtractor::operator()(const std::string& key, const std::string& value) 
{
	// Quick discard of any non-conversation keys
	if (!string::starts_with(key, "conv")) return;

	// Extract the conv index
	std::smatch results;
	if (!std::regex_match(key, results, _regexConvNum)) 
	{
		// No match, abort
		return;
	}

	// Get the conversation number
	int num = string::convert<int>(results[1].str());

	// We now have the conversation number and the substring (everything after
	// "conv_<n>_" which applies to this conversation.
	std::string convSubString = results[2];

	// Switch on the substring
	if (convSubString == "name") 
	{
		_convMap[num].name = value;
	}
	else if (convSubString == "talk_distance") 
	{
		_convMap[num].talkDistance = string::convert<float>(value, 60);
	}
	else if (convSubString == "actors_must_be_within_talkdistance")
	{
		_convMap[num].actorsMustBeWithinTalkdistance = (value == "1");
	}
	else if (convSubString == "actors_always_face_each_other_while_talking")
	{
		_convMap[num].actorsAlwaysFaceEachOther = (value == "1");
	}
	else if (convSubString == "max_play_count")
	{
		_convMap[num].maxPlayCount = string::convert<int>(value, -1);
	}
	else if (convSubString.substr(0, 6) == "actor_")
	{
		// This is an actor definition, extract the number
		int actorNum = string::convert<int>(convSubString.substr(6), -1);

		if (actorNum == -1)
		{
			return;
		}

		// Store the actor in the map
		_convMap[num].actors.emplace(actorNum, value);
	}
	else if (convSubString.substr(0, 4) == "cmd_") 
	{
		// This is a conversation command
		std::smatch cmdResults;

		if (!std::regex_match(convSubString, cmdResults, _regexConvCmd))
		{
			return; // not matching
		}

		int cmdIndex = string::convert<int>(cmdResults[1].str());
		std::string cmdSubStr = cmdResults[2];

		ConversationCommandPtr command;
		auto found = _convMap[num].commands.find(cmdIndex);

		if (found != _convMap[num].commands.end())
		{
			// Command already exists
			command = found->second;
		}
		else 
		{
			// Command with the given index does not exist yet, create it
			command = std::make_shared<ConversationCommand>();

			// Insert this into the map
			_convMap[num].commands[cmdIndex] = command;
		}

		if (cmdSubStr == "type")
		{
			try 
			{
				command->type = ConversationCommandLibrary::Instance().findCommandInfo(value).id;
			}
			catch (std::runtime_error& e) 
			{
				rError() << e.what() << std::endl;
				return;
			}
		}
		else if (cmdSubStr == "actor") 
		{
			command->actor = string::convert<int>(value);
		}
		else if (cmdSubStr == "wait_until_finished")
		{
			command->waitUntilFinished = (value == "1");
		}
		else if (cmdSubStr.substr(0,4) == "arg_") 
		{
			int cmdArgIndex = string::convert<int>(cmdSubStr.substr(4));

			command->arguments[cmdArgIndex] = value;
		}
	}
}

} // namespace conversation
