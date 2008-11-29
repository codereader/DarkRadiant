#ifndef CONVERSATION_H_
#define CONVERSATION_H_

#include <string>
#include <map>
#include <list>

namespace conversation {

/**
 * Data object representing a single Conversation.
 */
class Conversation
{
public:

	// The (unique) name of this conversation
	std::string name;
		
	// Constructor
	Conversation()
	{}
};

/**
 * Conversation map type.
 */
typedef std::map<int, Conversation> ConversationMap;

}

#endif /* CONVERSATION_H_ */
