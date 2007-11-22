#ifndef __RCF_SERVICE_DECL_H__
#define __RCF_SERVICE_DECL_H__

#include <string>
#include <RCF/Idl.hpp>
#include <SF/vector.hpp>

/**
 * greebo: This defines a service method in the D3 gamex86.dll file.
 *         Keep this in sync with the declaration in the main mod code!
 */
RCF_BEGIN(D3DarkModInterface, "DarkMod")
	// Writes the specified string to the DarkMod console
    RCF_METHOD_V1(void, writeToConsole, const std::string&);

	/**
	 * greebo: Sends the given command to the Doom3 Command System
	 *         Be sure to append the SIGNAL_DONE_CMD string to let 
	 *         Doom3 call the method "signalCommandDone" when finished.
	 */
	RCF_METHOD_V1(void, executeConsoleCommand, const std::string&);
	
	/**
	 * This advises the engine to redirect the console output to this
	 * DarkRadiant RCF Server. The method "writeToConsole" is called
	 * to send the console text to the DarkRadiantRCFService.
	 * 
	 * @param int: Send the port number of the DarkRadiantRCFService (default: 50002)
	 */
	RCF_METHOD_V1(void, startConsoleBuffering, int);
	
	/**
	 * Ends redirecting the console to DarkRadiantRCFService.
	 * Note: If the SIGNAL_DONE_CMD is sent to the executeConsoleCommand
	 * method, this is done automatically by the engine and DarkRadiant
	 * can skip this call. 
	 */
	RCF_METHOD_V1(void, endConsoleBuffering, int);

RCF_END(D3DarkModInterface);

/**
 * greebo: This defines DarkRadiant's RCF Interface, for use by Doom 3
 */
RCF_BEGIN(DarkRadiantRCFService, "DarkRadiant")
	// Gets called by Doom3/DarkMod while ConsoleBuffering is active.
	RCF_METHOD_V1(void, writeToConsole, const std::string&);

	// Gets called by Doom3 when the SIGNAL_DONE_CMD is executed.
	RCF_METHOD_V0(void, signalCommandDone);
RCF_END(DarkRadiantRCFService);

#endif /* __RCF_SERVICE_DECL_H__ */
