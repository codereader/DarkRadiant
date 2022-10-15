#include "imodule.h"

#include "i18n.h"
#include "ui/imenumanager.h"
#include "iselection.h"
#include "ui/ientityinspector.h"
#include "icommandsystem.h"
#include "ui/iuserinterface.h"
#include "ui/imainframe.h"

#include "AIEditingControl.h"
#include "AIHeadPropertyEditor.h"
#include "AIVocalSetPropertyEditor.h"
#include "FixupMapDialog.h"
#include "AIEditingPanel.h"
#include "MissionInfoEditDialog.h"

class EditingModule :
	public RegisterableModule
{
public:
	// RegisterableModule implementation
	const std::string& getName() const override
    {
		static std::string _name("DarkMod Editing");
		return _name;
	}

	const StringSet& getDependencies() const override
    {
        static StringSet _dependencies
        {
            MODULE_ENTITYINSPECTOR,
            MODULE_MENUMANAGER,
            MODULE_SELECTIONSYSTEM,
            MODULE_COMMANDSYSTEM,
            MODULE_MAINFRAME,
            MODULE_USERINTERFACE,
        };

		return _dependencies;
	}

	void initialiseModule(const IApplicationContext& ctx) override
	{
		// Associated "def_head" with an empty property editor instance
		GlobalEntityInspector().registerPropertyEditor(
			ui::DEF_HEAD_KEY, ui::AIHeadPropertyEditor::CreateNew);
        GlobalEntityInspector().registerPropertyEditorDialog(ui::DEF_HEAD_KEY,
            []() { return std::make_shared<ui::AIHeadEditorDialogWrapper>(); });

		GlobalEntityInspector().registerPropertyEditor(
			ui::DEF_VOCAL_SET_KEY, ui::AIVocalSetPropertyEditor::CreateNew);
        GlobalEntityInspector().registerPropertyEditorDialog(ui::DEF_VOCAL_SET_KEY,
            []() { return std::make_shared<ui::AIVocalSetEditorDialogWrapper>(); });

		GlobalCommandSystem().addCommand("FixupMapDialog", ui::FixupMapDialog::RunDialog);

		GlobalMenuManager().add("main/map",
			"FixupMapDialog", ui::menu::ItemType::Item,
			_("Fixup Map..."), // caption
			"", // icon
			"FixupMapDialog"
		);

		GlobalCommandSystem().addCommand("MissionInfoEditDialog", ui::MissionInfoEditDialog::ShowDialog);

		GlobalMenuManager().add("main/map",
			"MissionInfoEditDialog", ui::menu::ItemType::Item,
			_("Edit Package Info (darkmod.txt)..."), // caption
			"sr_icon_readable.png", // icon
			"MissionInfoEditDialog"
		);

		GlobalMainFrame().signal_MainFrameConstructed().connect([this] 
        {
            GlobalMainFrame().addControl(ui::AIEditingControl::Name, IMainFrame::ControlSettings
            {
                IMainFrame::Location::PropertyPanel,
                true
            });
        });

        GlobalUserInterface().registerControl(std::make_shared<ui::AIEditingControl>());
	}

	void shutdownModule() override
	{
        GlobalUserInterface().unregisterControl(ui::AIEditingControl::Name);

		// Remove associated property keys
		GlobalEntityInspector().unregisterPropertyEditor(ui::DEF_VOCAL_SET_KEY);
		GlobalEntityInspector().unregisterPropertyEditor(ui::DEF_HEAD_KEY);

        GlobalEntityInspector().unregisterPropertyEditorDialog(ui::DEF_HEAD_KEY);
        GlobalEntityInspector().unregisterPropertyEditorDialog(ui::DEF_VOCAL_SET_KEY);
	}
};

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry)
{
	module::performDefaultInitialisation(registry);

	registry.registerModule(std::make_shared<EditingModule>());
}
