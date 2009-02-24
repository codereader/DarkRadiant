#ifndef _ICOMMANDSYSTEM_H_
#define _ICOMMANDSYSTEM_H_

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include "math/Vector2.h"
#include "math/Vector3.h"

#include "imodule.h"

#include "string/string.h"

namespace cmd {

enum ArgumentType
{
	ARGTYPE_STRING,
	ARGTYPE_INT,
	ARGTYPE_DOUBLE,
	ARGTYPE_VECTOR3,
	ARGTYPE_VECTOR2,
	NUM_ARGTYPES,
};

// One command argument, provides several getter methods
class Argument
{
	std::string _strValue;
	double _doubleValue;
	int _intValue;
	Vector3 _vector3Value;
	Vector2 _vector2Value;

public:
	Argument() :
		_doubleValue(0),
		_intValue(0),
		_vector3Value(0,0,0),
		_vector2Value(0,0)
	{}

	Argument(const char* str) :
		_strValue(str),
		_doubleValue(strToDouble(str)),
		_intValue(strToInt(str)),
		_vector3Value(Vector3(str)),
		_vector2Value(Vector2(str))
	{}

	// String => Argument constructor
	Argument(const std::string& str) :
		_strValue(str),
		_doubleValue(strToDouble(str)),
		_intValue(strToInt(str)),
		_vector3Value(Vector3(str)),
		_vector2Value(Vector2(str))
	{}

	// Double => Argument constructor
	Argument(const double d) :
		_strValue(doubleToStr(d)),
		_doubleValue(d),
		_intValue(static_cast<int>(d)),
		_vector3Value(d,d,d),
		_vector2Value(d,d)
	{}

	// Double => Argument constructor
	Argument(const int i) :
		_strValue(intToStr(i)),
		_doubleValue(static_cast<double>(i)),
		_intValue(i),
		_vector3Value(i,i,i),
		_vector2Value(i,i)
	{}

	// Vector3 => Argument constructor
	Argument(const Vector3& v) :
		_strValue(doubleToStr(v[0]) + " " + doubleToStr(v[1]) + " " + doubleToStr(v[2])),
		_doubleValue(v.getLength()),
		_intValue(static_cast<int>(v.getLength())),
		_vector3Value(v),
		_vector2Value(v[0], v[1])
	{}

	// Vector2 => Argument constructor
	Argument(const Vector2& v) :
		_strValue(doubleToStr(v[0]) + " " + doubleToStr(v[1]) + " " + doubleToStr(v[2])),
		_doubleValue(v.getLength()),
		_intValue(static_cast<int>(v.getLength())),
		_vector3Value(v[0], v[1], 0),
		_vector2Value(v)
	{}

	// Copy Constructor
	Argument(const Argument& other) :
		_strValue(other._strValue),
		_doubleValue(other._doubleValue),
		_intValue(other._intValue),
		_vector3Value(other._vector3Value),
		_vector2Value(other._vector2Value)
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

	Vector3 getVector3() const {
		return _vector3Value;
	}

	Vector2 getVector2() const {
		return _vector2Value;
	}
};

typedef std::vector<Argument> ArgumentList;

/**
 * greebo: A command target must take an ArgumentList argument, like this:
 *
 * void doSomething(const ArgumentList& args);
 *
 * This can be both a free function and a member function.
 */
typedef boost::function<void (const ArgumentList&)> Function;

// A command signature consists just of arguments, no return types
class Signature :
	public std::vector<ArgumentType> 
{
public:
	Signature()
	{}

	// Additional convenience constructors
	Signature(ArgumentType type1) {
		push_back(type1);
	}

	Signature(ArgumentType type1, ArgumentType type2) {
		push_back(type1);
		push_back(type2);
	}

	Signature(ArgumentType type1, ArgumentType type2, ArgumentType type3) {
		push_back(type1);
		push_back(type2);
		push_back(type3);
	}

	Signature(ArgumentType type1, ArgumentType type2, ArgumentType type3, ArgumentType type4) {
		push_back(type1);
		push_back(type2);
		push_back(type3);
		push_back(type4);
	}
};

class ICommandSystem :
	public RegisterableModule
{
public:
	/**
	 * greebo: Declares a new command with the given signature.
	 */
	virtual void addCommand(const std::string& name, Function func, 
							const Signature& signature = Signature()) = 0;

	/**
	 * greebo: Define a new statement, which consists of a name and a
	 * string to exeute.
	 *
	 * Consider this as some sort of macro.
	 */
	virtual void addStatement(const std::string& statementName, 
							   const std::string& string) = 0;

	/**
	 * greebo: Executes the given string as if the user had typed it
	 * in the command console. The passed string can be a sequence of
	 * statements separated by semicolon ';' characters. Each statement
	 * can have zero or more arguments, separated by spaces.
	 *
	 * It is possible to pass string arguments by using
	 * double- or single-quote characters.
	 * e.g. "This; string; will be; treated as a whole".
	 *
	 * The last command needs not to be delimited by a semicolon.
	 *
	 * Example: nudgeLeft; nudgeRight -1 0 0; write "Bla! Test"
	 */
	virtual void execute(const std::string& input) = 0;

	/**
	 * Execute the named command with the given arguments.
	 */
	virtual void executeCommand(const std::string& name) = 0;
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
