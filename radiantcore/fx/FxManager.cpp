#include "FxManager.h"

#include "decl/DeclarationCreator.h"
#include "module/StaticModule.h"
#include "FxDeclaration.h"

namespace fx
{

IFxDeclaration::Ptr FxManager::findFx(const std::string& name)
{
    return std::static_pointer_cast<IFxDeclaration>(
        GlobalDeclarationManager().findDeclaration(decl::Type::Fx, name));
}

const std::string& FxManager::getName() const
{
	static std::string _name("FxManager");
	return _name;
}

const StringSet& FxManager::getDependencies() const
{
    static StringSet _dependencies
    {
        MODULE_DECLMANAGER,
    };

	return _dependencies;
}

void FxManager::initialiseModule(const IApplicationContext& ctx)
{
    GlobalDeclarationManager().registerDeclType("fx", std::make_shared<decl::DeclarationCreator<FxDeclaration>>(decl::Type::Fx));
    GlobalDeclarationManager().registerDeclFolder(decl::Type::Fx, "fx/", ".fx");
}

// Static module instance
module::StaticModuleRegistration<FxManager> fxManagerModule;

}
