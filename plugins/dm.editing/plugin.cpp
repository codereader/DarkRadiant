#include "imodule.h"

#include "i18n.h"
#include "ieventmanager.h"
#include "iradiant.h"
#include "iuimanager.h"
#include "iselection.h"
#include "ientityinspector.h"
#include "icommandsystem.h"
#include "itextstream.h"
#include "imainframe.h"
#include "debugging/debugging.h"

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
	virtual const std::string& getName() const {
		static std::string _name("DarkMod Editing");
		return _name;
	}

	virtual const StringSet& getDependencies() const {
		static StringSet _dependencies;

		if (_dependencies.empty())
		{
			_dependencies.insert(MODULE_ENTITYINSPECTOR);
			_dependencies.insert(MODULE_UIMANAGER);
			_dependencies.insert(MODULE_SELECTIONSYSTEM);
			_dependencies.insert(MODULE_COMMANDSYSTEM);
			_dependencies.insert(MODULE_MAINFRAME);
			_dependencies.insert(MODULE_RADIANT_APP);
		}

		return _dependencies;
	}

	virtual void initialiseModule(const IApplicationContext& ctx)
	{
		rMessage() << getName() << "::initialiseModule called." << std::endl;

		// Associated "def_head" with an empty property editor instance
		GlobalEntityInspector().registerPropertyEditor(
			ui::DEF_HEAD_KEY, ui::IPropertyEditorPtr(new ui::AIHeadPropertyEditor())
		);

		GlobalEntityInspector().registerPropertyEditor(
			ui::DEF_VOCAL_SET_KEY, ui::IPropertyEditorPtr(new ui::AIVocalSetPropertyEditor())
		);

		GlobalCommandSystem().addCommand("FixupMapDialog", ui::FixupMapDialog::RunDialog);

		GlobalUIManager().getMenuManager().add("main/map",
			"FixupMapDialog", ui::menuItem,
			_("Fixup Map..."), // caption
			"", // icon
			"FixupMapDialog"
		);

		GlobalCommandSystem().addCommand("MissionInfoEditDialog", ui::MissionInfoEditDialog::ShowDialog);

		GlobalUIManager().getMenuManager().add("main/map",
			"MissionInfoEditDialog", ui::menuItem,
			_("Edit Package Info (darkmod.txt)..."), // caption
			"sr_icon_readable.png", // icon
			"MissionInfoEditDialog"
		);

		GlobalMainFrame().signal_MainFrameConstructed().connect(
			sigc::ptr_fun(ui::AIEditingPanel::onMainFrameConstructed)
		);	
	}

	void shutdownModule()
	{
		ui::AIEditingPanel::Shutdown();

		// Remove associated property keys
		GlobalEntityInspector().unregisterPropertyEditor(ui::DEF_VOCAL_SET_KEY);
		GlobalEntityInspector().unregisterPropertyEditor(ui::DEF_HEAD_KEY);
	}
};
typedef std::shared_ptr<EditingModule> EditingModulePtr;

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry)
{
	module::performDefaultInitialisation(registry);

	registry.registerModule(std::make_shared<EditingModule>());
}
