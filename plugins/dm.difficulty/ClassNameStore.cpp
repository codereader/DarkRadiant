#include "ClassNameStore.h"

#include <gtk/gtkliststore.h>

namespace ui {

ClassNameStore::ClassNameStore() :
	_store(gtk_list_store_new(NUM_COLUMNS, G_TYPE_STRING))
{
	populateListStore();
	g_object_ref(G_OBJECT(_store));
}

ClassNameStore::~ClassNameStore() {
	g_object_unref(G_OBJECT(_store));
}

// Returns the GtkTreeModel* data storage containing all the classnames
// Contains a singleton instance of this class
GtkTreeModel* ClassNameStore::getModel() {
	static ClassNameStore _classNameStore;
	return GTK_TREE_MODEL(_classNameStore._store);
}

// EntityClassVisitor implementation
void ClassNameStore::visit(IEntityClassPtr eclass) {
	GtkTreeIter iter;

	// Add a new row and append the data
	gtk_list_store_append(_store, &iter);
	gtk_list_store_set(_store, &iter, CLASSNAME_COL, eclass->getName().c_str(), -1);
}

void ClassNameStore::populateListStore() {
	gtk_list_store_clear(_store);

	// Visit each entity class using <this> as visitor
	GlobalEntityClassManager().forEach(*this);
}

} // namespace ui
