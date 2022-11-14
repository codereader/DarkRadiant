#include "SkinEditor.h"

#include "i18n.h"

#include <wx/dataview.h>
#include <wx/splitter.h>
#include <wx/textctrl.h>
#include <wx/button.h>

#include "ui/modelselector/ModelTreeView.h"
#include "wxutil/dataview/ResourceTreeViewToolbar.h"
#include "wxutil/dataview/ThreadedDeclarationTreePopulator.h"
#include "wxutil/sourceview/DeclarationSourceView.h"
#include "wxutil/sourceview/SourceView.h"

namespace ui
{

namespace
{
    constexpr const char* const DIALOG_TITLE = N_("Skin Editor");
    constexpr const char* const SKIN_ICON = "icon_skin.png";

    const std::string RKEY_ROOT = "user/ui/skinEditor/";
    const std::string RKEY_SPLIT_POS_LEFT = RKEY_ROOT + "splitPosLeft";
    const std::string RKEY_SPLIT_POS_RIGHT = RKEY_ROOT + "splitPosRight";
    const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
}

SkinEditor::SkinEditor() :
    DialogBase(DIALOG_TITLE),
    _selectedModels(new wxutil::TreeModel(_selectedModelColumns, true)),
    _remappings(new wxutil::TreeModel(_remappingColumns, true))
{
    loadNamedPanel(this, "SkinEditorMainPanel");

    makeLabelBold(this, "SkinEditorSkinDefinitionsLabel");
    makeLabelBold(this, "SkinEditorEditSkinDefinitionLabel");
    makeLabelBold(this, "SkinEditorDeclarationSourceLabel");

    setupModelTreeView();
    setupSkinTreeView();
    setupSelectedModelList();
    setupRemappingPanel();
    setupPreview();

    getControl<wxButton>("SkinEditorCloseButton")->Bind(wxEVT_BUTTON, &SkinEditor::onCloseButton, this);

    // Set the default size of the window
    FitToScreen(0.9f, 0.9f);

    Layout();
    Fit();

    // Connect the window position tracker
    _windowPosition.loadFromPath(RKEY_WINDOW_STATE);
    _windowPosition.connect(this);
    _windowPosition.applyPosition();

    auto leftSplitter = getControl<wxSplitterWindow>("SkinEditorLeftSplitter");
    _leftPanePosition.connect(leftSplitter);
    _leftPanePosition.loadFromPath(RKEY_SPLIT_POS_LEFT);

    auto rightSplitter = getControl<wxSplitterWindow>("SkinEditorRightSplitter");
    _rightPanePosition.connect(rightSplitter);
    _rightPanePosition.loadFromPath(RKEY_SPLIT_POS_RIGHT);

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

    auto definitionLabel = getControl<wxStaticText>("SkinEditorModelListLabel");

    auto* treeToolbar = new wxutil::ResourceTreeViewToolbar(definitionLabel->GetParent(), _modelTreeView);
    treeToolbar->EnableFavouriteManagement(false);
    treeToolbar->SetName("ModelTreeViewToolbar");

    auto labelSizer = definitionLabel->GetContainingSizer();
    labelSizer->Replace(definitionLabel, treeToolbar);

    definitionLabel->Reparent(treeToolbar);
    treeToolbar->GetLeftSizer()->Add(definitionLabel, 0, wxALIGN_LEFT);

    panel->GetSizer()->Add(_modelTreeView, 1, wxEXPAND);
}

void SkinEditor::setupSkinTreeView()
{
    auto* panel = getControl<wxPanel>("SkinEditorSkinTreeView");
    _skinTreeView = new wxutil::DeclarationTreeView(panel, decl::Type::Skin, _columns, wxDV_SINGLE | wxDV_NO_HEADER);
    _skinTreeView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &SkinEditor::onSkinSelectionChanged, this);

