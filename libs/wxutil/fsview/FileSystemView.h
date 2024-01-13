#pragma once

#include <sigc++/signal.h>
#include "../dataview/TreeModel.h"
#include "../dataview/TreeView.h"
#include "Populator.h"

namespace wxutil
{

/**
 * Specialised Tree View class sutiable for displaying a hierarchy of
 * files and folders, based on a given file extension filter.
 * The base path can be set to the root of DarkRadiant's VFS or on of its
 * subdirectories. It's also capable of displaying the contents of an
 * arbitrary archive file, located somewhere in the physical filesystem.
 * 
 * Use the static constructor to create an instance of this widget.
 */
class FileSystemView :
    public TreeView
{
private:
    TreeModel::Ptr _treeStore;

    std::string _basePath;
    std::string _fileIcon;

    // TRUE if the treeview has been populated
    bool _populated;
    std::unique_ptr<fsview::Populator> _populator;
    std::string _preselectPath;

    std::set<std::string> _fileExtensions;

    sigc::signal<void()> _signalTreePopulated;

public:
    class SelectionChangedEvent :
        public wxEvent
    {
    private:
        std::string _selectedPath;
        bool _isFolder;

    public:
        SelectionChangedEvent(int id = 0);
        SelectionChangedEvent(const std::string& selectedPath, bool isFolder, int id = 0);
        SelectionChangedEvent(const SelectionChangedEvent& other) = default;

        wxEvent* Clone() const override;

        const std::string& GetSelectedPath() const;
        bool SelectionIsFolder();
    };

    FileSystemView(wxWindow* parent, const TreeModel::Ptr& model, long style = wxDV_SINGLE);

    static const fsview::TreeColumns& Columns();

public:
    static FileSystemView* Create(wxWindow* parent, long style = wxDV_SINGLE);

    const std::string& GetBasePath() const;

    // Sets the base path of this view. This can be either an absolute filesystem path or a VFS path
    // It's also valid to pass an absolute path to a PAK file as base path,
    // which causes the view to open this file and show its contents.
    void SetBasePath(const std::string& basePath);

    // Define the set of file extensions to list in the tree (e.g. "map" or "pfbx")
    // Use a single "*" extension to list all files.
    void SetFileExtensions(const std::set<std::string>& fileExtensions);

    // Set the default icon used for files (e.g. "cmenu_add_prefab.png", relative to the bitmaps/ folder)
    void SetDefaultFileIcon(const std::string& fileIcon);

    // (Re-)populates the tree view, looking for files in the defined paths
    // If the preselectPath argument is not empty, the item will be selected after population
    void Populate(const std::string& preselectPath = std::string());

    // Selects and scrolls to the item defined by the given path
    void SelectPath(const std::string& path);

    std::string GetSelectedPath();
    bool GetIsFolderSelected();

    // Returns the path to the archive (directory/PK4) of the selected path
    std::string GetArchivePathOfSelection();

    // Expands the given path, which is relative to the base path (e.g. "maps/")
    void ExpandPath(const std::string& relativePath);

    sigc::signal<void()>& signal_TreePopulated();

private:
    TreeModel::Ptr CreateDefaultModel();

    void SelectItem(const wxDataViewItem& item);
    void HandleSelectionChange();
    void OnSelectionChanged(wxDataViewEvent& ev);
    void OnTreeStorePopulationFinished(TreeModel::PopulationFinishedEvent& ev);
};

wxDECLARE_EVENT(EV_FSVIEW_SELECTION_CHANGED, FileSystemView::SelectionChangedEvent);

}
