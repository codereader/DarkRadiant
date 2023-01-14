#include "EClassManager.h"

#include "ideclmanager.h"
#include "ieclasscolours.h"
#include "i18n.h"
#include "iregistry.h"
#include "icommandsystem.h"
#include "messages/ScopedLongRunningOperation.h"

#include "decl/DeclarationCreator.h"

#include "module/StaticModule.h"

namespace eclass
{

// Get a named entity class, creating if necessary
IEntityClassPtr EClassManager::findOrInsert(const std::string& name, bool has_brushes)
{
    return std::static_pointer_cast<IEntityClass>(
        GlobalDeclarationManager().findOrCreateDeclaration(decl::Type::EntityDef, name)
    );
}

IEntityClassPtr EClassManager::findClass(const std::string& className)
{
    return std::static_pointer_cast<IEntityClass>(
        GlobalDeclarationManager().findDeclaration(decl::Type::EntityDef, className)
    );
}

void EClassManager::forEachEntityClass(EntityClassVisitor& visitor)
{
    forEachEntityClass([&](const IEntityClassPtr& eclass)
    {
        visitor.visit(eclass);
    });
}

void EClassManager::forEachEntityClass(const std::function<void(const IEntityClassPtr&)>& functor)
{
    GlobalDeclarationManager().foreachDeclaration(decl::Type::EntityDef, [&](const decl::IDeclaration::Ptr& decl)
    {
        functor(std::static_pointer_cast<IEntityClass>(decl));
    });
}

IModelDef::Ptr EClassManager::findModel(const std::string& name)
{
    return std::static_pointer_cast<IModelDef>(
        GlobalDeclarationManager().findDeclaration(decl::Type::ModelDef, name)
    );
}

void EClassManager::forEachModelDef(const std::function<void(const IModelDef::Ptr&)>& functor)
{
    GlobalDeclarationManager().foreachDeclaration(decl::Type::ModelDef, [&](const decl::IDeclaration::Ptr& decl)
    {
        functor(std::static_pointer_cast<IModelDef>(decl));
    });
}

void EClassManager::reloadDefs()
{
    GlobalDeclarationManager().reloadDeclarations();
}

// RegisterableModule implementation
const std::string& EClassManager::getName() const
{
	static std::string _name(MODULE_ECLASSMANAGER);
	return _name;
}

const StringSet& EClassManager::getDependencies() const
{
    static StringSet _dependencies
    {
        MODULE_DECLMANAGER,
        MODULE_XMLREGISTRY,
        MODULE_COMMANDSYSTEM,
        MODULE_ECLASS_COLOUR_MANAGER,
    };

	return _dependencies;
}

void EClassManager::initialiseModule(const IApplicationContext& ctx)
{
    GlobalDeclarationManager().registerDeclType("entityDef", std::make_shared<decl::DeclarationCreator<EntityClass>>(decl::Type::EntityDef));
    GlobalDeclarationManager().registerDeclType("model", std::make_shared<decl::DeclarationCreator<Doom3ModelDef>>(decl::Type::ModelDef));
    GlobalDeclarationManager().registerDeclFolder(decl::Type::EntityDef, "def/", ".def");

	GlobalCommandSystem().addCommand("ReloadDefs", std::bind(&EClassManager::reloadDefsCmd, this, std::placeholders::_1));

    _eclassColoursChanged = GlobalEclassColourManager().sig_overrideColourChanged().connect(
        sigc::mem_fun(this, &EClassManager::onEclassOverrideColourChanged));
}

void EClassManager::shutdownModule()
{
	rMessage() << getName() << "::shutdownModule called." << std::endl;

    _eclassColoursChanged.disconnect();
}

void EClassManager::onEclassOverrideColourChanged(const std::string& eclass, bool overrideRemoved)
{
    // An override colour in the IColourManager instance has changed
    // Do we have an affected eclass with that name?
    auto foundEclass = std::static_pointer_cast<EntityClass>(findClass(eclass));

    if (!foundEclass)
    {
        return;
    }

    // If the override was removed, we just reset the colour
    // We perform this switch to avoid firing the eclass changed signal twice
    if (overrideRemoved)
    {
        foundEclass->resetColour();
    }
    else
    {
        GlobalEclassColourManager().applyColours(*foundEclass);
    }
}

// This takes care of relading the entityDefs and refreshing the scenegraph
void EClassManager::reloadDefsCmd(const cmd::ArgumentList& args)
{
	radiant::ScopedLongRunningOperation operation(_("Reloading Defs"));

    reloadDefs();
}

// Static module instance
module::StaticModuleRegistration<EClassManager> eclassModule;

} // namespace eclass
