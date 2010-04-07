#include "UIManager.h"

#include "itextstream.h"
#include "iregistry.h"
#include "iradiant.h"
#include "icommandsystem.h"
#include "ieventmanager.h"
#include "colourscheme/ColourSchemeEditor.h"
#include "GroupDialog.h"
#include "ShutdownListener.h"
#include "debugging/debugging.h"
#include "FilterMenu.h"

namespace ui {

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

GdkPixbuf* UIManager::getLocalPixbuf(const std::string& fileName) {
	// Try to use a cached pixbuf first
	PixBufMap::iterator i = _localPixBufs.find(fileName);
	
	if (i != _localPixBufs.end()) {
		return i->second;
	}

	// Not cached yet, load afresh

	// Construct the full filename using the Bitmaps path
	std::string fullFileName(GlobalRegistry().get(RKEY_BITMAPS_PATH) + fileName);

	GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file(fullFileName.c_str(), NULL);

	if (pixbuf != NULL) {
		_localPixBufs.insert(PixBufMap::value_type(fileName, pixbuf));
		
		// Avoid destruction of this pixbuf
		g_object_ref(pixbuf);
	}
	else {
		globalErrorStream() << "Couldn't load pixbuf " << fullFileName << std::endl; 
	}

	return pixbuf;
}

GdkPixbuf* UIManager::getLocalPixbufWithMask(const std::string& fileName) {

	// Try to find a cached pixbuf before loading from disk
	PixBufMap::iterator i = _localPixBufsWithMask.find(fileName);
	
	if (i != _localPixBufsWithMask.end()) {
		return i->second;
	}

	// Not cached yet, load afresh

	std::string fullFileName(GlobalRegistry().get(RKEY_BITMAPS_PATH) + fileName);
	
	GdkPixbuf* rgb = gdk_pixbuf_new_from_file(fullFileName.c_str(), 0);
	if (rgb != NULL) {
		// File load successful, add alpha channel
		GdkPixbuf* rgba = gdk_pixbuf_add_alpha(rgb, TRUE, 255, 0, 255);
		gdk_pixbuf_unref(rgb);

		_localPixBufsWithMask.insert(PixBufMap::value_type(fileName, rgba));

		// Avoid destruction of this pixbuf
		g_object_ref(rgba);

		return rgba;
	}
	else {
		// File load failed
		globalErrorStream() << "Couldn't load pixbuf " << fullFileName << std::endl; 
		return NULL;
	}
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
	globalOutputStream() << "UIManager::initialiseModule called" << std::endl;

	_dialogManager = DialogManagerPtr(new DialogManager);

	_menuManager.loadFromRegistry();
	_toolbarManager.initialise();
	ColourSchemeManager::Instance().loadColourSchemes();
	
	GlobalCommandSystem().addCommand("EditColourScheme", ColourSchemeEditor::editColourSchemes);
	GlobalEventManager().addCommand("EditColourScheme", "EditColourScheme");

	_shutdownListener = UIManagerShutdownListenerPtr(new UIManagerShutdownListener(*this));
	GlobalRadiant().addEventListener(_shutdownListener);

	// Add the statusbar command text item
	_statusBarManager.addTextElement(
		STATUSBAR_COMMAND, 
		"",  // no icon
		IStatusBarManager::POS_COMMAND
	);
}

void UIManager::shutdownModule()
{
	// Remove all remaining pixbufs
	for (PixBufMap::iterator i = _localPixBufs.begin(); i != _localPixBufs.end(); ++i)
	{
		if (GDK_IS_PIXBUF(i->second))
		{
			g_object_unref(i->second);
		}
	}

	for (PixBufMap::iterator i = _localPixBufsWithMask.begin(); i != _localPixBufsWithMask.end(); ++i)
	{
		if (GDK_IS_PIXBUF(i->second))
		{
			g_object_unref(i->second);
		}
	}
}

} // namespace ui

extern "C" void DARKRADIANT_DLLEXPORT RegisterModule(IModuleRegistry& registry) {
	registry.registerModule(ui::UIManagerPtr(new ui::UIManager));
	
	// Initialise the streams using the given application context
	module::initialiseStreams(registry.getApplicationContext());
	
	// Remember the reference to the ModuleRegistry
	module::RegistryReference::Instance().setRegistry(registry);

	// Set up the assertion handler
	GlobalErrorHandler() = registry.getApplicationContext().getErrorHandlingFunction();
}
