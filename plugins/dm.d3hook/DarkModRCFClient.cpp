#include "DarkModRCFClient.h"

#include <gtk/gtkmain.h>
#include "DarkRadiantRCFServer.h"
#include "D3ProcessChecker.h"

#include "stream/stringstream.h"
#include "stream/textstream.h"

namespace {
	// This command must exist in D3
	const std::string SIGNAL_DONE_CMD("darkradiant_signal_cmd_done");
}

// Platform-specific Sleep(int msec) definition
#ifdef WIN32
	#include <windows.h>
#else
	// Linux doesn't know Sleep(), add a substitute def
	#include <unistd.h>
	#define Sleep(x) usleep(1000 * (x))
#endif 

DarkModRCFClient::DarkModRCFClient() :
	_client(RCF::TcpEndpoint("localhost", PORT_NUMBER))
{}

void DarkModRCFClient::executeCommand(const std::string& command) {
	
	if (!D3ProcessChecker::D3IsRunning()) {
		globalErrorStream() << "Doom 3/gamex86.dll not running.\n";
		return;
	}
	
	static DarkRadiantRCFServerPtr _server;

	if (_server != NULL) {
		globalErrorStream() << "RCF Server already running.\n";
		return;
	}
	
	// Construct the server to listen on port 50001
	_server = DarkRadiantRCFServerPtr(new DarkRadiantRCFServer);
	
	try {
		// Start console buffering and let DarkMod know our server port.
		_client.startConsoleBuffering(DarkRadiantRCFServer::PORT_NUMBER);
	
		// Be sure to append the SIGNAL_DONE_CMD string, this notifies us
		// when the command has been executed.
		std::string message = command + "\n" + SIGNAL_DONE_CMD + "\n";;
		
		// Send the command to the DarkMod RCF Server
		_client.executeConsoleCommand(RCF::Oneway, message);
	
		globalOutputStream() << "Command sent to DarkMod...\n";
	
		int d3CheckTime(0);
		
		while (!_server->commandIsDone()) {
			_server->cycle();
	
			Sleep(1);
			
			// Process GUI events
			while (gtk_events_pending()) {
				gtk_main_iteration();
			}
	
			d3CheckTime += 10;
	
			// Check for a running D3 instance all 10 seconds
			if (d3CheckTime > 10000 && !D3ProcessChecker::D3IsRunning()) {
				d3CheckTime = 0;
				break;
			}
		}
	
		// endConsoleBuffering(1) needs not to be called, is done automatically.
	
		// Let the server run a few cycles more (1 second)
		for (int i = 0; i < 100; i++) {
			_server->cycle();
			Sleep(10);
	
			// Process GUI events
			while (gtk_events_pending()) {
				gtk_main_iteration();
			}
		}
	}
	catch (const std::exception &e)	{
		globalErrorStream() << "Caught exception:\n";
		globalErrorStream() << "Type: " << typeid(e).name() << "\n";
		globalErrorStream() << "What: " << e.what() << "\n";
	}
	
	// Clear the server to allow re-entering this function
	_server = DarkRadiantRCFServerPtr();
}

void DarkModRCFClient::writeToConsole(const std::string& text) {
	try {
		// Issue command
		_client.writeToConsole(text);
	}
	catch (const std::exception &e)	{
		globalErrorStream() << "Caught exception:\n";
		globalErrorStream() << "Type: " << typeid(e).name() << "\n";
		globalErrorStream() << "What: " << e.what() << "\n";
	}
}
