#pragma once

#include "ui/ientityinspector.h"
#include "PropertyEditorFactory.h"

namespace ui
{

class EntityInspectorModule :
    public IEntityInspectorModule
{
private:
    std::unique_ptr<PropertyEditorFactory> _propertyEditorFactory;

public:
    PropertyEditorFactory& getPropertyEditorFactory();

    void registerPropertyEditor(const std::string& key, const IPropertyEditor::CreationFunc& creator) override;
    void unregisterPropertyEditor(const std::string& key) override;

    void registerPropertyEditorDialog(const std::string& key, const IPropertyEditorDialog::CreationFunc& create) override;
    IPropertyEditorDialog::Ptr createDialog(const std::string& key) override;
    void unregisterPropertyEditorDialog(const std::string& key) override;

    const std::string& getName() const override;
    const StringSet& getDependencies() const override;
    void initialiseModule(const IApplicationContext& ctx) override;
    void shutdownModule() override;

    static EntityInspectorModule& Instance();
};

} // namespace
