#ifndef __DARKRADIANT_RCF_SERVER_H__
#define __DARKRADIANT_RCF_SERVER_H__

#include <boost/shared_ptr.hpp>
#include <RCF/RcfServer.hpp>
#include "RCFInterface.h"

/**
 * greebo: This class encapsulates an RCF Server instance listening
 *         for incoming requests on localhost, port 50002.
 *
 * Instantiation of this object is sufficient for starting the RCF server.
 *
 * Note: the RCF interface functions must exactly match the ones defined in 
 * the declaration file within the D3 Game project.
 */
class DarkRadiantRCFServer
{
	// The actual RCF Server instance
	RCF::RcfServer _server;

	// TRUE as soon as D3 emits the COMMAND DONE signal
	bool _commandIsDone;

public:
	static const int PORT_NUMBER = 50002; 
	
	// Constructor starts the server thread
	DarkRadiantRCFServer();

	// Destructor stops the thread
	~DarkRadiantRCFServer();

	// Handle the incoming events
	void cycle();

	// Returns TRUE as soon as D3 emits the COMMAND DONE signal.
	bool commandIsDone() const;

	// --- DarkRadiant RCF interface goes below ----
	void writeToConsole(const std::string& text);
	void signalCommandDone();
};
typedef boost::shared_ptr<DarkRadiantRCFServer> DarkRadiantRCFServerPtr;

#endif /* __DARKRADIANT_RCF_SERVER_H__ */
