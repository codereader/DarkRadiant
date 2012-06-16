#pragma once

#include "imodule.h"

#include "i18n.h"
#include "ieventmanager.h"
#include "iuimanager.h"
#include "icommandsystem.h"
#include "itextstream.h"

#include "ParticleEditor.h"

namespace ui
{

class ParticleEditorModule :
	public RegisterableModule
{
public:
	// RegisterableModule implementation
	virtual const std::string& getName() const
	{
		static std::string _name("ParticlesEditor");
		return _name;
	}

	virtual const StringSet& getDependencies() const {
		static StringSet _dependencies;

		if (_dependencies.empty())
		{
			_dependencies.insert(MODULE_EVENTMANAGER);
			_dependencies.insert(MODULE_UIMANAGER);
			_dependencies.insert(MODULE_COMMANDSYSTEM);
		}

		return _dependencies;
	}

	virtual void initialiseModule(const ApplicationContext& ctx)
	{
		rMessage() << getName() << "::initialiseModule called." << std::endl;

		// Add the callback event
		GlobalCommandSystem().addCommand(
			"ParticlesEditor",
			ParticleEditor::displayDialog
		);
		GlobalEventManager().addCommand("ParticlesEditor", "ParticlesEditor");

		// Add the menu item
		IMenuManager& mm = GlobalUIManager().getMenuManager();
		mm.add("main/entity",
				"ParticlesEditor",
				ui::menuItem,
				_("Particle Editor..."),
				"particle16.png",
				"ParticlesEditor");
	}
};
typedef boost::shared_ptr<ParticleEditorModule> ParticleEditorModulePtr;

} // namespace
