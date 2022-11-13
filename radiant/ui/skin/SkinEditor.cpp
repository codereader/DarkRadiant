#include "SkinEditor.h"

#include <wx/dataview.h>

#include "i18n.h"
#include "ui/modelselector/ModelTreeView.h"
#include "wxutil/dataview/ResourceTreeViewToolbar.h"

namespace ui
{

namespace
{
    constexpr const char* const DIALOG_TITLE = N_("Skin Editor");

    const std::string RKEY_ROOT = "user/ui/skinEditor/";
    const std::string RKEY_SPLIT_POS = RKEY_ROOT + "splitPos";
    const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
}

SkinEditor::SkinEditor() :
    DialogBase(DIALOG_TITLE)
{
    loadNamedPanel(this, "SkinEditorMainPanel");

    setupModelTreeView();

    // Set the default size of the window
    FitToScreen(0.8f, 0.9f);

    Layout();
    Fit();

    // Connect the window position tracker
    _windowPosition.loadFromPath(RKEY_WINDOW_STATE);
    _windowPosition.connect(this);
    _windowPosition.applyPosition();

    //_panedPosition.connect(splitter);
    //_panedPosition.loadFromPath(RKEY_SPLIT_POS);

    CenterOnParent();
}

SkinEditor::~SkinEditor()
{
    
}

void SkinEditor::setupModelTreeView()
{
    auto* panel = getControl<wxPanel>("SkinEditorModelTreeView");
    _modelTreeView = new ModelTreeView(panel);
    //_modelTreeView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &SkinEditor::_onModelSelectionChanged, this);

    auto* treeToolbar = new wxutil::ResourceTreeViewToolbar(panel, _modelTreeView);
    treeToolbar->EnableFavouriteManagement(false);

    auto definitionLabel = getControl<wxStaticText>("SkinEditorModelListLabel");
    definitionLabel->GetContainingSizer()->Detach(definitionLabel);
    definitionLabel->Reparent(treeToolbar);
    treeToolbar->GetLeftSizer()->Add(definitionLabel, 0, wxALIGN_LEFT);

    panel->GetSizer()->Add(treeToolbar, 0, wxEXPAND | wxBOTTOM, 6);
    panel->GetSizer()->Add(_modelTreeView, 1, wxEXPAND);
}

int SkinEditor::ShowModal()
{
    // Restore the position
    _windowPosition.applyPosition();

    _modelTreeView->Populate();

    int returnCode = DialogBase::ShowModal();

    // Tell the position tracker to save the information
    _windowPosition.saveToPath(RKEY_WINDOW_STATE);

    return returnCode;
}

void SkinEditor::ShowDialog(const cmd::ArgumentList& args)
{
    auto* editor = new SkinEditor;

    editor->ShowModal();
    editor->Destroy();
}

}
