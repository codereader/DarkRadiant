#ifndef ECLASSTREEBUILDER_H_
#define ECLASSTREEBUILDER_H_

#include "ieclass.h"
#include "gtkutil/VFSTreePopulator.h"

namespace ui {

/**
 * greebo: This traverses all the entity classes loaded so far and
 *         pushes them into the given tree store.
 */
class EClassTreeBuilder :
	public EntityClassVisitor
{
	// The target treestore (FIXME: needed?)
	GtkTreeStore* _treeStore;
	
	// The helper class, doing the tedious treeview insertion for us.
	gtkutil::VFSTreePopulator _treePopulator;
	
public:
	EClassTreeBuilder(GtkTreeStore* targetStore);
	
	// Visitor implementation
	virtual void visit(IEntityClassPtr eclass);

private:
	// Returns an inheritance path, like this: "moveables/swords/"
	std::string getInheritancePathRecursive(const IEntityClassPtr& eclass);
};

} // namespace ui

#endif /*ECLASSTREEBUILDER_H_*/
