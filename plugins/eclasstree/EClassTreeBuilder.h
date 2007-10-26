#ifndef ECLASSTREEBUILDER_H_
#define ECLASSTREEBUILDER_H_

#include "ieclass.h"
typedef struct _GtkTreeStore GtkTreeStore; 

/**
 * greebo: This traverses all the entity classes loaded so far and
 *         pushes them into the given tree store.
 */
class EClassTreeBuilder :
	public EntityClassVisitor
{
	GtkTreeStore* _treeStore;
	
public:
	EClassTreeBuilder(GtkTreeStore* targetStore);
	
	// Visitor implementation
	virtual void visit(IEntityClassPtr eclass);
};

#endif /*ECLASSTREEBUILDER_H_*/
