#include "FileSystemView.h"

#include "i18n.h"
#include "../Bitmap.h"
#include "../Icon.h"

namespace wxutil
{

namespace
{
    const char* const DEFAULT_FILE_ICON = "file.png";
}

wxDEFINE_EVENT(EV_FSVIEW_SELECTION_CHANGED, FileSystemView::SelectionChangedEvent);

FileSystemView::SelectionChangedEvent::SelectionChangedEvent(int id) :
    wxEvent(id, EV_FSVIEW_SELECTION_CHANGED)
{}

FileSystemView::SelectionChangedEvent::SelectionChangedEvent(const std::string& selectedPath, bool isFolder, int id) :
    wxEvent(id, EV_FSVIEW_SELECTION_CHANGED),
    _selectedPath(selectedPath),
    _isFolder(isFolder)
{}

wxEvent* FileSystemView::SelectionChangedEvent::Clone() const
{
    return new FileSystemView::SelectionChangedEvent(*this);
}

const std::string& FileSystemView::SelectionChangedEvent::GetSelectedPath() const
{
    return _selectedPath;
}

bool FileSystemView::SelectionChangedEvent::SelectionIsFolder()
{
    return _isFolder;
}

FileSystemView::FileSystemView(wxWindow* parent, const TreeModel::Ptr& model, long style) :
    TreeView(parent, model.get(), style),
    _treeStore(model),
    _fileIcon(DEFAULT_FILE_ICON)
{
    _fileExtensions.insert("*"); // list all files by default

    // Single visible column, containing the directory/shader name and the icon
    AppendIconTextColumn(_("File"), Columns().filename.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);
    AppendTextColumn(_("Location"), Columns().archiveDisplay.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_RIGHT, wxDATAVIEW_COL_SORTABLE);
    AppendTextColumn(_("Size"), Columns().size.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_RIGHT, wxDATAVIEW_COL_SORTABLE);

    // Get selection and connect the changed callback
    Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &FileSystemView::OnSelectionChanged, this);
    Bind(EV_TREEMODEL_POPULATION_FINISHED, &FileSystemView::OnTreeStorePopulationFinished, this);

    // Use the TreeModel's full string search function
    AddSearchColumn(Columns().filename);
}

const fsview::TreeColumns& FileSystemView::Columns()
{
    static fsview::TreeColumns _columns;
    return _columns;
}

FileSystemView* FileSystemView::Create(wxWindow* parent, long style)
{
    TreeModel::Ptr model(new TreeModel(Columns()));
    return new FileSystemView(parent, model, style);
}

const std::string& FileSystemView::GetBasePath() const
{
    return _basePath;
}

void FileSystemView::SetBasePath(const std::string& basePath)
{
    _basePath = basePath;
}

void FileSystemView::SetFileExtensions(const std::set<std::string>& fileExtensions)
{
    _fileExtensions = fileExtensions;
}

void FileSystemView::SetDefaultFileIcon(const std::string& fileIcon)
{
    _fileIcon = fileIcon;
}

void FileSystemView::Populate(const std::string& preselectPath)
{
    _populated = true;
    _preselectPath = preselectPath;

    if (_populator && _populator->GetBasePath() == GetBasePath())
    {
        // Population already running for this path
        return;
    }

    // Clear the existing run first (this waits for it to finish)
    _populator.reset();

    // Clear the treestore
    _treeStore->Clear();

    TreeModel::Row row = _treeStore->AddItem();

    wxutil::Icon loadingIcon(GetLocalBitmap(_fileIcon));

    row[Columns().filename] = wxVariant(wxDataViewIconText(_("Loading..."), loadingIcon));
    row[Columns().isFolder] = false;
    row[Columns().vfspath] = "__loadingnode__"; // to prevent that item from being found
    row.SendItemAdded();

    _populator.reset(new fsview::Populator(Columns(), this, GetBasePath(), _fileExtensions));
    _populator->SetDefaultFileIcon(_fileIcon);

    // Start the thread, will send an event when finished
    _populator->Populate();
}

void FileSystemView::SelectPath(const std::string& path)
{
    // #4490: Only preselect if path is not empty, wxGTK will buffer that
    // and call ExpandAncestors() on a stale wxDataViewItem in its internal idle routine
    if (path.empty()) return;

    // Find and select the item
    SelectItem(_treeStore->FindString(path, Columns().vfspath));
}

void FileSystemView::ExpandPath(const std::string& relativePath)
{
    if (relativePath.empty()) return;

    Expand(_treeStore->FindString(relativePath, Columns().vfspath));
}

std::string FileSystemView::GetSelectedPath()
{
    wxDataViewItem item = GetSelection();

    if (!item.IsOk()) return "";

    wxutil::TreeModel::Row row(item, *GetModel());

    return row[Columns().vfspath];
}

bool FileSystemView::GetIsFolderSelected()
{
    wxDataViewItem item = GetSelection();

    if (!item.IsOk()) return false;

    wxutil::TreeModel::Row row(item, *GetModel());

    return row[Columns().isFolder].getBool();
}

std::string FileSystemView::GetArchivePathOfSelection()
{
    wxDataViewItem item = GetSelection();

    if (!item.IsOk()) return "";

    wxutil::TreeModel::Row row(item, *GetModel());

    return row[Columns().archivePath];
}

void FileSystemView::SelectItem(const wxDataViewItem& item)
{
    if (!item.IsOk()) return;
     
    Select(item);
    EnsureVisible(item);
    HandleSelectionChange();
}

TreeModel::Ptr FileSystemView::CreateDefaultModel()
{
    _treeStore.reset(new TreeModel(Columns()));
    return _treeStore;
}

void FileSystemView::HandleSelectionChange()
{
    auto selectedPath = GetSelectedPath();

    SelectionChangedEvent event(GetSelectedPath(), GetIsFolderSelected(), this->GetId());
    event.SetEventObject(this);
    
    HandleWindowEvent(event);
}

void FileSystemView::OnSelectionChanged(wxDataViewEvent& ev)
{
    HandleSelectionChange();
}

void FileSystemView::OnTreeStorePopulationFinished(TreeModel::PopulationFinishedEvent& ev)
{
    _treeStore = ev.GetTreeModel();

    wxDataViewItem preselectItem;

    if (!_preselectPath.empty())
    {
        // Find and select the classname
        preselectItem = _treeStore->FindString(_preselectPath, Columns().vfspath);
    }

    AssociateModel(_treeStore.get());

    if (preselectItem.IsOk())
    {
        SelectItem(preselectItem);
    }

    _populator.reset();

    // Auto-size the first level
    TriggerColumnSizeEvent();

    // Call client code
    _signalTreePopulated.emit();
}

sigc::signal<void>& FileSystemView::signal_TreePopulated()
{
    return _signalTreePopulated;
}

}
