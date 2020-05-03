#include "ilayer.h"
#include "i18n.h"
#include "ieventmanager.h"
#include "icommandsystem.h"
#include "itextstream.h"
#include "imapinfofile.h"
#include "imap.h"

#include "module/StaticModule.h"
#include "LayerManager.h"
#include "LayerInfoFileModule.h"

#include "wxutil/dialog/Dialog.h"
#include "wxutil/dialog/MessageBox.h"
#include "wxutil/EntryAbortedException.h"

namespace scene
{

namespace
{

const char* const COMMAND_PREFIX_ADDTOLAYER("AddSelectionToLayer");
const char* const COMMAND_PREFIX_MOVETOLAYER("MoveSelectionToLayer");
const char* const COMMAND_PREFIX_SHOWLAYER("ShowLayer");
const char* const COMMAND_PREFIX_HIDELAYER("HideLayer");

inline void DoWithMapLayerManager(const std::function<void(scene::ILayerManager&)>& func)
{
	if (!GlobalMapModule().getRoot())
	{
		rError() << "No map loaded, cannot do this." << std::endl;
		return;
	}

	func(GlobalMapModule().getRoot()->getLayerManager());
}

}

class LayerModule :
	public ILayerModule
{
public:
	const std::string& getName() const override
	{
		static std::string _name(MODULE_LAYERS);
		return _name;
	}

	const StringSet& getDependencies() const override
	{
		static StringSet _dependencies;

		if (_dependencies.empty())
		{
			_dependencies.insert(MODULE_EVENTMANAGER);
			_dependencies.insert(MODULE_COMMANDSYSTEM);
			_dependencies.insert(MODULE_MAPINFOFILEMANAGER);
		}

		return _dependencies;
	}

	void initialiseModule(const ApplicationContext& ctx) override
	{
		rMessage() << getName() << "::initialiseModule called." << std::endl;

		GlobalCommandSystem().addCommand(COMMAND_PREFIX_ADDTOLAYER,
			std::bind(&LayerModule::addSelectionToLayer, this, std::placeholders::_1),
			{ cmd::ARGTYPE_INT });

		GlobalCommandSystem().addCommand(COMMAND_PREFIX_MOVETOLAYER,
			std::bind(&LayerModule::moveSelectionToLayer, this, std::placeholders::_1),
			{ cmd::ARGTYPE_INT });

		GlobalCommandSystem().addCommand(COMMAND_PREFIX_SHOWLAYER,
			std::bind(&LayerModule::showLayer, this, std::placeholders::_1),
			{ cmd::ARGTYPE_INT });

		GlobalCommandSystem().addCommand(COMMAND_PREFIX_HIDELAYER,
			std::bind(&LayerModule::hideLayer, this, std::placeholders::_1),
			{ cmd::ARGTYPE_INT });

		// Register the statements for layers 0..9
		// The statements are defined in commandsystem.xml, they just need to be
		// registered in the event manager module
		for (int i = 0; i < 10; i++)
		{
			GlobalEventManager().addCommand(
				COMMAND_PREFIX_ADDTOLAYER + string::to_string(i),
				COMMAND_PREFIX_ADDTOLAYER + string::to_string(i)
			);

			GlobalEventManager().addCommand(
				COMMAND_PREFIX_MOVETOLAYER + string::to_string(i),
				COMMAND_PREFIX_MOVETOLAYER + string::to_string(i)
			);

			GlobalEventManager().addCommand(
				COMMAND_PREFIX_SHOWLAYER + string::to_string(i),
				COMMAND_PREFIX_SHOWLAYER + string::to_string(i)
			);

			GlobalEventManager().addCommand(
				COMMAND_PREFIX_HIDELAYER + string::to_string(i),
				COMMAND_PREFIX_HIDELAYER + string::to_string(i)
			);
		}

		// Register the "create layer" command
		GlobalCommandSystem().addCommand("CreateNewLayer",
			std::bind(&LayerModule::createLayerCmd, this, std::placeholders::_1),
			{ cmd::ARGTYPE_STRING | cmd::ARGTYPE_OPTIONAL });
		GlobalEventManager().addCommand("CreateNewLayer", "CreateNewLayer");

		GlobalMapInfoFileManager().registerInfoFileModule(
			std::make_shared<LayerInfoFileModule>()
		);
	}

	ILayerManager::Ptr createLayerManager() override
	{
		return std::make_shared<LayerManager>();
	}

private:
	// Command target
	void createLayerCmd(const cmd::ArgumentList& args)
	{
		if (!GlobalMapModule().getRoot())
		{
			rError() << "No map loaded, cannot create a layer." << std::endl;
			return;
		}

		std::string initialName = !args.empty() ? args[0].getString() : "";

		while (true)
		{
			// Query the name of the new layer from the user
			std::string layerName;

			if (!initialName.empty()) 
			{
				// If we got a layer name passed through the arguments,
				// we use this one, but only the first time
				layerName = initialName;
				initialName.clear();
			}

			if (layerName.empty()) 
			{
				try 
				{
					layerName = wxutil::Dialog::TextEntryDialog(
						_("Enter Name"),
						_("Enter Layer Name"),
						"",
						GlobalMainFrame().getWxTopLevelWindow()
					);
				}
				catch (wxutil::EntryAbortedException&)
				{
					break;
				}
			}

			if (layerName.empty())
			{
				// Wrong name, let the user try again
				wxutil::Messagebox::ShowError(_("Cannot create layer with empty name."));
				continue;
			}

			// Attempt to create the layer, this will return -1 if the operation fails
			int layerID = GlobalMapModule().getRoot()->getLayerManager().createLayer(layerName);

			if (layerID != -1)
			{
				// Success, break the loop
				break;
			}
			else 
			{
				// Wrong name, let the user try again
				wxutil::Messagebox::ShowError(_("This name already exists."));
				continue;
			}
		}
	}

	void addSelectionToLayer(const cmd::ArgumentList& args)
	{
		if (args.size() != 1)
		{
			rError() << "Usage: " << COMMAND_PREFIX_ADDTOLAYER << " <LayerID> " << std::endl;
			return;
		}

		DoWithMapLayerManager([&](ILayerManager& manager)
		{
			manager.addSelectionToLayer(args[0].getInt());
		});
	}

	void moveSelectionToLayer(const cmd::ArgumentList& args)
	{
		if (args.size() != 1)
		{
			rError() << "Usage: " << COMMAND_PREFIX_MOVETOLAYER << " <LayerID> " << std::endl;
			return;
		}

		DoWithMapLayerManager([&](ILayerManager& manager)
		{
			manager.moveSelectionToLayer(args[0].getInt());
		});
	}

	void showLayer(const cmd::ArgumentList& args)
	{
		if (args.size() != 1)
		{
			rError() << "Usage: " << COMMAND_PREFIX_SHOWLAYER << " <LayerID> " << std::endl;
			return;
		}

		DoWithMapLayerManager([&](ILayerManager& manager)
		{
			manager.setLayerVisibility(args[0].getInt(), true);
		});
	}

	void hideLayer(const cmd::ArgumentList& args)
	{
		if (args.size() != 1)
		{
			rError() << "Usage: " << COMMAND_PREFIX_HIDELAYER << " <LayerID> " << std::endl;
			return;
		}

		DoWithMapLayerManager([&](ILayerManager& manager)
		{
			manager.setLayerVisibility(args[0].getInt(), false);
		});
	}
};

module::StaticModule<LayerModule> layerManagerFactoryModule;

}
