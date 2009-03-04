#ifndef EM_STATEMENT_H_
#define EM_STATEMENT_H_

#include "icommandsystem.h"
#include "generic/callback.h"

#include "Command.h"

/**
 * greebo: A Statement is a special Command containing a string which is executed
 * by DarkRadiant's CommandSystem.
 */
class Statement :
	public Command
{
	// The statement to execute
	std::string _statement;
	
public:
	Statement(const std::string& statement, bool reactOnKeyUp = false) :
		Command(Callback(), reactOnKeyUp),
		_statement(statement)
	{}

	// Invoke the statement, overriding the base class method
	virtual void execute() {
		if (_enabled) {
			GlobalCommandSystem().execute(_statement);
		}
	}

}; // class Statement

#endif /* EM_STATEMENT_H_ */
