#include "NamespaceFactory.h"
#include "itextstream.h"

#include "Namespace.h"
#include "module/StaticModule.h"

INamespacePtr NamespaceFactory::createNamespace()
{
	return std::make_shared<Namespace>();
}

// RegisterableModule implementation
const std::string& NamespaceFactory::getName() const
{
	static std::string _name(MODULE_NAMESPACE_FACTORY);
	return _name;
}

const StringSet& NamespaceFactory::getDependencies() const
{
	static StringSet _dependencies;
	// no dependencies
	return _dependencies;
}

void NamespaceFactory::initialiseModule(const IApplicationContext& ctx)
{
	rMessage() << getName() << "::initialiseModule called.\n";
}

// Define the static NamespaceFactoryModule
module::StaticModuleRegistration<NamespaceFactory> namespaceFactoryModule;
