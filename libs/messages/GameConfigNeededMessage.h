#pragma once

#include "igame.h"
#include "imessagebus.h"

namespace game
{

/**
 * Message object sent to the MessageBus when the application
 * is lacking a game configuration.
 * 
 * Use the setConfig() method to assign a configuration to 
 * return to the sender.
 *
 * When this message stays unhandled after it went through the
 * MessageBus, the application is unlikely to be able to continue
 * and will probably terminate.
 */
class ConfigurationNeeded :
	public radiant::IMessage
{
private:
	GameConfiguration _config;

public:
	ConfigurationNeeded()
	{}

	const GameConfiguration& getConfig() const
	{
		return _config;
	}

	void setConfig(const GameConfiguration& config)
	{
		_config = config;
	}
};

}
