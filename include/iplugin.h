#ifndef IPLUGIN_H_
#define IPLUGIN_H_

#include "generic/constant.h"

/**
 * Abstract base class for optional plugins. Plugins will add their commands
 * to the UIManager themselves, so no actual interface is necessary, however
 * the module system works off types so a base class is needed to identify and
 * instantiate the plugins.
 */
class IPlugin
{
public:
	STRING_CONSTANT(Name, "IPlugin");
	INTEGER_CONSTANT(Version, 1);
};

/**
 * Modules typedefs.
 */
template<typename Type>
class Modules;
typedef Modules<IPlugin> PluginModules;

template<typename Type>
class ModulesRef;
typedef ModulesRef<IPlugin> PluginModulesRef;

#endif /*IPLUGIN_H_*/
