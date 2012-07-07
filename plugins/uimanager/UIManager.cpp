#include "UIManager.h"

#include "itextstream.h"
#include "iregistry.h"
#include "iradiant.h"
#include "imainframe.h"
#include "icommandsystem.h"
#include "ieventmanager.h"
#include "colourscheme/ColourSchemeEditor.h"
#include "GroupDialog.h"
#include "debugging/debugging.h"
#include "FilterMenu.h"
#include "gtkutil/dialog/MessageBox.h"

#include <gtkmm/iconfactory.h>

namespace ui
{

IDialogManager& UIManager::getDialogManager()
{
	return *_dialogManager;
}

IMenuManager& UIManager::getMenuManager() {
	return _menuManager;
}

IToolbarManager& UIManager::getToolbarManager() {
	return _toolbarManager;
}

IColourSchemeManager& UIManager::getColourSchemeManager() {
	return ColourSchemeManager::Instance();
}

IGroupDialog& UIManager::getGroupDialog() {
	return GroupDialog::Instance();
}

IStatusBarManager& UIManager::getStatusBarManager() {
	return _statusBarManager;
}

void UIManager::addLocalBitmapsAsIconFactory()
{
    // Destination Gtk::IconFactory
    _iconFactory = Gtk::IconFactory::create();

    // Iterate over each file in the bitmaps dir
    std::string bitmapsPath = GlobalRegistry().get(RKEY_BITMAPS_PATH) + "/";

    Glib::Dir bitmapsDir(bitmapsPath);
    for (Glib::DirIterator i = bitmapsDir.begin();
         i != bitmapsDir.end();
         ++i)
    {
        Glib::ustring filename = *i;

		// Skip directories
		if (Glib::file_test(bitmapsPath + filename, Glib::FILE_TEST_IS_DIR))
		{
			continue;
		}

        // Load the pixbuf into an IconSet
		try
		{
			Gtk::IconSet is(
				Gdk::Pixbuf::create_from_file(bitmapsPath + filename)
			);

			// Add IconSet to Factory with "darkradiant:" stock prefix
			Glib::ustring filenameWithoutExtension = filename.substr(
				0, filename.rfind(".")
			);

			Gtk::StockID stockID(
				Glib::ustring::compose(
					"darkradiant:%1", filenameWithoutExtension
				)
			);

			_iconFactory->add(stockID, is);
		}
		catch (Gdk::PixbufError& ex)
		{
			rWarning() << "Could not load pixbuf from file: " <<
				filename << ": " << ex.what() << std::endl;
		}
		catch (Glib::FileError& ex)
		{
			rWarning() << "Could not load pixbuf from file: " <<
				filename << ": " << ex.what() << std::endl;
		}
    }

    // Add the IconFactory to the default factory list
    _iconFactory->add_default();
}

Glib::RefPtr<Gdk::Pixbuf> UIManager::getLocalPixbuf(const std::string& fileName)
{
	// Try to use a cached pixbuf first
	PixBufMap::iterator i = _localPixBufs.find(fileName);

	if (i != _localPixBufs.end())
	{
		return i->second;
	}

	// Not cached yet, load afresh

	// Construct the full filename using the Bitmaps path
	std::string fullFileName(GlobalRegistry().get(RKEY_BITMAPS_PATH) + fileName);

	Glib::RefPtr<Gdk::Pixbuf> pixbuf;

	try
	{
		pixbuf = Gdk::Pixbuf::create_from_file(fullFileName);

		if (!pixbuf)
		{
			rError() << "Couldn't load pixbuf " << fullFileName << std::endl;
		}
	}
	catch (Glib::FileError& err)
	{
		rWarning() << "Couldn't load pixbuf " << fullFileName << std::endl;
		rWarning() << err.what() << std::endl;
	}

	_localPixBufs.insert(PixBufMap::value_type(fileName, pixbuf));

	return pixbuf;
}

Glib::RefPtr<Gdk::Pixbuf> UIManager::getLocalPixbufWithMask(const std::string& fileName) {

	// Try to find a cached pixbuf before loading from disk
	PixBufMap::iterator i = _localPixBufsWithMask.find(fileName);

	if (i != _localPixBufsWithMask.end())
	{
		return i->second;
	}

	// Not cached yet, load afresh

	std::string fullFileName(GlobalRegistry().get(RKEY_BITMAPS_PATH) + fileName);

	Glib::RefPtr<Gdk::Pixbuf> rgba;

	try
	{
		Glib::RefPtr<Gdk::Pixbuf> rgb = Gdk::Pixbuf::create_from_file(fullFileName);

		if (rgb)
		{
			// File load successful, add alpha channel
			rgba = rgb->add_alpha(true, 255, 0, 255);
		}
		else
		{
			rError() << "Couldn't load rgb pixbuf " << fullFileName << std::endl;
		}
	}
	catch (Glib::FileError& err)
	{
		rWarning() << "Couldn't load rgb pixbuf " << fullFileName << std::endl;
		rWarning() << err.what() << std::endl;
	}

	_localPixBufsWithMask.insert(PixBufMap::value_type(fileName, rgba));

	return rgba;
}

Glib::RefPtr<Gtk::Builder>
UIManager::getGtkBuilderFromFile(const std::string& localFileName) const
{
    std::string fullPath = module::GlobalModuleRegistry()
                           .getApplicationContext()
                           .getRuntimeDataPath()
                           + "ui/"
                           + localFileName;

    // Attempt to load the Glade file into a Gtk::Builder
    Glib::RefPtr<Gtk::Builder> builder;
    try
    {
        builder = Gtk::Builder::create_from_file(
            fullPath
        );
    }
    catch (const Glib::FileError& e)
    {
        std::cerr << "[UIManager] Glib::FileError with code "
                  << e.code()
                  << " attempting to load Glade file '"
                  << fullPath << "'";
        throw;
    }
	catch (const Gtk::BuilderError& e)
	{
		std::cerr << "[UIManager] Gtk::BuilderError with code "
                  << e.code()
                  << " attempting to load Glade file '"
                  << fullPath << "'";

		gtkutil::MessageBox::ShowError("[UIManager] Gtk::BuilderError with code "
                  + string::to_string(e.code())
                  + " attempting to load Glade file '"
                  + fullPath + "'", GlobalMainFrame().getTopLevelWindow());

        throw;
	}

    // The builder was created successfully.
    std::cout << "[UIManager] Successfully loaded " << fullPath
              << std::endl;

    return builder;
}

IFilterMenuPtr UIManager::createFilterMenu()
{
	return IFilterMenuPtr(new FilterMenu);
}

void UIManager::clear()
{
	_menuManager.clear();
	_dialogManager = DialogManagerPtr();
}

const std::string& UIManager::getName() const {
	static std::string _name(MODULE_UIMANAGER);
	return _name;
}

const StringSet& UIManager::getDependencies() const
{
	static StringSet _dependencies;

	if (_dependencies.empty()) {
		_dependencies.insert(MODULE_EVENTMANAGER);
		_dependencies.insert(MODULE_XMLREGISTRY);
		_dependencies.insert(MODULE_RADIANT);
		_dependencies.insert(MODULE_COMMANDSYSTEM);
	}

	return _dependencies;
}

void UIManager::initialiseModule(const ApplicationContext& ctx)
{
	rMessage() << "UIManager::initialiseModule called" << std::endl;

	_dialogManager = DialogManagerPtr(new DialogManager);

	_menuManager.loadFromRegistry();
	_toolbarManager.initialise();
	ColourSchemeManager::Instance().loadColourSchemes();

	GlobalCommandSystem().addCommand("EditColourScheme", ColourSchemeEditor::editColourSchemes);
	GlobalEventManager().addCommand("EditColourScheme", "EditColourScheme");

	GlobalRadiant().signal_radiantShutdown().connect(
        sigc::mem_fun(this, &UIManager::clear)
    );

	// Add the statusbar command text item
	_statusBarManager.addTextElement(
		STATUSBAR_COMMAND,
		"",  // no icon
		IStatusBarManager::POS_COMMAND
	);

    addLocalBitmapsAsIconFactory();
}

void UIManager::shutdownModule()
{
	_localPixBufs.clear();
	_localPixBufsWithMask.clear();
}

} // namespace ui

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry)
{
	registry.registerModule(ui::UIManagerPtr(new ui::UIManager));

	// Initialise the streams using the given application context
	module::initialiseStreams(registry.getApplicationContext());

	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);

	// Set up the assertion handler
	GlobalErrorHandler() = registry.getApplicationContext().getErrorHandlingFunction();
}