    // Single visible column, containing the directory/decl name and the icon
    _skinTreeView->AppendIconTextColumn(decl::getTypeName(decl::Type::Skin), _columns.iconAndName.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

    // Use the TreeModel's full string search function
    _skinTreeView->AddSearchColumn(_columns.leafName);

    auto* treeToolbar = new wxutil::ResourceTreeViewToolbar(panel, _skinTreeView);
    treeToolbar->EnableFavouriteManagement(false);

    auto definitionLabel = getControl<wxStaticText>("SkinEditorSkinDefinitionsLabel");
    definitionLabel->GetContainingSizer()->Detach(definitionLabel);
    definitionLabel->Reparent(treeToolbar);
    treeToolbar->GetLeftSizer()->Add(definitionLabel, 0, wxALIGN_LEFT);

    panel->GetSizer()->Add(treeToolbar, 0, wxEXPAND | wxBOTTOM, 6);
    panel->GetSizer()->Add(_skinTreeView, 1, wxEXPAND);
}

void SkinEditor::setupSelectedModelList()
{
    auto* panel = getControl<wxPanel>("SkinEditorSelectedModelList");
    _selectedModelList = wxutil::TreeView::CreateWithModel(panel, _selectedModels.get(), wxDV_SINGLE | wxDV_NO_HEADER);

    // Single visible column
    _selectedModelList->AppendIconTextColumn(_("Model"), _selectedModelColumns.name.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);
    _selectedModelList->EnableSearchPopup(false);

    auto item = panel->GetSizer()->Add(_selectedModelList, 1, wxEXPAND, 0);

    // Add a spacing to match the height of the model tree view toolbar
    auto toolbar = findNamedObject<wxWindow>(this, "ModelTreeViewToolbar");
    auto toolbarItem = toolbar->GetContainingSizer()->GetItem(toolbar);
    item->SetBorder(toolbarItem->GetSize().GetHeight() + 3);
    item->SetFlag(item->GetFlag() | wxTOP);
}

void SkinEditor::setupPreview()
{
    auto panel = getControl<wxPanel>("SkinEditorPreviewPanel");
    _modelPreview = std::make_unique<wxutil::ModelPreview>(panel);
    panel->GetSizer()->Add(_modelPreview->getWidget(), 1, wxEXPAND);

    panel = getControl<wxPanel>("SkinEditorDeclarationPanel");
    _sourceView = new wxutil::D3DeclarationViewCtrl(panel);
    panel->GetSizer()->Add(_sourceView, 1, wxEXPAND);
}

void SkinEditor::setupRemappingPanel()
{
    auto panel = getControl<wxPanel>("SkinEditorRemappingPanel");

    _remappingList = wxutil::TreeView::CreateWithModel(panel, _remappings.get(), wxDV_SINGLE);

    _remappingList->AppendToggleColumn(_("Active"), _remappingColumns.active.getColumnIndex(),
        wxDATAVIEW_CELL_ACTIVATABLE, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);
    _remappingList->AppendTextColumn(_("Original"), _remappingColumns.original.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);
    _remappingList->AppendTextColumn(_("Replacement"), _remappingColumns.replacement.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);
    _remappingList->EnableSearchPopup(false);

    panel->GetSizer()->Add(_remappingList, 1, wxEXPAND, 0);
}

decl::ISkin::Ptr SkinEditor::getSelectedSkin()
{
    auto selectedSkin = _skinTreeView->GetSelectedDeclName();
    return selectedSkin.empty() ? decl::ISkin::Ptr() : GlobalModelSkinCache().findSkin(selectedSkin);
}

void SkinEditor::updateSkinButtonSensitivity()
{
    auto selectedSkin = _skinTreeView->GetSelectedDeclName();

    getControl<wxButton>("SkinEditorCopyDefButton")->Enable(!selectedSkin.empty());
}

void SkinEditor::updateModelControlsFromSkin(const decl::ISkin::Ptr& skin)
{
    _selectedModels->Clear();

    if (!skin) return;

    for (const auto& model : skin->getModels())
    {
        auto row = _selectedModels->AddItem();
        row[_selectedModelColumns.name] = wxVariant(wxDataViewIconText(model));
        row.SendItemAdded();
    }
}

void SkinEditor::updateRemappingControlsFromSkin(const decl::ISkin::Ptr& skin)
{
    _remappings->Clear();

    if (!skin) return;

    for (const auto& remapping : skin->getAllRemappings())
    {
        auto row = _remappings->AddItem();

        row[_remappingColumns.active] = true;
        row[_remappingColumns.original] = remapping.Original;
        row[_remappingColumns.replacement] = remapping.Replacement;

        row.SendItemAdded();
    }
}

void SkinEditor::updateSourceView(const decl::ISkin::Ptr& skin)
{
    if (skin)
    {
        // Surround the definition with curly braces, these are not included
        auto definition = fmt::format("{0}\n{{{1}}}", skin->getDeclName(), skin->getBlockSyntax().contents);
        _sourceView->SetValue(definition);
    }
    else
    {
        _sourceView->SetValue("");
    }

    _sourceView->Enable(skin != nullptr);
}

void SkinEditor::updateSkinControlsFromSelection()
{
    auto skin = getSelectedSkin();

    getControl<wxWindow>("SkinEditorNotebook")->Enable(skin != nullptr);
    getControl<wxWindow>("SkinEditorEditSkinDefinitionLabel")->Enable(skin != nullptr);
    getControl<wxWindow>("SkinEditorSkinNameLabel")->Enable(skin != nullptr);
    getControl<wxWindow>("SkinEditorSkinName")->Enable(skin != nullptr);

    updateSourceView(skin);
    updateModelControlsFromSkin(skin);
    updateRemappingControlsFromSkin(skin);
    updateSkinButtonSensitivity();

    if (!skin)
    {
        getControl<wxTextCtrl>("SkinEditorSkinName")->SetValue("");
        return;
    }

    getControl<wxTextCtrl>("SkinEditorSkinName")->SetValue(skin->getDeclName());
}

void SkinEditor::onCloseButton(wxCommandEvent& ev)
{
    EndModal(wxCLOSE);
}

void SkinEditor::onSkinSelectionChanged(wxDataViewEvent& ev)
{
    updateSkinControlsFromSelection();
}

int SkinEditor::ShowModal()
{
    // Restore the position
    _windowPosition.applyPosition();

    _modelTreeView->Populate();
    _skinTreeView->Populate(std::make_shared<wxutil::ThreadedDeclarationTreePopulator>(decl::Type::Skin, _columns, SKIN_ICON));

    updateSkinControlsFromSelection();

    int returnCode = DialogBase::ShowModal();

    // Tell the position tracker to save the information
    _windowPosition.saveToPath(RKEY_WINDOW_STATE);
    _leftPanePosition.saveToPath(RKEY_SPLIT_POS_LEFT);
    _rightPanePosition.saveToPath(RKEY_SPLIT_POS_RIGHT);

    return returnCode;
}

void SkinEditor::ShowDialog(const cmd::ArgumentList& args)
{
    auto* editor = new SkinEditor;

    editor->ShowModal();
    editor->Destroy();
}

}
