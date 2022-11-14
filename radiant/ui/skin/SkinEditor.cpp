#include "SkinEditor.h"

#include <wx/dataview.h>

#include "i18n.h"
#include "ui/modelselector/ModelTreeView.h"
#include "wxutil/dataview/ResourceTreeViewToolbar.h"
#include "wxutil/dataview/ThreadedDeclarationTreePopulator.h"

namespace ui
{

namespace
{
    constexpr const char* const DIALOG_TITLE = N_("Skin Editor");
    constexpr const char* const SKIN_ICON = "icon_skin.png";

    const std::string RKEY_ROOT = "user/ui/skinEditor/";
    const std::string RKEY_SPLIT_POS = RKEY_ROOT + "splitPos";
    const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
}

SkinEditor::SkinEditor() :
    DialogBase(DIALOG_TITLE),
    _selectedModels(new wxutil::TreeModel(_selectedModelColumns, true))
{
    loadNamedPanel(this, "SkinEditorMainPanel");

    makeLabelBold(this, "SkinEditorSkinDefinitionsLabel");
    makeLabelBold(this, "SkinEditorEditSkinDefinitionLabel");

    setupModelTreeView();
    setupSkinTreeView();
    setupSelectedModelList();

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

    // Single visible column, containing the directory/decl name and the icon
    _selectedModelList->AppendIconTextColumn(_("Model"), _columns.iconAndName.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);
    _selectedModelList->EnableSearchPopup(false);

    auto item = panel->GetSizer()->Add(_selectedModelList, 1, wxEXPAND, 0);

    // Add a spacing to match the height of the model tree view toolbar
    auto toolbar = findNamedObject<wxWindow>(this, "ModelTreeViewToolbar");
    auto toolbarItem = toolbar->GetContainingSizer()->GetItem(toolbar);
    item->SetBorder(toolbarItem->GetSize().GetHeight() + 3);
    item->SetFlag(item->GetFlag() | wxTOP);
}

int SkinEditor::ShowModal()
{
    // Restore the position
    _windowPosition.applyPosition();

    _modelTreeView->Populate();
    _skinTreeView->Populate(std::make_shared<wxutil::ThreadedDeclarationTreePopulator>(decl::Type::Skin, _columns, SKIN_ICON));

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
