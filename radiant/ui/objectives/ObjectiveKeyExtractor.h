#ifndef OBJECTIVEKEYEXTRACTOR_H_
#define OBJECTIVEKEYEXTRACTOR_H_

namespace ui
{

/**
 * Entity Visitor which extracts objective keyvalues (of the form "obj<n>_blah")
 * and populates the given GtkTreeStore with appropriate information.
 */
class ObjectiveKeyExtractor
: public Entity::Visitor
{
	// Tree store to populate
	GtkTreeStore* _store;
	
public:

	/**
	 * Constructor. Set the tree store to populate.
	 */
	ObjectiveKeyExtractor(GtkTreeStore* store)
	: _store(store)
	{ }
	
	/**
	 * Required visit function.
	 */
	void visit(const std::string& key, const std::string& value) {
		GtkTreeIter iter;
		gtk_tree_store_append(_store, &iter, NULL);
		gtk_tree_store_set(_store, &iter, 0, key.c_str(), 1, value.c_str(), -1);
	}
};

}

#endif /*OBJECTIVEKEYEXTRACTOR_H_*/
