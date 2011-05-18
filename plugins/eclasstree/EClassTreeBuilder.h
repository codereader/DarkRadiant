#ifndef ECLASSTREEBUILDER_H_
#define ECLASSTREEBUILDER_H_

#include "ieclass.h"
#include "gtkutil/VFSTreePopulator.h"

namespace ui
{

struct EClassTreeColumns;

/**
 * greebo: This traverses all the entity classes loaded so far and
 *         pushes them into the given tree store.
 */
class EClassTreeBuilder :
	public EntityClassVisitor,
	public gtkutil::VFSTreePopulator::Visitor
{
private:
	// The helper class, doing the tedious treeview insertion for us.
	gtkutil::VFSTreePopulator _treePopulator;

	const EClassTreeColumns& _columns;

	Glib::RefPtr<Gdk::Pixbuf> _entityIcon;

public:
	EClassTreeBuilder(const Glib::RefPtr<Gtk::TreeStore>& targetStore,
					  const EClassTreeColumns& columns);

	// Visitor implementation
	virtual void visit(const IEntityClassPtr& eclass);

	void visit(const Glib::RefPtr<Gtk::TreeStore>& store,
			   const Gtk::TreeModel::iterator& iter,
			   const std::string& path,
			   bool isExplicit);

private:
	// Returns an inheritance path, like this: "moveables/swords/"
	std::string getInheritancePathRecursive(const IEntityClassPtr& eclass);
};

} // namespace ui

#endif /*ECLASSTREEBUILDER_H_*/
