#ifndef ECLASSTREE_H_
#define ECLASSTREE_H_

#include "iradiant.h"
#include "icommandsystem.h"
#include "gtkutil/window/BlockingTransientWindow.h"
#include <boost/shared_ptr.hpp>

#include <gtkmm/liststore.h>
#include <gtkmm/treestore.h>
#include <gtkmm/treeselection.h>

class EntityClassAttribute;

namespace ui
{

class EClassTree;
typedef boost::shared_ptr<EClassTree> EClassTreePtr;

struct EClassTreeColumns :
	public Gtk::TreeModel::ColumnRecord
{
	EClassTreeColumns() { add(name); add(icon); }

	Gtk::TreeModelColumn<Glib::ustring> name;	// name
	Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > icon;	// icon
};

class EClassTree :
	public gtkutil::BlockingTransientWindow
{
private:
	// The EClass treeview widget and underlying liststore
	EClassTreeColumns _eclassColumns;
	Gtk::TreeView* _eclassView;
	Glib::RefPtr<Gtk::TreeStore> _eclassStore;
	Glib::RefPtr<Gtk::TreeSelection> _eclassSelection;

	struct PropertyListColumns :
		public Gtk::TreeModel::ColumnRecord
	{
		PropertyListColumns()
		{
			add(name);
			add(value);
			add(colour);
			add(inherited);
		}

		Gtk::TreeModelColumn<Glib::ustring> name;
		Gtk::TreeModelColumn<Glib::ustring> value;
		Gtk::TreeModelColumn<Glib::ustring> colour;
		Gtk::TreeModelColumn<Glib::ustring> inherited;
	};

	// The treeview and liststore for the property pane
	PropertyListColumns _propertyColumns;
	Gtk::TreeView* _propertyView;
	Glib::RefPtr<Gtk::ListStore> _propertyStore;

	// Private constructor, traverses the entity classes
	EClassTree();

public:
	// Shows the singleton class (static command target)
	static void showWindow(const cmd::ArgumentList& args);

private:
	virtual void _preShow();

	// Constructs and adds all the dialog widgets
	void populateWindow();

	Gtk::Widget& createButtons(); 	// Dialog buttons
	Gtk::Widget& createEClassTreeView(); // EClass Tree
	Gtk::Widget& createPropertyTreeView(); // Property Tree

	// Loads the spawnargs into the right treeview
    void addToListStore(const EntityClassAttribute& attr);
	void updatePropertyView(const std::string& eclassName);

	// GTKmm callbacks
	void onClose();
	void onSelectionChanged();
};

} // namespace ui

#endif /*ECLASSTREE_H_*/

