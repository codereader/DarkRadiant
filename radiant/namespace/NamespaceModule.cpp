#include "NamespaceModule.h"

#include "modulesystem/singletonmodule.h"
#include "modulesystem/moduleregistry.h"

NamespaceAPI::NamespaceAPI() :
	_namespace(new Namespace())
{}

INamespace* NamespaceAPI::getTable() {
	// Return the pointer contained in the shared_ptr object
	return _namespace.get();
}

typedef SingletonModule<NamespaceAPI> NamespaceModule;
typedef Static<NamespaceModule> StaticNamespaceModule;
StaticRegisterModule staticRegisterDefaultNamespace(StaticNamespaceModule::instance());
