#include "RadiantModule.h"
#include "RadiantThreadManager.h"

#include <iostream>

#include "ifiletypes.h"
#include "iregistry.h"
#include "icommandsystem.h"
#include "itextstream.h"
#include "ifilesystem.h"
#include "iuimanager.h"
#include "ieclass.h"
#include "ipreferencesystem.h"
#include "ieventmanager.h"
#include "iclipper.h"

#include "scene/Node.h"

#include "entity.h"
#include "map/AutoSaver.h"
#include "map/PointFile.h"
#include "ui/texturebrowser/TextureBrowser.h"
#include "ui/mediabrowser/MediaBrowser.h"
#include "ui/overlay/OverlayDialog.h"
#include "ui/prefdialog/PrefDialog.h"
#include "ui/splash/Splash.h"
#include "gtkutil/FileChooser.h"
#include "ui/mru/MRU.h"
#include "gtkutil/MultiMonitor.h"
#include "gtkutil/SourceView.h"

#include "modulesystem/StaticModule.h"

#include "mainframe_old.h"

namespace radiant
{

sigc::signal<void> RadiantModule::signal_radiantStarted() const
{
    return _radiantStarted;
}

sigc::signal<void> RadiantModule::signal_radiantShutdown() const
{
    return _radiantShutdown;
}

const ThreadManager& RadiantModule::getThreadManager() const
{
    if (!_threadManager)
    {
        _threadManager.reset(new RadiantThreadManager);
    }
    return *_threadManager;
}

void RadiantModule::broadcastShutdownEvent()
{
    _radiantShutdown.emit();
    _radiantShutdown.clear();
}

// Broadcasts a "startup" event to all the listeners
void RadiantModule::broadcastStartupEvent()
{
    _radiantStarted.emit();
}

// RegisterableModule implementation
const std::string& RadiantModule::getName() const
{
	static std::string _name(MODULE_RADIANT);
	return _name;
}

const StringSet& RadiantModule::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty())
    {
		_dependencies.insert(MODULE_COMMANDSYSTEM);
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_PREFERENCESYSTEM);
		_dependencies.insert(MODULE_EVENTMANAGER);
		_dependencies.insert(MODULE_SELECTIONSYSTEM);
		_dependencies.insert(MODULE_RENDERSYSTEM);
		_dependencies.insert(MODULE_CLIPPER);
	}

	return _dependencies;
}

void RadiantModule::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << "RadiantModule::initialiseModule called." << std::endl;

	// Reset the node id count
  	scene::Node::resetIds();

    map::PointFile::Instance().registerCommands();
    MainFrame_Construct();
	ui::MediaBrowser::registerPreferences();
	ui::TextureBrowser::construct();
	entity::registerCommands();
    map::AutoSaver().init();
}

void RadiantModule::shutdownModule()
{
	rMessage() << "RadiantModule::shutdownModule called." << std::endl;

	GlobalFileSystem().shutdown();

	map::PointFile::Instance().destroy();
	ui::OverlayDialog::destroy();
	ui::TextureBrowser::destroy();

    _radiantShutdown.clear();
}

void RadiantModule::postModuleInitialisation()
{
	ui::Splash::Instance().setProgressAndText(_("Creating Preference Dialog"), 0.85f);

	// Create the empty Settings node and set the title to empty.
	ui::PrefDialog::Instance().createOrFindPage(_("Game"));
	ui::PrefPagePtr settingsPage = ui::PrefDialog::Instance().createOrFindPage(_("Settings"));
	settingsPage->setTitle("");

	ui::Splash::Instance().setProgressAndText(_("Constructing Menu"), 0.89f);

	// Construct the MRU commands and menu structure, load the recently used files
	GlobalMRU().initialise();

	gtkutil::MultiMonitor::printMonitorInfo();

	// Add GtkSourceView styles to preferences
	ui::PrefPagePtr page = ui::PrefDialog::Instance().createOrFindPage(_("Settings/Source View"));

	std::list<std::string> schemeNames = gtkutil::SourceView::getAvailableStyleSchemeIds();
	page->appendCombo("Style Scheme", gtkutil::RKEY_SOURCEVIEW_STYLE, schemeNames, true);

	// Initialise the mediabrowser
    ui::Splash::Instance().setProgressAndText(_("Initialising MediaBrowser"), 0.92f);
    ui::MediaBrowser::init();
}

// Define the static Radiant module
module::StaticModule<RadiantModule> radiantCoreModule;

// Return the static Radiant module to other code within the main binary
RadiantModulePtr getGlobalRadiant()
{
	return radiantCoreModule.getModule();
}

} // namespace radiant

