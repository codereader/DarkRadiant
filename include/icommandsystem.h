#pragma once

#include <memory>
#include <functional>
#include "math/Vector2.h"
#include "math/Vector3.h"

#include "imodule.h"

#include "string/convert.h"

namespace cmd
{

// Use these to define argument types
enum ArgumentTypeFlags
{
	ARGTYPE_VOID		= 0,
	ARGTYPE_STRING		= 1 << 0,
	ARGTYPE_INT			= 1 << 1,
	ARGTYPE_DOUBLE		= 1 << 2,
	ARGTYPE_VECTOR3		= 1 << 3,
	ARGTYPE_VECTOR2		= 1 << 4,
	// future types go here
	ARGTYPE_OPTIONAL	= 1 << 16,
};

/**
 * @brief A single command argument which may be of several different types.
 *
 * The argument maintains a list of type flags which indicate which types this argument may be
 * interpreted as. For example, an argument constructed from an integer may also be interpreted as
 * a double, so will have both ARGTYPE_INT and ARGTYPE_DOUBLE.
 */
class Argument
{
	std::string _strValue;
	double _doubleValue;
	int _intValue;
	Vector3 _vector3Value;
	Vector2 _vector2Value;

	// The type flags
	std::size_t _type;

public:
	Argument() :
		_doubleValue(0),
		_intValue(0),
		_vector3Value(0,0,0),
		_vector2Value(0,0),
		_type(ARGTYPE_VOID)
	{}

	// String => Argument constructor
	Argument(const std::string& str) :
		_strValue(str),
		_doubleValue(string::convert<double>(str)),
		_intValue(string::convert<int>(str)),
		_vector3Value(string::convert<Vector3>(str)),
		_vector2Value(Vector2(str)),
		_type(ARGTYPE_STRING)
	{
		tryNumberConversion();
		tryVectorConversion();
	}

	// Double => Argument constructor
	Argument(const double d) :
		_strValue(string::to_string(d)),
		_doubleValue(d),
		_intValue(static_cast<int>(d)),
		_vector3Value(d,d,d),
		_vector2Value(d,d),
		_type(ARGTYPE_DOUBLE)
	{
		// Enable INT flag if double value is rounded
		if (lrint(_doubleValue) == _intValue) {
			_type |= ARGTYPE_INT;
		}
	}

	// Int => Argument constructor
	Argument(const int i) :
		_strValue(string::to_string(i)),
		_doubleValue(static_cast<double>(i)),
		_intValue(i),
		_vector3Value(i,i,i),
		_vector2Value(i,i),
		_type(ARGTYPE_INT|ARGTYPE_DOUBLE) // INT can be used as DOUBLE too
	{}

	// Vector3 => Argument constructor
	Argument(const Vector3& v) :
		_strValue(string::to_string(v[0]) + " " + string::to_string(v[1]) + " " + string::to_string(v[2])),
		_doubleValue(v.getLength()),
		_intValue(static_cast<int>(v.getLength())),
		_vector3Value(v),
		_vector2Value(v[0], v[1]),
		_type(ARGTYPE_VECTOR3)
	{}

	// Vector2 => Argument constructor
	Argument(const Vector2& v) :
		_strValue(string::to_string(v[0]) + " " + string::to_string(v[1]) + " " + string::to_string(v[2])),
		_doubleValue(v.getLength()),
		_intValue(static_cast<int>(v.getLength())),
		_vector3Value(v[0], v[1], 0),
		_vector2Value(v),
		_type(ARGTYPE_VECTOR2)
	{}

	// Copy Constructor
	Argument(const Argument& other) :
		_strValue(other._strValue),
		_doubleValue(other._doubleValue),
		_intValue(other._intValue),
		_vector3Value(other._vector3Value),
		_vector2Value(other._vector2Value),
		_type(other._type)
	{}

	std::size_t getType() const
	{
		return _type;
	}

	std::string getString() const
	{
		return _strValue;
	}

	bool getBoolean() const
	{
		return getInt() != 0;
	}

	int getInt() const
	{
		return _intValue;
	}

	double getDouble() const
	{
		return _doubleValue;
	}

	Vector3 getVector3() const
	{
		return _vector3Value;
	}

	Vector2 getVector2() const
	{
		return _vector2Value;
	}

private:
	void tryNumberConversion()
	{
		// Try to cast the string value to numbers
		try {
			_intValue = std::stoi(_strValue);
			// cast succeeded
			_type |= ARGTYPE_INT;
		}
		catch (std::logic_error&) {}

		try
		{
			_doubleValue = std::stod(_strValue);
			// cast succeeded
			_type |= ARGTYPE_DOUBLE;
		}
		catch (std::invalid_argument&) {}
	}

