#pragma once

#include "ieclass.h"

#include <boost/shared_ptr.hpp>
#include <gtkmm/liststore.h>
#include <wx/arrstr.h>

namespace ui
{

class ClassNameStore;
typedef boost::shared_ptr<ClassNameStore> ClassNameStorePtr;

class ClassNameStore :
	private EntityClassVisitor
{
public:
	struct ListStoreColumns :
		public Gtk::TreeModel::ColumnRecord
	{
		ListStoreColumns() { add(classname); }

		Gtk::TreeModelColumn<Glib::ustring> classname;
	};

private:
	// The liststore containing the eclass info
	ListStoreColumns _columns;
	Glib::RefPtr<Gtk::ListStore> _store;

	wxArrayString _classNames;

public:
	// Constructor, traverses the eclasses and fills the GtkListStore
	ClassNameStore();

	const wxArrayString& getStringList() const;

	const ListStoreColumns& getColumns() const;

	// Returns the GtkTreeModel* data storage containing all the classnames
	// Contains a singleton instance of this class
	const Glib::RefPtr<Gtk::ListStore>& getModel() const;

	static ClassNameStore& Instance();

	static void destroy();

private:
	static ClassNameStorePtr& InstancePtr();

	// EntityClassVisitor implementation
	virtual void visit(const IEntityClassPtr& eclass);

	// Traverses all entities and fills the store
	void populateListStore();
};

} // namespace ui
