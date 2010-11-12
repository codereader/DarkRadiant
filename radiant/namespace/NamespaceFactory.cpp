#include "NamespaceFactory.h"
#include "itextstream.h"

#include "Namespace.h"
#include "modulesystem/StaticModule.h"

INamespacePtr NamespaceFactory::createNamespace() {
	return NamespacePtr(new Namespace);
}

// RegisterableModule implementation
const std::string& NamespaceFactory::getName() const {
	static std::string _name(MODULE_NAMESPACE_FACTORY);
	return _name;
}

const StringSet& NamespaceFactory::getDependencies() const {
	static StringSet _dependencies;
	// no dependencies
	return _dependencies;
}

void NamespaceFactory::initialiseModule(const ApplicationContext& ctx) {
	globalOutputStream() << getName() << "::initialiseModule called.\n";
}

// Define the static NamespaceFactoryModule
module::StaticModule<NamespaceFactory> namespaceFactoryModule;
