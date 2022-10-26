#include "EntityInspectorModule.h"

#include "ideclmanager.h"
#include "iregistry.h"
#include "iselection.h"
#include "icommandsystem.h"
#include "imap.h"
#include "igame.h"
#include "ui/iuserinterface.h"

#include "EntityInspector.h"
#include "module/StaticModule.h"

namespace ui
{

class EntityInspectorControl :
    public IUserControl
{
public:
    std::string getControlName() override
    {
        return UserControl::EntityInspector;
    }

    std::string getDisplayName() override
    {
        return _("Entity");
    }

    std::string getIcon() override
    {
        return "cmenu_add_entity.png";
    }

    wxWindow* createWidget(wxWindow* parent) override
    {
        return new EntityInspector(parent);
    }
};

PropertyEditorFactory& EntityInspectorModule::getPropertyEditorFactory()
{
    return *_propertyEditorFactory;
}

const std::string& EntityInspectorModule::getName() const
{
    static std::string _name(MODULE_ENTITYINSPECTOR);
    return _name;
}

const StringSet& EntityInspectorModule::getDependencies() const
{
    static StringSet _dependencies
    {
        MODULE_DECLMANAGER,
        MODULE_XMLREGISTRY,
        MODULE_SELECTIONSYSTEM,
        MODULE_GAMEMANAGER,
        MODULE_COMMANDSYSTEM,
        MODULE_MAP,
        MODULE_USERINTERFACE,
    };

    return _dependencies;
}

void EntityInspectorModule::initialiseModule(const IApplicationContext& ctx)
{
    _propertyEditorFactory.reset(new PropertyEditorFactory);

    GlobalUserInterface().registerControl(std::make_shared<EntityInspectorControl>());
}

void EntityInspectorModule::shutdownModule()
{
    GlobalUserInterface().unregisterControl(UserControl::EntityInspector);
    _propertyEditorFactory.reset();
}

void EntityInspectorModule::registerPropertyEditor(const std::string& key, const IPropertyEditor::CreationFunc& creator)
{
    _propertyEditorFactory->registerPropertyEditor(key, creator);
}

void EntityInspectorModule::unregisterPropertyEditor(const std::string& key)
{
    _propertyEditorFactory->unregisterPropertyEditor(key);
}

void EntityInspectorModule::registerPropertyEditorDialog(const std::string& key, const IPropertyEditorDialog::CreationFunc& create)
{
    _propertyEditorFactory->registerPropertyEditorDialog(key, create);
}

IPropertyEditorDialog::Ptr EntityInspectorModule::createDialog(const std::string& key)
{
    return _propertyEditorFactory->createDialog(key);
}

void EntityInspectorModule::unregisterPropertyEditorDialog(const std::string& key)
{
    _propertyEditorFactory->unregisterPropertyEditorDialog(key);
}

EntityInspectorModule& EntityInspectorModule::Instance()
{
    return static_cast<EntityInspectorModule&>(GlobalEntityInspector());
}

module::StaticModuleRegistration<EntityInspectorModule> _entityInspectorModule;

}
