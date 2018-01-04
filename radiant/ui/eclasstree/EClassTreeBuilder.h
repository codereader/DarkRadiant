#pragma once

#include "ieclass.h"
#include "wxutil/VFSTreePopulator.h"
#include <wx/icon.h>
#include <wx/thread.h>

namespace ui
{

struct EClassTreeColumns;

/**
 * greebo: This traverses all the entity classes loaded so far and
 *         pushes them into the given tree store.
 */
class EClassTreeBuilder :
	public EntityClassVisitor,
	public wxutil::VFSTreePopulator::Visitor,
	public wxThread
{
private:
	const EClassTreeColumns& _columns;

	// The tree store to populate
	wxutil::TreeModel::Ptr _treeStore;

	// The event handler to notify on completion
	wxEvtHandler* _finishedHandler;

	// The helper class, doing the tedious treeview insertion for us.
	wxutil::VFSTreePopulator _treePopulator;

	wxIcon _entityIcon;

public:
	EClassTreeBuilder(const EClassTreeColumns& columns, wxEvtHandler* finishedHandler);

	~EClassTreeBuilder(); // waits for thread to finish

	void populate();

	// Visitor implementation
	virtual void visit(const IEntityClassPtr& eclass);

	void visit(wxutil::TreeModel& store, wxutil::TreeModel::Row& row,
			   const std::string& path, bool isExplicit);

protected:
	// Thread entry point
	ExitCode Entry();

private:
	// Returns an inheritance path, like this: "moveables/swords/"
	std::string getInheritancePathRecursive(const IEntityClassPtr& eclass);
};

} // namespace ui
