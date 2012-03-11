#ifndef INCLUDE_UIMANAGER_H_
#define INCLUDE_UIMANAGER_H_

#include "math/Vector3.h"
#include "imodule.h"

#include <gdkmm/pixbuf.h>
#include <gtkmm/builder.h>

// Forward declarations
namespace Gtk
{
	class Toolbar;
	class Widget;
}

class IColourSchemeManager {
public:
    virtual ~IColourSchemeManager() {}
	// greebo: Returns the named colour, returns <0,0,0> if not found
	virtual Vector3 getColour(const std::string& colourName) = 0;
};

namespace ui {
	/** greebo: The possible menu item types, one of these
	 * 			has to be passed when creating menu items.
	 */
	enum eMenuItemType {
		menuNothing,
		menuRoot,
		menuBar,
		menuFolder,
		menuItem,
		menuSeparator,
	};
} // namespace ui

/** greebo: Implementation documentation: see MenuManager.h.
 */
class IMenuManager
{
public:
    /** Destructor
	 */
    virtual ~IMenuManager() {}

	/** greebo: Retrieves the menuitem widget specified by the path.
	 *
	 * Example: get("main/file/open") delivers the widget for the "Open..." command.
	 *
	 * @returns: the widget, or NULL, if no the path hasn't been found.
	 */
	virtual Gtk::Widget* get(const std::string& path) = 0;

	/** greebo: Shows/hides the menuitem under the given path.
	 *
	 * @path: the path to the item (e.g. "main/view/cameraview")
	 * @visible: FALSE, if the widget should be hidden, TRUE otherwise
	 */
	virtual void setVisibility(const std::string& path, bool visible) = 0;

	/** greebo: Adds a new item as child under the given path.
	 *
	 * @insertPath: the path where to insert the item: "main/filters"
	 * @name: the name of the new item
	 * @type: the item type (usually menuFolder / menuItem)
	 * @caption: the display string of the menu item (incl. mnemonic)
	 * @icon: the icon filename (can be empty)
	 * @eventname: the event name (e.g. "ToggleShowSizeInfo")
	 */
	virtual Gtk::Widget* add(const std::string& insertPath,
						   const std::string& name,
						   ui::eMenuItemType type,
						   const std::string& caption,
						   const std::string& icon,
						   const std::string& eventName) = 0;

	/** greebo: Inserts a new menuItem as sibling _before_ the given insertPath.
	 *
	 * @insertPath: the path where to insert the item: "main/filters"
	 * @name: the name of the new menu item (no path, just the name)
	 * @caption: the display string including mnemonic
	 * @icon: the image file name relative to "bitmaps/", can be empty.
	 * @eventName: the event name this item is associated with (can be empty).
	 *
	 * @returns: the Gtk::Widget*
	 */
	virtual Gtk::Widget* insert(const std::string& insertPath,
							  const std::string& name,
							  ui::eMenuItemType type,
							  const std::string& caption,
							  const std::string& icon,
							  const std::string& eventName) = 0;

	/**
	 * Removes an entire path from the menus.
 	 */
	virtual void remove(const std::string& path) = 0;

	/** Recursively iterates over the menu items and updates
	 *  the accelerator strings by requesting them from the EventManager.
	 */
	virtual void updateAccelerators() = 0;
};

class IToolbarManager
{
public:
    virtual ~IToolbarManager() {}
	virtual Gtk::Toolbar* getToolbar(const std::string& toolbarName) = 0;
};

// The name of the command status bar item
#define STATUSBAR_COMMAND "Command"

class IStatusBarManager
{
public:

	// Use these positions to place the status bar elements in between
	// the default ones. A position of 31 would put a widget in between
	// POS_BRUSHCOUNT and POS_SHADERCLIPBOARD.
	enum StandardPositions {
		POS_FRONT			= 0,
		POS_COMMAND			= 10,
		POS_POSITION		= 20,
		POS_BRUSHCOUNT		= 30,
		POS_SHADERCLIPBOARD	= 40,
		POS_GRID			= 50,
		POS_BACK			= 9000,
	};

	/**
	 * Destructor
	 */
	virtual ~IStatusBarManager() {}

