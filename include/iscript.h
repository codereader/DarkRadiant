#ifndef ISCRIPT_H_
#define ISCRIPT_H_

#include "imodule.h"

// Forward decl.
namespace boost { namespace python { class object; } }

namespace script
{

struct ExecutionResult
{
	// The output of the script
	std::string	output;

	// whether an error occurred
	bool	errorOccurred;
};
typedef boost::shared_ptr<ExecutionResult> ExecutionResultPtr;

} // namespace script

class IScriptInterface
{
public:
    virtual ~IScriptInterface() {}
	/**
	 * This method gets invoked by the Scripting System to let this class
	 * add its interface to the Python context.
	 */
	virtual void registerInterface(boost::python::object& nspace) = 0;
};
typedef boost::shared_ptr<IScriptInterface> IScriptInterfacePtr;

/**
 * DarkRadiant's Scripting System, based on boost::python. It's possible
 * to expose additional interfaces by using the addInterface() method.
 */
class IScriptingSystem :
	public RegisterableModule
{
public:
	/**
	 * greebo: Add a named interface to the scripting system. The interface object
	 * must provide a "registerInterface" method which will declare the names
	 * and objects to the given namespace.
	 */
	virtual void addInterface(const std::string& name, const IScriptInterfacePtr& iface) = 0;

	/**
	 * greebo: Executes the given python script file. The filename is specified relatively
	 * to the scripts/ folder.
	 */
	virtual void executeScriptFile(const std::string& filename) = 0;

	/**
	 * greebo: Interprets the given string as python script.
	 *
	 * @returns: the result object.
	 */
	virtual script::ExecutionResultPtr executeString(const std::string& scriptString) = 0;
};
typedef boost::shared_ptr<IScriptingSystem> IScriptingSystemPtr;

// String identifier for the script module
const std::string MODULE_SCRIPTING_SYSTEM("ScriptingSystem");

// This is the accessor for the scripting system
inline IScriptingSystem& GlobalScriptingSystem() {
	// Cache the reference locally
	static IScriptingSystem& _scriptingSystem(
		*boost::static_pointer_cast<IScriptingSystem>(
			module::GlobalModuleRegistry().getModule(MODULE_SCRIPTING_SYSTEM)
		)
	);
	return _scriptingSystem;
}

#endif /*ISCRIPT_H_*/
