#pragma once

#include <wx/thread.h>
#include "ifilesystem.h"
#include "wxutil/VFSTreePopulator.h"
#include "PrefabSelector.h"

namespace ui
{

class PrefabPopulator :
	public wxutil::VFSTreePopulator::Visitor,
	public wxThread
{
private:
	const PrefabSelector::TreeColumns& _columns;

	// The tree store to populate
	wxutil::TreeModel::Ptr _treeStore;

	// The event handler to notify on completion
	wxEvtHandler* _finishedHandler;

	// The helper class, doing the tedious treeview insertion for us.
	wxutil::VFSTreePopulator _treePopulator;

	wxIcon _prefabIcon;
	wxIcon _folderIcon;

	std::string _prefabBasePath;

public:
	PrefabPopulator(const PrefabSelector::TreeColumns& columns, 
					wxEvtHandler* finishedHandler,
					const std::string& prefabBasePath);

	~PrefabPopulator(); // waits for thread to finish

    // Returns the prefab path as used in the constructor
    const std::string& getPrefabPath() const;

	void populate();

	void visit(wxutil::TreeModel& store, wxutil::TreeModel::Row& row,
		const std::string& path, bool isExplicit);

protected:
	// Thread entry point
	ExitCode Entry();

    void visitFile(const vfs::FileInfo& fileInfo);
};

}