	void tryVectorConversion()
	{
		// Use a stringstream to parse the string
        std::stringstream strm(_strValue);
        strm << std::skipws;

		// Try converting the first two values
        strm >> _vector2Value.x();
        strm >> _vector2Value.y();

		if (!strm.fail()) {
			_type |= ARGTYPE_VECTOR2;

			// Try to parse the third value
			strm >> _vector3Value.z();

			if (!strm.fail()) {
				// Third value successfully parsed
				_type |= ARGTYPE_VECTOR3;
				// Copy the two values from the parsed Vector2
				_vector3Value.x() = _vector2Value.x();
				_vector3Value.y() = _vector2Value.y();
			}
		}
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
typedef std::function<void (const ArgumentList&)> Function;

/// Convert a zero-argument function into a Function by discarding the ArgumentList
template <typename F> Function noArgs(F f)
{
    return [f](const ArgumentList&) { f(); };
}

/**
 * @brief Signature for a function which can test if a particular command should
 * be enabled.
 */
using CheckFunction = std::function<bool()>;

// A command signature consists just of arguments, return type is always void
typedef std::vector<std::size_t> Signature;

/**
 * greebo: Auto-completion information returned by the CommandSystem
 * when the user is entering a partial command.
 */
struct AutoCompletionInfo
{
	// The command prefix this info is referring to
	std::string prefix;

	// The candidaes, alphabetically ordered, case-insensitively
	typedef std::vector<std::string> Candidates;
	Candidates candidates;
};

/**
 * @brief Interface for the CommandSystem module.
 *
 * Commands are self-contained blocks of code (function calls or lambdas) which
 * can be invoked from menu items or from typing string commands in the
 * DarkRadiant console. They can also be called from Python.
 *
 * Commands can be invoked programmatically via the executeCommand() method,
 * which is sometimes useful if the implementing function isn't exposed via a
 * suitable module interface. Note however that neither the name of the command
 * (an arbitrary string) or the types of its arguments are checked at
 * compile-time, so calling C++ methods directly is generally preferable where
 * possible.
 */
class ICommandSystem: public RegisterableModule
{
public:

	/**
	 * Visit each command/bind using the given lambda. The functor is going to be called
	 * with the command name as argument.
	 */
	virtual void foreachCommand(const std::function<void(const std::string&)>& functor) = 0;

	/**
	 * greebo: Declares a new command with the given signature.
	 */
	virtual void addCommand(const std::string& name, Function func,
							const Signature& signature = Signature()) = 0;

    /**
     * @brief Add a new command with a check function which can test if the
     * command is currently runnable.
     *
     * This is aimed at commands which are not always available, e.g. because
     * they require one or more objects to be selected. If the command is not
     * currently available, the UI might choose to disable the button or menu
     * item which invokes it.
     *
     * @param name
     * Name of the command.
     *
     * @param func
     * Function to call when the command is invoked.
     *
     * @param check
     * Function to check whether the command should be enabled based on current
     * application state.
     */
    virtual void addWithCheck(const std::string& name, Function func, CheckFunction check,
                              const Signature& = {}) = 0;

    /// Returns true if the named command exists
    virtual bool commandExists(const std::string& name) = 0;

    /**
     * @brief Check if the named command is currently runnable.
     *
     * This is just a signal to the UI that a command should be disabled; the
     * command system does NOT guarantee that a command for which canExecute()
     * returns false won't actually be invoked by a subsequent call to
     * executeCommand().
     */
    virtual bool canExecute(const std::string& name) const = 0;

	/**
	 * Remove a named command.
	 */
	virtual void removeCommand(const std::string& name) = 0;

	/**
	 * greebo: Define a new statement, which consists of a name and a
	 * string to execute.
	 *
	 * Consider this as some sort of macro.
	 *
	 * @statementName: The name of the statement, e.g. "exportASE"
	 * @string: The string to execute.
	 * @saveStatementToRegistry: when TRUE (default) this statement/bind
	 * is saved to the registry at program shutdown. Pass FALSE if you
	 * don't want to let this statement persist between sessions.
	 */
	virtual void addStatement(const std::string& statementName,
							  const std::string& string,
							  bool saveStatementToRegistry = true) = 0;

	/**
	 * Visit each statement (bind) using the given lambda. The functor is going to be called
	 * with the statement name as argument.
	 */
	virtual void foreachStatement(const std::function<void(const std::string&)>& functor, bool customStatementsOnly = false) = 0;

	/**
	 * Returns the signature for the named command or bind. Statements
	 * always have an empty signature.
	 */
	virtual Signature getSignature(const std::string& name) = 0;

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

    /// Execute the named command with the given list of arguments
    virtual void executeCommand(const std::string& name, const ArgumentList& args = {}) = 0;

    /// Convenience method to execute a command with 1 argument
    void executeCommand(const std::string& name, const Argument& arg1)
    {
        executeCommand(name, ArgumentList{arg1});
    }

    /// Convenience method to execute a command with 2 arguments
    void executeCommand(const std::string& name, const Argument& arg1, const Argument& arg2)
    {
        executeCommand(name, {arg1, arg2});
    }

    /**
	 * greebo: Returns autocompletion info for the given prefix.
	 */
	virtual AutoCompletionInfo getAutoCompletionInfo(const std::string& prefix) = 0;
};
typedef std::shared_ptr<ICommandSystem> ICommandSystemPtr;

} // namespace cmd

const char* const MODULE_COMMANDSYSTEM("CommandSystem");

// This is the accessor for the commandsystem
inline cmd::ICommandSystem& GlobalCommandSystem()
{
	static module::InstanceReference<cmd::ICommandSystem> _reference(MODULE_COMMANDSYSTEM);
	return _reference;
}
