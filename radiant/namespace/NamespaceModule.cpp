#include "NamespaceModule.h"

#include "modulesystem/singletonmodule.h"
#include "modulesystem/moduleregistry.h"

NamespaceAPI::NamespaceAPI() :
	_defaultNamespace(new Namespace())
{}

INamespace* NamespaceAPI::getTable() {
	// Return the pointer contained in the shared_ptr object
	return _defaultNamespace.get();
}

typedef SingletonModule<NamespaceAPI> NamespaceModule;
typedef Static<NamespaceModule> StaticNamespaceModule;
StaticRegisterModule staticRegisterDefaultNamespace(StaticNamespaceModule::instance());

Namespace& getDefaultNamespace() {
	// Retrieve the contained object from the static module
	INamespace* base = static_cast<INamespace*>(StaticNamespaceModule::instance().getTable());
	// Downcast it to Namespace 
	Namespace* ns = dynamic_cast<Namespace*>(base);
	// This must succeed
	assert(ns);
	return *ns;
}

Namespace& getClonedNamespace() {
	static Namespace _clonedNamespace;
	return _clonedNamespace;
}
