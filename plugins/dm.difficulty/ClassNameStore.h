#ifndef __CLASSNAME_STORE_H_
#define __CLASSNAME_STORE_H_

#include "ieclass.h"

typedef struct _GtkListStore GtkListStore;
typedef struct _GtkTreeModel GtkTreeModel;

namespace ui {

class ClassNameStore :
	private EntityClassVisitor
{
	// The liststore containing the eclass info
	GtkListStore* _store;

public:
	enum {
		CLASSNAME_COL, // classname
		NUM_COLUMNS,
	};

	// Constructor, traverses the eclasses and fills the GtkListStore
	ClassNameStore();

	~ClassNameStore();

	// Returns the GtkTreeModel* data storage containing all the classnames
	// Contains a singleton instance of this class
	static GtkTreeModel* getModel();

private:
	// EntityClassVisitor implementation
	virtual void visit(IEntityClassPtr eclass);

	// Traverses all entities and fills the store
	void populateListStore();
};

} // namespace ui

#endif /* __CLASSNAME_STORE_H_ */
