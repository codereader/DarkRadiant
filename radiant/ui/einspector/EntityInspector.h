#ifndef ENTITYINSPECTOR_H_
#define ENTITYINSPECTOR_H_

#include "PropertyEditor.h"

#include "ientityinspector.h"
#include "iradiant.h"
#include "icommandsystem.h"
#include "iselection.h"
#include "ientity.h"
#include "iundo.h"
#include "string/string.h"
#include "gtkutil/menu/PopupMenu.h"
#include "gtkutil/PanedPosition.h"

#include <gtkmm/liststore.h>

#include <map>
#include <boost/enable_shared_from_this.hpp>
#include <boost/algorithm/string/predicate.hpp>

/* FORWARD DECLS */
namespace Gtk
{
	class VBox;
	class Frame;
	class CheckButton;
	class TreeView;
	class Entry;
	class TreeViewColumn;
	class Paned;
}

class Entity;
class Selectable;
class EntityClassAttribute;

namespace ui
{

class EntityInspector;
typedef boost::shared_ptr<EntityInspector> EntityInspectorPtr;

/**
 * The EntityInspector class represents the GTK dialog for editing properties
 * on the selected game entity. The class is implemented as a singleton and
 * contains a method to return the current instance.
 */
class EntityInspector :
	public IEntityInspector,
 	public SelectionSystem::Observer,
    public Entity::Observer,
	public UndoSystem::Observer,
	public boost::enable_shared_from_this<EntityInspector>
{
public:
	struct ListStoreColumns :
		public Gtk::TreeModel::ColumnRecord
	{
		ListStoreColumns()
		{
			add(name);
			add(value);
			add(colour);
			add(icon);
			add(isInherited);
			add(helpIcon);
			add(hasHelpText);
		}

		Gtk::TreeModelColumn<Glib::ustring> name;
		Gtk::TreeModelColumn<Glib::ustring> value;
		Gtk::TreeModelColumn<Glib::ustring> colour;
		Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > icon;
		Gtk::TreeModelColumn<bool> isInherited;
		Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > helpIcon;
		Gtk::TreeModelColumn<bool> hasHelpText;
	};
	typedef boost::shared_ptr<ListStoreColumns> ListStoreColumnsPtr;

private:
	struct StringCompareFunctorNoCase :
		public std::binary_function<std::string, std::string, bool>
	{
		bool operator()(const std::string& s1, const std::string& s2) const
		{
			// return boost::algorithm::ilexicographical_compare(s1, s2); // slow!
			return string_compare_nocase(s1.c_str(), s2.c_str()) < 0;
		}
	};

	// Currently selected entity, this pointer is only non-NULL if the
	// current entity selection includes exactly 1 entity.
	Entity* _selectedEntity;

	// Main EntityInspector widget
	Gtk::VBox* _mainWidget;

	// Frame to contain the Property Editor
	Gtk::Frame* _editorFrame;

	// The checkbox for showing the eclass properties
	Gtk::CheckButton* _showInheritedCheckbox;
	Gtk::CheckButton* _showHelpColumnCheckbox;

	// A label showing the primitive number
	Gtk::Label* _primitiveNumLabel;

    // View and model for the keyvalue list
	ListStoreColumnsPtr _columns;
	Glib::RefPtr<Gtk::ListStore> _kvStore;
	Gtk::TreeView* _keyValueTreeView;

	Gtk::TreeViewColumn* _helpColumn;

    // Cache of Gtk::TreeModel::iterators pointing to keyvalue rows,
	// so we can quickly find existing keys to change their values
	typedef std::map<std::string, Gtk::TreeModel::iterator, StringCompareFunctorNoCase> TreeIterMap;
    TreeIterMap _keyValueIterMap;

	// Key and value edit boxes. These remain available even for multiple entity
    // selections.
	Gtk::Entry* _keyEntry;
	Gtk::Entry* _valEntry;

	// The pane dividing the treeview and the property editors
	Gtk::Paned* _paned;

	// An object tracking the divider position of the paned view
	gtkutil::PanedPosition _panedPosition;

	// Context menu
	gtkutil::PopupMenuPtr _contextMenu;

	// Currently displayed PropertyEditor
	IPropertyEditorPtr _currentPropertyEditor;

	// The clipboard for spawnargs
	struct ClipBoard
	{
		std::string key;
		std::string value;

		bool empty() const {
			return key.empty();
		}
	} _clipBoard;

	// Data structure to store the type (vector3, text etc) and the options
	// string for a single property.
	struct PropertyParms
	{
		std::string type;
		std::string options;
	};

	// Map of property names to PropertyParms, mapped like this: regex => parms
	typedef std::map<std::string, PropertyParms> PropertyParmMap;
	PropertyParmMap _propertyTypes;

private:

    // Utility functions to construct the Gtk components
	void construct();

	Gtk::Widget& createPropertyEditorPane(); // bottom widget pane
    Gtk::Widget& createTreeViewPane(); // tree view for selecting attributes
    void createContextMenu();

	// Utility function to retrieve the string selection from the given column in the
	// list store
	std::string getListSelection(const Gtk::TreeModelColumn<Glib::ustring>& col);
	bool getListSelection(const Gtk::TreeModelColumn<bool>& col);

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

    // gtkmm callbacks
	void _onEntryActivate();
	void _onSetProperty();
	void _onToggleShowInherited();
	void _onToggleShowHelpIcons();
	bool _onQueryTooltip(int x, int y, bool keyboard_tooltip, const Glib::RefPtr<Gtk::Tooltip>& tooltip);

    static std::string  cleanInputString( const std::string& );

    // Add and remove inherited properties from the entity class
    void addClassAttribute(const EntityClassAttribute& a);
    void addClassProperties();
    void removeClassProperties();

	// Update the GTK components when a new selection is made in the tree view
    void treeSelectionChanged();

    // Update our selected entity pointer from the selection system
    void getEntityFromSelectionSystem();

    // Change the selected entity pointer, setting up the observer
    void changeSelectedEntity(Entity* newEntity);

    // Set the keyval on all selected entities from the key and value textboxes
	void setPropertyFromEntries();

    // Applies the given key/value pair to the selection (works with multiple
    // selected entities)
    void applyKeyValueToSelection(const std::string& key,
                                  const std::string& value);

	// Initialise the property lookup tables
	void loadPropertyMap();

	// Returns property type and option for the given entity key
	PropertyParms getPropertyParmsForKey(const std::string& key);

    // Update tree view contents and property editor
    void updateGUIElements();

public:
	// Constructor
    EntityInspector();

	// Get the main widget for packing
	Gtk::Widget& getWidget();

	/** greebo: Gets called by the RadiantSelectionSystem upon selection change.
	 */
	void selectionChanged(const scene::INodePtr& node, bool isComponent);

	void registerPropertyEditor(const std::string& key, const IPropertyEditorPtr& editor);
	IPropertyEditorPtr getRegisteredPropertyEditor(const std::string& key);
	void unregisterPropertyEditor(const std::string& key);

	void onRadiantShutdown();

	// Gets called after an undo operation
	void postUndo();
	// Gets called after a redo operation
	void postRedo();

    /* Entity::Observer implementation */
    void onKeyInsert(const std::string& key, EntityKeyValue& value);
    void onKeyChange(const std::string& key, const std::string& value);
    void onKeyErase(const std::string& key, EntityKeyValue& value);

	// greebo: Tells the inspector to reload the window settings from the registry.
	void restoreSettings();

	/**
	 * greebo: Static command target for toggling the Entity Inspector in the GroupDialog.
	 */
	static void toggle(const cmd::ArgumentList& args);

	// RegisterableModule implementation
	virtual const std::string& getName() const;
	virtual const StringSet& getDependencies() const;
	virtual void initialiseModule(const ApplicationContext& ctx);
};

} // namespace ui

#endif /*ENTITYINSPECTOR_H_*/
