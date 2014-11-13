#include "PrefabPopulator.h"

#include "itextstream.h"
#include "iuimanager.h"
#include "ifiletypes.h"
#include "wxutil/TreeModel.h"

#include <wx/artprov.h>

namespace ui
{

namespace
{
	const char* const FOLDER_ICON = "folder.png";
	const char* const PREFAB_ICON = "cmenu_add_prefab.png";
}

PrefabPopulator::PrefabPopulator(const PrefabSelector::TreeColumns& columns,
	wxEvtHandler* finishedHandler, const std::string& prefabBasePath) :
	wxThread(wxTHREAD_JOINABLE),
	_columns(columns),
	_treeStore(new wxutil::TreeModel(_columns)),
	_finishedHandler(finishedHandler),
	_treePopulator(_treeStore),
	_prefabBasePath(prefabBasePath)
{
	_prefabIcon.CopyFromBitmap(
		wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + PREFAB_ICON));
	_folderIcon.CopyFromBitmap(
		wxArtProvider::GetBitmap(GlobalUIManager().ArtIdPrefix() + FOLDER_ICON));
}

PrefabPopulator::~PrefabPopulator()
{
	// We might have a running thread, wait for it
	if (IsRunning())
	{
		Delete();
	}
}

void PrefabPopulator::visit(const std::string& filename)
{
	if (TestDestroy())
	{
		return;
	}

	// Let the VFSTreePopulator do the insertion
	_treePopulator.addPath(filename);
}

wxThread::ExitCode PrefabPopulator::Entry()
{
    // Get the first extension from the list of possible patterns (e.g. *.pfb or *.map)
    FileTypePatterns patterns = GlobalFiletypes().getPatternsForType("prefab");

    std::string defaultExt = "";

    if (!patterns.empty())
    {
        defaultExt = patterns.begin()->extension; // ".pfb"
    }

	// Traverse the VFS
	GlobalFileSystem().forEachFile(_prefabBasePath, defaultExt, *this, 0);

	if (TestDestroy()) return static_cast<wxThread::ExitCode>(0);

	// Visit the tree populator in order to fill in the column data
	_treePopulator.forEachNode(*this);

	if (TestDestroy()) return static_cast<wxThread::ExitCode>(0);

	// Sort the model before returning it
	_treeStore->SortModelFoldersFirst(_columns.filename, _columns.isFolder);

	if (!TestDestroy())
	{
		// Send the event to our listener, only if we are not forced to finish
		wxQueueEvent(_finishedHandler, new wxutil::TreeModel::PopulationFinishedEvent(_treeStore));
	}

	return static_cast<wxThread::ExitCode>(0);
}

void PrefabPopulator::populate()
{
	if (IsRunning()) return;

	Run();
}

void PrefabPopulator::visit(wxutil::TreeModel& /* store */, wxutil::TreeModel::Row& row,
	const std::string& path, bool isExplicit)
{
	if (TestDestroy()) return;

	// Get the display path, everything after rightmost slash
	row[_columns.filename] = wxVariant(wxDataViewIconText(path.substr(path.rfind("/") + 1), 
		isExplicit ? _prefabIcon : _folderIcon));
	row[_columns.vfspath] = _prefabBasePath + path;
	row[_columns.isFolder] = !isExplicit;
}

}
