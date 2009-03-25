#ifndef ENTITYINSPECTOR_H_
#define ENTITYINSPECTOR_H_

#include "PropertyEditor.h"

#include "iradiant.h"
#include "icommandsystem.h"
#include "iselection.h"
#include "gtkutil/menu/PopupMenu.h"
#include "gtkutil/event/SingleIdleCallback.h"
#include "gtkutil/PanedPosition.h"

#include <gtk/gtkliststore.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtktogglebutton.h>
#include <gtk/gtktreeviewcolumn.h>
#include <map>

/* FORWARD DECLS */

class Entity;
class Selectable;

namespace ui {

namespace {

	// Data structure to store the type (vector3, text etc) and the options
	// string for a single property.
	struct PropertyParms {
		std::string type;
		std::string options;
	};

	// Map of property names to PropertyParms
	typedef std::map<std::string, PropertyParms> PropertyParmMap;

}

class EntityInspector;
typedef boost::shared_ptr<EntityInspector> EntityInspectorPtr;


/* The EntityInspector class represents the GTK dialog for editing properties
 * on the selected game entity. The class is implemented as a singleton and
 * contains a method to return the current instance.
 */
class EntityInspector :
 	public SelectionSystem::Observer,
 	public gtkutil::SingleIdleCallback,
	public RadiantEventListener
{
	// Currently selected entity, this pointer is only non-NULL if the
	// current entity selection includes exactly 1 entity.
	Entity* _selectedEntity;

	// Main EntityInspector widget
    GtkWidget* _widget;

	// Frame to contain the Property Editor
    GtkWidget* _editorFrame;

	// The checkbox for showing the eclass properties
	GtkWidget* _showInheritedCheckbox;
	GtkWidget* _showHelpColumnCheckbox;

    // Key list store and view
    GtkListStore* _listStore;
    GtkWidget* _treeView;

	GtkTreeViewColumn* _helpColumn;

	// Key and value edit boxes. These remain available even for multiple entity
    // selections.
	GtkWidget* _keyEntry;
	GtkWidget* _valEntry;

	// An object tracking the divider position of the paned view
	gtkutil::PanedPosition _panedPosition;

	// Context menu
	gtkutil::PopupMenu _contextMenu;

	// Currently displayed PropertyEditor
	PropertyEditorPtr _currentPropertyEditor;

    // Whether to show inherited properties or not
    bool _showInherited;

    // The last selected key
    std::string _lastKey;

	// The clipboard for spawnargs
	struct ClipBoard
	{
		std::string key;
		std::string value;

		bool empty() const {
			return key.empty();
		}
	} _clipBoard;

private:

    // Utility functions to construct the Gtk components

    GtkWidget* createDialogPane(); // bottom widget pane
    GtkWidget* createTreeViewPane(); // tree view for selecting attributes
    void createContextMenu();

	// Utility function to retrieve the string selection from the given column in the
	// list store
	std::string getListSelection(int col);

	/* gtkutil::PopupMenu callbacks */
	void _onAddKey();
	void _onDeleteKey();
	void _onCopyKey();
	void _onCutKey();
	void _onPasteKey();

	bool _testDeleteKey();
	bool _testCopyKey();
	bool _testCutKey();
	bool _testPasteKey();

    /* GTK CALLBACKS */
    static void callbackTreeSelectionChanged(GtkWidget* widget, EntityInspector* self);
	static void _onEntryActivate(GtkWidget*, EntityInspector*);
	static void _onSetProperty(GtkWidget*, EntityInspector*);
	static void _onToggleShowInherited(GtkToggleButton*, EntityInspector*);
	static void _onToggleShowHelpIcons(GtkToggleButton*, EntityInspector*);

	static gboolean _onQueryTooltip(GtkWidget* widget,
									 gint x, gint y, gboolean keyboard_mode,
									 GtkTooltip* tooltip, EntityInspector* self);

    static std::string  cleanInputString( const std::string& );

    // Routines to populate the TreeStore with the keyvals attached to the
    // currently-selected object.
    void refreshTreeModel();
    void appendClassProperties();

	// Update the GTK components when a new selection is made in the tree view
    void treeSelectionChanged();

    // Update our selected entity pointer from the selection system
    void updateSelectedEntity();

    // Set the keyval on all selected entities from the key and value textboxes
	void setPropertyFromEntries();

	// Applies the given key/value pair to the selection (works with multiple selected entities)
	void applyKeyValueToSelection(const std::string& key, const std::string& value);

	// Static map of property names to PropertyParms objects
	const PropertyParmMap& getPropertyMap();

protected:

	// GTK idle callback, used for refreshing display
	void onGtkIdle();

	// Constructor
    EntityInspector();

	// Holds the static shared_ptr
	static EntityInspectorPtr& getInstancePtr();

public:

    // Return or create the singleton instance
    static EntityInspector& getInstance();

    // Get the Gtk Widget for display in the main application
    GtkWidget* getWidget();

	// Callback used by the EntityCreator when a key value changes on an entity
    static void keyValueChanged();

	/** greebo: Gets called by the RadiantSelectionSystem upon selection change.
	 */
	void selectionChanged(const scene::INodePtr& node, bool isComponent);

	// RadiantEventListener implementation, gets called right before shutdown
	virtual void onRadiantShutdown();

	// greebo: Tells the inspector to reload the window settings from the registry.
	void restoreSettings();

	/**
	 * greebo: Static command target for toggling the Entity Inspector in the GroupDialog.
	 */
	static void toggle(const cmd::ArgumentList& args);
};

} // namespace ui

#endif /*ENTITYINSPECTOR_H_*/