	/**
	 * Get the status bar widget, for packing into the main window.
	 */
	virtual Gtk::Widget* getStatusBar() = 0;

	/**
	 * greebo: This adds a named element to the status bar. Pass the widget
	 * which should be added and specify the position order.
	 *
	 * @name: the name of the element (can be used for later lookup).
	 * @widget: the widget to pack.
	 * @pos: the position to insert. Use POS_FRONT or POS_BACK to put the element
	 *       at the front or back of the status bar container.
	 */
	virtual void addElement(const std::string& name, Gtk::Widget* widget, int pos) = 0;

	/**
	 * greebo: A specialised method, adding a named text element.
	 * Use the setText() method to update this element.
	 *
	 * @name: the name for this element (can be used as key for the setText() method).
	 * @icon: the icon file to pack into the element, relative the BITMAPS_PATH. Leave empty
	 *        if no icon is desired.
	 * @pos: the position to insert. Use POS_FRONT or POS_BACK to put the element
	 *       at the front or back of the status bar container.
 	 */
	virtual void addTextElement(const std::string& name, const std::string& icon, int pos) = 0;

	/**
	 * Updates the content of the named text element. The name must refer to
	 * an element previously added by addTextElement().
	 */
	virtual void setText(const std::string& name, const std::string& text) = 0;

	/**
	 * Returns a named status bar widget, previously added by addElement().
	 *
	 * @returns: NULL if the named widget does not exist.
	 */
	virtual Gtk::Widget* getElement(const std::string& name) = 0;
};

// Forward declarations
class IGroupDialog;		// see igroupdialog.h for definition

namespace ui
{

class IDialogManager;	// see idialogmanager.h for definition
class IFilterMenu;		// see ifiltermenu.h for definition
typedef boost::shared_ptr<IFilterMenu> IFilterMenuPtr;

/// Convenience type for GtkBuilder refptr
typedef Glib::RefPtr<Gtk::Builder> GtkBuilderPtr;

} // namespace ui

const std::string MODULE_UIMANAGER("UIManager");

/** greebo: The UI Manager abstract base class.
 *
 * The UIManager provides an interface to add UI items like menu commands
 * toolbar icons, update status bar texts and such.
 */
class IUIManager :
	public RegisterableModule
{
public:
	virtual IMenuManager& getMenuManager() = 0;
	virtual IToolbarManager& getToolbarManager() = 0;
	virtual IColourSchemeManager& getColourSchemeManager() = 0;
	virtual IGroupDialog& getGroupDialog() = 0;
	virtual IStatusBarManager& getStatusBarManager() = 0;
	virtual ui::IDialogManager& getDialogManager() = 0;

	// Convenience functions to load a local image (from the bitmaps directory)
	// and return a Gdk::PixBuf reference for use by certain widgets (e.g. TreeView).
	virtual Glib::RefPtr<Gdk::Pixbuf> getLocalPixbuf(const std::string& fileName) = 0;
	virtual Glib::RefPtr<Gdk::Pixbuf> getLocalPixbufWithMask(const std::string& fileName) = 0;

    /**
     * \brief
     * Create a GtkBuilder object from a .glade UI file in the runtime ui/
     * directory.
     */
    virtual ui::GtkBuilderPtr getGtkBuilderFromFile(
                const std::string& localFileName
    ) const = 0;

	// Creates and returns a new top-level filter menu bar, see ifiltermenu.h
	virtual ui::IFilterMenuPtr createFilterMenu() = 0;
};

// This is the accessor for the UI manager
inline IUIManager& GlobalUIManager() {
	// Cache the reference locally
	static IUIManager& _uiManager(
		*boost::static_pointer_cast<IUIManager>(
			module::GlobalModuleRegistry().getModule(MODULE_UIMANAGER)
		)
	);
	return _uiManager;
}

// Shortcut accessors
inline IColourSchemeManager& ColourSchemes() {
	return GlobalUIManager().getColourSchemeManager();
}

inline IGroupDialog& GlobalGroupDialog() {
	return GlobalUIManager().getGroupDialog();
}

#endif /*INCLUDE_UIMANAGER_H_*/
