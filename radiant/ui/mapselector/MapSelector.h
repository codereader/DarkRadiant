#pragma once

#include "icommandsystem.h"

#include "wxutil/dialog/DialogBase.h"
#include "wxutil/fsview/FileSystemView.h"
#include "wxutil/WindowPosition.h"

class wxStdDialogButtonSizer;

namespace ui
{

/// Dialog for browsing and selecting a map from (V)FS
class MapSelector :
    public wxutil::DialogBase
{
private:
    wxPanel* _dialogPanel;
    wxStdDialogButtonSizer* _buttons;

    // Main tree view with the folder hierarchy
    wxutil::FileSystemView* _treeView;

    // The window position tracker
    wxutil::WindowPosition _position;

    bool _handlingSelectionChange;
    std::set<std::string> _mapFileExtensions;

private:
    // Private constructor, creates widgets
    MapSelector();

    // Helper functions to configure GUI components
    void setupTreeView(wxWindow* parent);

    // Populate the tree view with files
    void populateTree();

    // Get the path that should be used for map population
    // This reflects the settings made by the user on the top of the selector window
    std::string getBaseFolder();

    // Return the selected map path
    std::string getSelectedPath();

    void onRescanPath(wxCommandEvent& ev);
    void onSelectionChanged(wxutil::FileSystemView::SelectionChangedEvent& ev);
    void updateButtonSensitivity();

public:
    int ShowModal() override;

    /**
    * Display the Selector return the path of the file selected by the user.
    */
    static std::string ChooseMapFile();

    static void OpenMapFromProject(const cmd::ArgumentList& args);
};

}
