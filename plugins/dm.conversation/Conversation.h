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
};

/**
 * Conversation map type.
 */
typedef std::map<int, Conversation> ConversationMap;

}

#endif /* CONVERSATION_H_ */
