#include "gtkutil/dialog.h"
#include "DarkModRCFClient.h"

#include "DarkRadiantRCFServer.h"
#include "D3ProcessChecker.h"
#include <gtk/gtkmain.h>

#include "stream/stringstream.h"
#include "stream/textstream.h"

namespace {
	// This command must exist in D3
	const std::string SIGNAL_DONE_CMD("darkradiant_signal_cmd_done");
}

// Platform-specific Sleep(int msec) definition
#ifdef WIN32
	#include <windows.h>
	#define INTERVAL 1 // ms
#else
	// Linux doesn't know Sleep(), add a substitute def
	#include <unistd.h>
	#define Sleep(x) usleep(static_cast<int>(1000 * (x)))
	#define INTERVAL 0.1 // ms
#endif 

DarkModRCFClient::DarkModRCFClient() :
	_client(RCF::TcpEndpoint("localhost", PORT_NUMBER))
{}

void DarkModRCFClient::executeCommand(const std::string& command) {
	
	if (!D3ProcessChecker::D3IsRunning()) {
		gtkutil::errorDialog(
			"Doom 3/DarkMod process not running.", GlobalRadiant().getMainWindow()
		);
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
		std::string message = command + "\n" + SIGNAL_DONE_CMD + "\n";
		
		// Send the command to the DarkMod RCF Server
		_client.executeConsoleCommand(RCF::Oneway, message);
	
		globalOutputStream() << "Command sent to DarkMod...\n";
	
		int d3CheckTicker(0);
		int d3CheckInterval(static_cast<int>(10000/INTERVAL)); // every 10 seconds
		
		while (!_server->commandIsDone()) {
			_server->cycle();
	
			Sleep(INTERVAL);
			
			// Process GUI events
			while (gtk_events_pending()) {
				gtk_main_iteration();
			}
	
			d3CheckTicker++;
	
			// Check for a running D3 instance every 10 seconds
			if (d3CheckTicker > d3CheckInterval && !D3ProcessChecker::D3IsRunning()) {
				d3CheckTicker = 0;
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
