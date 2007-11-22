#ifndef DARKMODRCFCLIENT_H_
#define DARKMODRCFCLIENT_H_

#include "RCFInterface.h"
#include <RCF/TcpEndpoint.hpp>

class DarkModRCFClient
{
	RcfClient<D3DarkModInterface> _client;
public:
	static const int PORT_NUMBER = 50001;
	
	DarkModRCFClient();
	
	/**
	 * Sends the given command string to the D3 console and
	 * redirects the console output to DarkRadiant's console
	 * while DarkMod is processing the command.
	 * 
	 * Automatically handles the buffer redirect and wait loops.
	 */
	void executeCommand(const std::string& command);
};

#endif /*DARKMODRCFCLIENT_H_*/
