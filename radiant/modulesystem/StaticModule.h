#ifndef STATICMODULE_H_
#define STATICMODULE_H_

#include "imodule.h"

/** 
 * greebo: Use this class to define a static RegisterableModule.
 * 
 *         The template parameter must be a RegisterModule class and
 *         is automatically registered with the ModuleRegistry by 
 *         the StaticModule constructor.
 * 
 *         If immediate registering is not desired, the constructor
 *         could add the incoming modules to a static std::list
 *         and another static routine would add the modules on demand.
 * 
 * Usage: StaticModule<RegisterableModule> myStaticModule;
 */
namespace module {

template <class ModuleType>
class StaticModule
{
	// Define a boost::shared_ptr for the given class type
	typedef boost::shared_ptr<ModuleType> ModuleTypePtr;
	ModuleTypePtr _module;
	
public:
	// The constructor
	StaticModule() :
		_module(new ModuleType())
	{
		getRegistry().registerModule(_module);
	}
	
	inline ModuleTypePtr getModule() {
		return _module;
	}
};

} // namespace module

#endif /*STATICMODULE_H_*/
