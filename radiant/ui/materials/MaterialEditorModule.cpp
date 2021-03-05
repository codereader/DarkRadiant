#include "MaterialEditor.h"

#include "i18n.h"
#include "imodule.h"
#include "icommandsystem.h"
#include "itextstream.h"
#include "imenumanager.h"
#include "module/StaticModule.h"

namespace ui
{

class MaterialEditorModule :
    public RegisterableModule
{
public:
    // RegisterableModule
    const std::string& getName() const override
    {
        static std::string _name("MaterialEditor");
        return _name;
    }

    const StringSet& getDependencies() const override
    {
        static StringSet _dependencies
        {
            MODULE_COMMANDSYSTEM,
            MODULE_MENUMANAGER,
        };

        return _dependencies;
    }

    void initialiseModule(const IApplicationContext& ctx) override
    {
        rMessage() << getName() << "::initialiseModule called." << std::endl;

        GlobalCommandSystem().addCommand("MaterialEditor", MaterialEditor::ShowDialog);

        // Bind the command to the menu
        GlobalMenuManager().add("main/entity", 	// menu location path
            "MaterialEditor", // name
            ui::menu::ItemType::Item,	// type
            _("Material Editor..."),	// caption
            "icon_texture.png",	// icon
            "MaterialEditor"); // event name
    }
};

module::StaticModule<MaterialEditorModule> materialEditorModule;

}
