#include "ClassNameStore.h"

namespace ui {

ClassNameStore::ClassNameStore() :
	_store(Gtk::ListStore::create(_columns))
{
	populateListStore();

	// Sort the list alphabetically
	_store->set_sort_column(_columns.classname, Gtk::SORT_ASCENDING);
}

void ClassNameStore::destroy()
{
	InstancePtr().reset();
}

ClassNameStore& ClassNameStore::Instance()
{
	if (InstancePtr() == NULL)
	{
		InstancePtr().reset(new ClassNameStore);
	}

	return *InstancePtr();
}

ClassNameStorePtr& ClassNameStore::InstancePtr()
{
	static ClassNameStorePtr _instancePtr;
	return _instancePtr;
}

const ClassNameStore::ListStoreColumns& ClassNameStore::getColumns() const
{
	return _columns;
}

const Glib::RefPtr<Gtk::ListStore>& ClassNameStore::getModel() const
{
	return _store;
}

// EntityClassVisitor implementation
void ClassNameStore::visit(const IEntityClassPtr& eclass)
{
	Gtk::TreeModel::Row row = *_store->append();

	row[_columns.classname] = eclass->getName();
}

void ClassNameStore::populateListStore()
{
	_store->clear();

	// Visit each entity class using <this> as visitor
	GlobalEntityClassManager().forEachEntityClass(*this);
}

} // namespace ui
