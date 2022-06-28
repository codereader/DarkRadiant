#ifndef _STATEMENT_H_
#define _STATEMENT_H_

#include "itextstream.h"
#include "Executable.h"

namespace cmd {

class Statement :
	public Executable
{
	// The input string to execute
	std::string _string;

	// Whether this statement is a default one (won't be saved or deleted)
	bool _isReadOnly;

public:
	Statement(const std::string& str, bool isReadOnly = false) :
		_string(str),
		_isReadOnly(isReadOnly)
	{}

	void execute(const ArgumentList& args) override {
		// Execution means parsing another string
		GlobalCommandSystem().execute(_string);
	}

    bool canExecute() const override {
        return GlobalCommandSystem().canExecute(_string);
    }

	Signature getSignature() override {
		return Signature(); // signature is always empty
	}

	const std::string& getValue() const {
		return _string;
	}

	// Whether this statement is a default one (won't be saved or deleted)
	bool isReadonly() {
		return _isReadOnly;
	}
};
typedef std::shared_ptr<Statement> StatementPtr;

} // namespace cmd

#endif /* _STATEMENT_H_ */
