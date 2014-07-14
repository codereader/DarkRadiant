#pragma once

#include <wx/thread.h>
#include "ifilesystem.h"
#include "wxutil/VFSTreePopulator.h"
#include "PrefabSelector.h"

namespace ui
{

class PrefabPopulator :
	public wxutil::VFSTreePopulator::Visitor,
	public wxThread,
	public VirtualFileSystem::Visitor
{
private:
	const PrefabSelector::TreeColumns& _columns;

	// The tree store to populate
	wxutil::TreeModel* _treeStore;

	// The event handler to notify on completion
	wxEvtHandler* _finishedHandler;

	// The helper class, doing the tedious treeview insertion for us.
	wxutil::VFSTreePopulator _treePopulator;

	wxIcon _prefabIcon;
	wxIcon _folderIcon;

public:
	PrefabPopulator(const PrefabSelector::TreeColumns& columns, wxEvtHandler* finishedHandler);

	~PrefabPopulator(); // waits for thread to finish

	void populate();

	// FileSystem::Visitor implementation
	void visit(const std::string& filename);

	void visit(wxutil::TreeModel* store, wxutil::TreeModel::Row& row,
		const std::string& path, bool isExplicit);

protected:
	// Thread entry point
	ExitCode Entry();
};

}
