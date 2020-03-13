#include "ilayer.h"
#include "ieventmanager.h"
#include "icommandsystem.h"
#include "imapinfofile.h"

#include "modulesystem/StaticModule.h"
#include "LayerManager.h"
#include "LayerInfoFileModule.h"
#include "LayerCommandTarget.h"

#include "wxutil/dialog/Dialog.h"
#include "wxutil/dialog/MessageBox.h"
#include "wxutil/EntryAbortedException.h"

namespace scene
{

class LayerModule :
	public ILayerModule
{
private:
	typedef std::vector<LayerCommandTargetPtr> CommandTargetList;
	CommandTargetList _commandTargets;

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

#if 0 // TODO
		// Create the "master" layer with ID DEFAULT_LAYER
		createLayer(_(DEFAULT_LAYER_NAME));
#endif

		// Add command targets for the first 10 layer IDs here
		for (int i = 0; i < 10; i++)
		{
			_commandTargets.emplace_back(std::make_shared<LayerCommandTarget>(i));
		}

		// Register the "create layer" command
		GlobalCommandSystem().addCommand("CreateNewLayer",
			std::bind(&LayerModule::createLayerCmd, this, std::placeholders::_1),
			{ cmd::ARGTYPE_STRING | cmd::ARGTYPE_OPTIONAL });
		GlobalEventManager().addCommand("CreateNewLayer", "CreateNewLayer");

		GlobalMapModule().signal_mapEvent().connect(
			sigc::mem_fun(*this, &LayerModule::onMapEvent)
		);

		GlobalMapInfoFileManager().registerInfoFileModule(
			std::make_shared<LayerInfoFileModule>()
		);
	}

	ILayerManager::Ptr createLayerManager() override
	{
		return std::make_shared<LayerManager>();
	}

private:
	void onMapEvent(IMap::MapEvent ev)
	{
		// TODO
	}

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
};

module::StaticModule<LayerModule> layerManagerFactoryModule;

}
