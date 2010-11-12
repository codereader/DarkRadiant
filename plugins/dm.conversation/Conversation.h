#ifndef CONVERSATION_H_
#define CONVERSATION_H_

#include <string>
#include <map>
#include <vector>

#include "ConversationCommand.h"

namespace conversation {

/**
 * Data object representing a single Conversation.
 */
class Conversation
{
public:

	// The (unique) name of this conversation
	std::string name;

	float talkDistance;
	bool actorsMustBeWithinTalkdistance;
	bool actorsAlwaysFaceEachOther;
	int maxPlayCount;

	// Indexed list of commands
	typedef std::map<int, ConversationCommandPtr> CommandMap;
	CommandMap commands;

	// Indexed list of actors (by name)
	typedef std::map<int, std::string> ActorMap;
	ActorMap actors;

	// Constructor
	Conversation() :
		talkDistance(60),
		actorsMustBeWithinTalkdistance(true),
		actorsAlwaysFaceEachOther(true),
		maxPlayCount(-1)
	{}

	// Copy Constructor
	Conversation(const Conversation& other) :
		name(other.name),
		talkDistance(other.talkDistance),
		actorsMustBeWithinTalkdistance(other.actorsMustBeWithinTalkdistance),
		actorsAlwaysFaceEachOther(other.actorsAlwaysFaceEachOther),
		maxPlayCount(other.maxPlayCount),
		actors(other.actors)
	{
		// Copy all commands, one by one
		for (CommandMap::const_iterator i = other.commands.begin();
			 i != other.commands.end(); ++i)
		{
			// Copy-construct a new conversation command
			ConversationCommandPtr copy(new ConversationCommand(*i->second));

			// Insert this into our own map
			commands[i->first] = copy;
		}
	}
};

/**
 * Conversation map type.
 */
typedef std::map<int, Conversation> ConversationMap;

}

#endif /* CONVERSATION_H_ */
