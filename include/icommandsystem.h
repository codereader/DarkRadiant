#ifndef _ICOMMANDSYSTEM_H_
#define _ICOMMANDSYSTEM_H_

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include "math/Vector3.h"

#include "imodule.h"

#include "string/string.h"

namespace cmd {

// One command argument, provides several getter methods
class Argument
{
	std::string _strValue;
	double _doubleValue;
	int _intValue;
	Vector3 _vectorValue;
	void* _pointerValue;

public:
	// String => Argument constructor
	Argument(const std::string& str) :
		_strValue(str),
		_doubleValue(strToDouble(str)),
		_intValue(strToInt(str)),
		_vectorValue(Vector3(str)),
		_pointerValue(NULL)
	{}

	// Double => Argument constructor
	Argument(const double d) :
		_strValue(doubleToStr(d)),
		_doubleValue(d),
		_intValue(static_cast<int>(d)),
		_vectorValue(0,0,0),
		_pointerValue(NULL)
	{}

	// Double => Argument constructor
	Argument(const int i) :
		_strValue(intToStr(i)),
		_doubleValue(static_cast<double>(i)),
		_intValue(i),
		_vectorValue(0,0,0),
		_pointerValue(NULL)
	{}

	// Vector3 => Argument constructor
	Argument(const Vector3& v) :
		_strValue(doubleToStr(v[0]) + " " + doubleToStr(v[1]) + " " + doubleToStr(v[2])),
		_doubleValue(v.getLength()),
		_intValue(static_cast<int>(v.getLength())),
		_vectorValue(v),
		_pointerValue(NULL)
	{}

	// Pointer => Argument constructor
	Argument(void* ptr) :
		_strValue(""),
		_doubleValue(0),
		_intValue(0),
		_vectorValue(0,0,0),
		_pointerValue(ptr)
	{}

	// Copy Constructor
	Argument(const Argument& other) :
		_strValue(other._strValue),
		_doubleValue(other._doubleValue),
		_intValue(other._intValue),
		_vectorValue(other._vectorValue),
		_pointerValue(other._pointerValue)
	{}

	std::string getString() const {
		return _strValue;
	}

	int getInt() const {
		return _intValue;
	}

	double getDouble() const {
		return _doubleValue;
	}

	Vector3 getVector() const {
		return _vectorValue;
	}

	void* getPointer() const {
		return _pointerValue;
	}
};

typedef std::vector<Argument> ArgumentList;

/**
 * greebo: A command target must take a ArgumentList argument, like this:
 *
 * void execute(const ArgumentList& args);
 *
 * This can be both a free function and a member function.
 */
typedef boost::function<void (const ArgumentList&)> CommandFunction;

enum ArgumentType
{
	ARGTYPE_STRING,
	ARGTYPE_INT,
	ARGTYPE_DOUBLE,
	ARGTYPE_VECTOR3,
	ARGTYPE_POINTER,
	NUM_ARGTYPES,
};

// A command signature consists just of arguments, no return types
typedef std::vector<ArgumentType> CommandSignature;

class ICommandSystem :
	public RegisterableModule
{
public:
	/**
	 * greebo: Declares a new command with the given signature.
	 */
	virtual void addCommand(const std::string& name, 
						    CommandFunction func, 
							const CommandSignature& signature = CommandSignature()) = 0;

	/**
	 * Execute the named command with the given arguments.
	 */
	virtual void executeCommand(const std::string& name, const Argument& arg1) = 0;
	virtual void executeCommand(const std::string& name, const Argument& arg1, const Argument& arg2) = 0;
	virtual void executeCommand(const std::string& name, const Argument& arg1, const Argument& arg2, const Argument& arg3) = 0;

	// For more than 3 arguments, use this method to pass a vector of arguments
	virtual void executeCommand(const std::string& name, 
								 const ArgumentList& args) = 0;
};
typedef boost::shared_ptr<ICommandSystem> ICommandSystemPtr;

} // namespace cmd

const std::string MODULE_COMMANDSYSTEM("CommandSystem");

// This is the accessor for the commandsystem
inline cmd::ICommandSystem& GlobalCommandSystem() {
	// Cache the reference locally
	static cmd::ICommandSystem& _cmdSystem(
		*boost::static_pointer_cast<cmd::ICommandSystem>(
			module::GlobalModuleRegistry().getModule(MODULE_COMMANDSYSTEM)
		)
	);
	return _cmdSystem;
}

#endif /* _ICOMMANDSYSTEM_H_ */
