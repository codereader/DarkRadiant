#pragma once

#include <sigc++/connection.h>

#include "ieclass.h"
#include "icommandsystem.h"

#include "EntityClass.h"
#include "Doom3ModelDef.h"

namespace eclass
{

/**
 * \namespace eclass
 * Implementing classes for the Entity Class manager module.
 * \ingroup eclass
 */

/**
 * EClassManager - responsible for entityDef and modelDef declarations.
 */
class EClassManager final :
    public IEntityClassManager
{
private:
    sigc::connection _eclassColoursChanged;

public:
    // IEntityClassManager implementation
    IEntityClassPtr findOrInsert(const std::string& name, bool has_brushes) override;
    IEntityClassPtr findClass(const std::string& className) override;
    void forEachEntityClass(EntityClassVisitor& visitor) override;
    void forEachEntityClass(const std::function<void(const IEntityClassPtr&)>& functor) override;

    // Find the modeldef with the given name
    IModelDef::Ptr findModel(const std::string& name) override;
    void forEachModelDef(const std::function<void(const IModelDef::Ptr&)>& functor) override;

	// Reloads all entityDefs/modelDefs
    void reloadDefs() override;

    // RegisterableModule implementation
	const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const IApplicationContext& ctx) override;
    void shutdownModule() override;

private:
	void reloadDefsCmd(const cmd::ArgumentList& args);

    void onEclassOverrideColourChanged(const std::string& eclass, bool overrideRemoved);
};

} // namespace
