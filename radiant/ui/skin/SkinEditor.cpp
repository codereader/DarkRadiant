#include "SkinEditor.h"

#include "i18n.h"
#include "imodelcache.h"
#include "ieclass.h"

#include <wx/dataview.h>
#include <wx/splitter.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/bookctrl.h>

#include "gamelib.h"
#include "os/file.h"
#include "os/path.h"
#include "MaterialSelectorColumn.h"
#include "SkinEditorTreeView.h"
#include "decl/DeclLib.h"
#include "ui/modelselector/ModelTreeView.h"
#include "util/ScopedBoolLock.h"
#include "wxutil/FileChooser.h"
#include "wxutil/dataview/ResourceTreeViewToolbar.h"
#include "wxutil/dataview/ThreadedDeclarationTreePopulator.h"
#include "wxutil/decl/DeclFileInfo.h"
#include "wxutil/dialog/MessageBox.h"
#include "wxutil/sourceview/DeclarationSourceView.h"
#include "wxutil/sourceview/SourceView.h"

namespace ui
{

namespace
{
    constexpr const char* const DIALOG_TITLE = N_("Skin Editor");

    const std::string RKEY_ROOT = "user/ui/skinEditor/";
    const std::string RKEY_SPLIT_POS_LEFT = RKEY_ROOT + "splitPosLeft";
    const std::string RKEY_SPLIT_POS_RIGHT = RKEY_ROOT + "splitPosRight";
    const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
}

SkinEditor::SkinEditor() :
    DialogBase(DIALOG_TITLE),
    _selectedModels(new wxutil::TreeModel(_selectedModelColumns, true)),
    _remappings(new wxutil::TreeModel(_remappingColumns, true)),
    _controlUpdateInProgress(false),
    _skinUpdateInProgress(false)
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
    getControl<wxTextCtrl>("SkinEditorSkinName")->Bind(wxEVT_TEXT, &SkinEditor::onSkinNameChanged, this);

    getControl<wxNotebook>("SkinEditorNotebook")->Bind(wxEVT_NOTEBOOK_PAGE_CHANGED, [&](auto& ev)
    {
        _remappingList->CancelEditing(); // Stop editing when switching tabs
    });

    auto oldInfoPanel = getControl<wxPanel>("SkinEditorSaveNotePanel");
    auto declFileInfo = new wxutil::DeclFileInfo(oldInfoPanel->GetParent(), decl::Type::Skin);
    replaceControl(oldInfoPanel, declFileInfo);

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
    // Stop editing on all columns
    _remappingList->CancelEditing();

    _skinModifiedConn.disconnect();
}

void SkinEditor::setupModelTreeView()
{
    auto* panel = getControl<wxPanel>("SkinEditorModelTreeView");
    _modelTreeView = new ModelTreeView(panel);
    _modelTreeView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &SkinEditor::onModelTreeSelectionChanged, this);

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
    _skinTreeView = new SkinEditorTreeView(panel, _columns, wxDV_SINGLE | wxDV_NO_HEADER);
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

    getControl<wxButton>("SkinEditorNewDefButton")->Bind(wxEVT_BUTTON, &SkinEditor::onNewSkin, this);
    getControl<wxButton>("SkinEditorCopyDefButton")->Bind(wxEVT_BUTTON, &SkinEditor::onCopySkin, this);
    getControl<wxButton>("SkinEditorRevertButton")->Bind(wxEVT_BUTTON, &SkinEditor::onDiscardChanges, this);
    getControl<wxButton>("SkinEditorSaveButton")->Bind(wxEVT_BUTTON, &SkinEditor::onSaveChanges, this);
    getControl<wxButton>("SkinEditorDeleteDefButton")->Bind(wxEVT_BUTTON, &SkinEditor::onDeleteSkin, this);
}

void SkinEditor::setupSelectedModelList()
{
    auto* panel = getControl<wxPanel>("SkinEditorSelectedModelList");
    _selectedModelList = wxutil::TreeView::CreateWithModel(panel, _selectedModels.get(), wxDV_SINGLE | wxDV_NO_HEADER);
    _selectedModelList->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &SkinEditor::onSkinModelSelectionChanged, this);

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

    auto addButton = getControl<wxWindow>("SkinEditorAddModelButton");
    addButton->Bind(wxEVT_BUTTON, &SkinEditor::onAddModelToSkin, this);

    auto removeButton = getControl<wxWindow>("SkinEditorRemoveModelButton");
    removeButton->Bind(wxEVT_BUTTON, &SkinEditor::onRemoveModelFromSkin, this);
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

    auto originalColumn = new MaterialSelectorColumn(_("Original (click to edit)"), _remappingColumns.original.getColumnIndex());
    _remappingList->AppendColumn(originalColumn);

    auto replacementColumn = new MaterialSelectorColumn(_("Replacement (click to edit)"), _remappingColumns.replacement.getColumnIndex());
    _remappingList->AppendColumn(replacementColumn);

    replacementColumn->signal_onMaterialSelected().connect(
        sigc::mem_fun(*this, &SkinEditor::onReplacementEntryChanged));

    _remappingList->Bind(wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, &SkinEditor::onRemappingRowChanged, this);
    _remappingList->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &SkinEditor::onRemappingSelectionChanged, this);
    _remappingList->Bind(wxEVT_DATAVIEW_ITEM_EDITING_STARTED, &SkinEditor::onRemappingEditStarted, this);
    _remappingList->Bind(wxEVT_DATAVIEW_ITEM_EDITING_DONE, &SkinEditor::onRemappingEditDone, this);
    _remappingList->EnableSearchPopup(false);

    panel->GetSizer()->Prepend(_remappingList, 1, wxEXPAND, 0);

    auto populateButton = getControl<wxButton>("SkinEditorAddMaterialsFromModelsButton");
    populateButton->Bind(wxEVT_BUTTON, &SkinEditor::onPopulateMappingsFromModel, this);

    auto removeButton = getControl<wxButton>("SkinEditorRemoveMappingButton");
    removeButton->Bind(wxEVT_BUTTON, &SkinEditor::onRemoveSelectedMapping, this);
}

decl::ISkin::Ptr SkinEditor::getSelectedSkin()
{
    auto selectedSkin = _skinTreeView->GetSelectedDeclName();
    return selectedSkin.empty() ? decl::ISkin::Ptr() : GlobalModelSkinCache().findSkin(selectedSkin);
}

std::string SkinEditor::getSelectedModelFromTree()
{
    return _modelTreeView->GetSelectedModelPath();
}

std::string SkinEditor::getSelectedSkinModel()
{
    auto item = _selectedModelList->GetSelection();
    if (!item.IsOk()) return {};

    wxutil::TreeModel::Row row(item, *_selectedModels);
    return row[_selectedModelColumns.name].getString().ToStdString();
}

std::string SkinEditor::getSelectedRemappingSourceMaterial()
{
    auto item = _remappingList->GetSelection();
    if (!item.IsOk()) return {};

    wxutil::TreeModel::Row row(item, *_remappings);
    return row[_remappingColumns.original].getString().ToStdString();
}

void SkinEditor::updateSkinButtonSensitivity()
{
    auto skinCanBeModified = _skin && GlobalModelSkinCache().skinCanBeModified(_skin->getDeclName());

    getControl<wxButton>("SkinEditorCopyDefButton")->Enable(_skin != nullptr);
    getControl<wxButton>("SkinEditorRevertButton")->Enable(_skin && _skin->isModified());
    getControl<wxButton>("SkinEditorDeleteDefButton")->Enable(skinCanBeModified);
    getControl<wxButton>("SkinEditorSaveButton")->Enable(skinCanBeModified && _skin->isModified());
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
    _remappingList->CancelEditing(); // stop editing when re-populating anything
    _remappings->Clear();

    if (!skin) return;

    // The wildcard item goes first
    auto wildcardRow = _remappings->AddItem();

    wildcardRow[_remappingColumns.active] = false;
    wildcardRow[_remappingColumns.original] = "*";
    wildcardRow[_remappingColumns.replacement] = "";

    wildcardRow.SendItemAdded();

    for (const auto& remapping : skin->getAllRemappings())
    {
        auto row = _remappings->AddItem();

        if (remapping.Original == "*")
        {
            wildcardRow[_remappingColumns.active] = true;
            wildcardRow[_remappingColumns.replacement] = remapping.Replacement;
            wildcardRow.SendItemChanged();
            continue;
        }

        row[_remappingColumns.active] = true;
        row[_remappingColumns.original] = remapping.Original;
        row[_remappingColumns.replacement] = remapping.Replacement;

        row.SendItemAdded();
    }

    updateRemappingButtonSensitivity();
}

void SkinEditor::updateRemappingButtonSensitivity()
{
    auto selectedSource = getSelectedRemappingSourceMaterial();
    getControl<wxButton>("SkinEditorRemoveMappingButton")->Enable(!selectedSource.empty() && selectedSource != "*");
}

void SkinEditor::updateSourceView(const decl::ISkin::Ptr& skin)
{
    _sourceView->SetReadOnly(false);

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
    // No source editing at the moment
    _sourceView->SetReadOnly(true);
}

void SkinEditor::updateSkinControlsFromSelection()
{
    util::ScopedBoolLock lock(_controlUpdateInProgress);

    auto skin = getSelectedSkin();
    auto skinCanBeModified = skin && GlobalModelSkinCache().skinCanBeModified(skin->getDeclName());

    // Enable/disable notebook tabs independently to allow page switching
    getControl<wxWindow>("SkinEditorTargetModels")->Enable(skinCanBeModified);
    getControl<wxWindow>("SkinEditorRemappings")->Enable(skinCanBeModified);

    getControl<wxWindow>("SkinEditorEditSkinDefinitionLabel")->Enable(skinCanBeModified);
    getControl<wxWindow>("SkinEditorSkinNameLabel")->Enable(skinCanBeModified);
    getControl<wxWindow>("SkinEditorSkinName")->Enable(skinCanBeModified);

    updateSourceView(skin);
    updateModelControlsFromSkin(skin);
    updateRemappingControlsFromSkin(skin);
    updateSkinButtonSensitivity();
    updateModelButtonSensitivity();
    updateSkinTreeItem();

    auto name = skin ? skin->getDeclName() : "";
    auto nameCtrl = getControl<wxTextCtrl>("SkinEditorSkinName");

    if (nameCtrl->GetValue() != name)
    {
        nameCtrl->SetValue(name);
    }

    updateDeclFileInfo();
}

void SkinEditor::updateDeclFileInfo()
{
    auto declFileInfo = getControl<wxutil::DeclFileInfo>("SkinEditorSaveNotePanel");

    if (_skin)
    {
        declFileInfo->Show();
        declFileInfo->SetDeclarationName(_skin->getDeclName());
    }
    else
    {
        declFileInfo->Hide();
    }
}

void SkinEditor::updateModelButtonSensitivity()
{
    // Update the button sensitivity
    auto addButton = getControl<wxWindow>("SkinEditorAddModelButton");
    auto removeButton = getControl<wxWindow>("SkinEditorRemoveModelButton");
    auto selectedModel = getSelectedModelFromTree();
    auto selectedSkin = getSelectedSkin();

    // Add button is active if there's a skin and the selected model is not already part of the skin
    if (selectedSkin && !selectedModel.empty() && selectedSkin->getModels().count(selectedModel) == 0)
    {
        addButton->Enable();
    }
    else
    {
        addButton->Disable();
    }

    // Remove button is active if we have a selection in the skin model list
    removeButton->Enable(!getSelectedSkinModel().empty());
}

void SkinEditor::updateSkinTreeItem()
{
    if (!_skin) return;

    auto item = _skinTreeView->GetTreeModel()->FindString(_skin->getDeclName(), _columns.declName);

    if (!item.IsOk())
    {
        return;
    }

    bool isModified = _skin->isModified();

    wxutil::TreeModel::Row row(item, *_skinTreeView->GetModel());

    row[_columns.iconAndName].setAttr(!row[_columns.isFolder].getBool() ?
        wxutil::TreeViewItemStyle::Modified(isModified) : wxDataViewItemAttr()
    );

    wxDataViewIconText value = row[_columns.iconAndName];

    if (!isModified && value.GetText().EndsWith("*"))
    {
        value.SetText(value.GetText().RemoveLast(1));
        row[_columns.iconAndName] = wxVariant(value);
    }
    else if (isModified && !value.GetText().EndsWith("*"))
    {
        value.SetText(value.GetText() + "*");
        row[_columns.iconAndName] = wxVariant(value);
    }

    row.SendItemChanged();
}

void SkinEditor::updateModelPreview()
{
    if (!_skin)
    {
        _modelPreview->setModel({});
        _modelPreview->setSkin({});
        return;
    }

    // If we have a valid model selection, use it
    auto model = getSelectedSkinModel();

    // Fall back to use the first model if nothing selected
    if (model.empty())
    {
        const auto& allModels = _skin->getModels();
        model = allModels.empty() ? "" : *allModels.begin();
    }

    _modelPreview->setModel(model);
    _modelPreview->setSkin(_skin->getDeclName());
}

bool SkinEditor::askUserAboutModifiedSkin()
{
    if (!_skin) return true;

    // Get the original name
    auto origName = _skin->getDeclName();

    // Does not make sense to save a null skin
    assert(!origName.empty());

    // The skin we're editing has been changed from the saved one
    wxutil::Messagebox box(_("Save Changes"),
        fmt::format(_("Do you want to save the changes to the skin\n{0}?"), origName),
        IDialog::MESSAGE_SAVECONFIRMATION, this);

    switch (box.run())
    {
    case IDialog::RESULT_YES:
        // User wants to save, return true if save was successful
        return saveChanges();

    case IDialog::RESULT_NO:
        // user doesn't want to save
        discardChanges();
        return true; // proceed

    default:
        // Cancel or anything else: don't proceed
        return false;
    }
}

bool SkinEditor::okToCloseDialog()
{
    // Check all unsaved skins
    std::list<decl::ISkin::Ptr> modifiedDecls;
    for (const auto& skinName : GlobalModelSkinCache().getAllSkins())
    {
        if (auto skin = GlobalModelSkinCache().findSkin(skinName); skin && skin->isModified())
        {
            modifiedDecls.push_back(skin);
        }
    }

    for (const auto& skin : modifiedDecls)
    {
        _skinTreeView->SetSelectedDeclName(skin->getDeclName());

        // Prompt user to save or discard
        if (!askUserAboutModifiedSkin())
        {
            return false; // cancel the close event
        }
    }

    // At this point, everything is saved
    return true;
}

bool SkinEditor::saveChanges()
{
    if (!_skin || !_skin->isModified())
    {
        return true;
    }

    // Stop editing on all columns
    _remappingList->CancelEditing();

    if (skinHasBeenNewlyCreated())
    {
        while (true)
        {
            // Ask the user where to save it
            wxutil::FileChooser chooser(this, _("Select Skin file"), false, "skin", SKIN_FILE_EXTENSION);

            fs::path skinPath = game::current::getWriteableGameResourcePath();
            skinPath /= SKINS_FOLDER;

            if (!os::fileOrDirExists(skinPath.string()))
            {
                rMessage() << "Ensuring skin path: " << skinPath << std::endl;
                fs::create_directories(skinPath);
            }

            // Point the file chooser to that new file
            chooser.setCurrentPath(skinPath.string());
            chooser.askForOverwrite(false);

            auto result = chooser.display();

            if (result.empty())
            {
                return false; // save aborted
            }

            try
            {
                // Setting the path might fail if it's invalid, try to set it and break the loop
                auto skinsFolder = os::standardPathWithSlash(std::string(SKINS_FOLDER));
                auto pathRelativeToSkinsFolder = decl::geRelativeDeclSavePath(os::standardPath(result), skinsFolder, SKIN_FILE_EXTENSION);

                _skin->setFileInfo(vfs::FileInfo(skinsFolder, pathRelativeToSkinsFolder, vfs::Visibility::NORMAL));
                break;
            }
            catch (const std::invalid_argument& ex)
            {
                // Invalid path, notify user and get ready for the next round
                wxutil::Messagebox::ShowError(ex.what(), this);
            }
        }
    }

    try
    {
        // Write to the specified .mtr file
        _skin->commitModifications();
        GlobalDeclarationManager().saveDeclaration(_skin);

        _skinTreeView->SetSelectedDeclName(_skin->getDeclName());
    }
    catch (const std::runtime_error& ex)
    {
        rError() << "Could not save file: " << ex.what() << std::endl;
        wxutil::Messagebox::ShowError(ex.what(), this);

        return false; // failure means to abort the process
    }

    updateSkinTreeItem();
    updateSkinButtonSensitivity();

    return true;
}

void SkinEditor::discardChanges()
{
    if (!_skin) return;

    util::ScopedBoolLock lock(_skinUpdateInProgress);

    // Stop editing on all columns
    _remappingList->CancelEditing();

    if (_skin->isModified() && _skin->getBlockSyntax().fileInfo.name.empty())
    {
        // This decl has been created but not saved yet, discarding it means removing it
        try
        {
            GlobalDeclarationManager().removeDeclaration(decl::Type::Skin, _skin->getDeclName());
        }
        catch (const std::runtime_error& ex)
        {
            rError() << "Could not delete the skin: " << ex.what() << std::endl;
            wxutil::Messagebox::ShowError(ex.what(), this);
        }

        handleSkinSelectionChanged();
    }
    else
    {
        _skin->revertModifications();
        updateSkinControlsFromSelection();
    }
}

void SkinEditor::deleteSkin()
{
    if (!_skin) return;

    auto fileInfo = _skin->getBlockSyntax().fileInfo.isEmpty() ? _("") : " " + _skin->getBlockSyntax().fileInfo.fullPath();

    if (wxutil::Messagebox::Show(_("Confirm Removal"),
        fmt::format(_("The selected skin {0} will be removed,\nincluding its source text in the .skin file{1}.\n"
            "This action cannot be undone. Are you sure you want to remove this skin?"),
            _skin->getDeclName(), fileInfo), IDialog::MESSAGE_ASK, this) == IDialog::RESULT_NO)
    {
        return;
    }

    try
    {
        GlobalDeclarationManager().removeDeclaration(decl::Type::Skin, _skin->getDeclName());
    }
    catch (const std::runtime_error& ex)
    {
        rError() << "Could not delete the skin: " << ex.what() << std::endl;
        wxutil::Messagebox::ShowError(ex.what(), this);
    }

    handleSkinSelectionChanged();
}

bool SkinEditor::_onDeleteEvent()
{
    // Return true if okToCloseDialog() vetoes the close event
    return !okToCloseDialog();
}

void SkinEditor::onCloseButton(wxCommandEvent& ev)
{
    if (okToCloseDialog())
    {
        EndModal(wxCLOSE);
    }
}

void SkinEditor::onSkinSelectionChanged(wxDataViewEvent& ev)
{
    // Stop editing on all columns
    _remappingList->CancelEditing();

    if (_controlUpdateInProgress) return;

    handleSkinSelectionChanged();
}

void SkinEditor::handleSkinSelectionChanged()
{
    _skinModifiedConn.disconnect();

    _skin = getSelectedSkin();

    if (_skin)
    {
        _skinModifiedConn = _skin->signal_DeclarationChanged().connect(
            sigc::mem_fun(*this, &SkinEditor::onSkinDeclarationChanged));
    }

    updateSkinControlsFromSelection();
    updateModelPreview();
}

void SkinEditor::onSkinNameChanged(wxCommandEvent& ev)
{
    if (_controlUpdateInProgress) return;

    // Block declaration changed signals
    util::ScopedBoolLock lock(_skinUpdateInProgress);

    // Rename the active skin decl
    auto nameEntry = static_cast<wxTextCtrl*>(ev.GetEventObject());

    GlobalModelSkinCache().renameSkin(_skin->getDeclName(), nameEntry->GetValue().ToStdString());
    auto item = _skinTreeView->GetTreeModel()->FindString(_skin->getDeclName(), _columns.declName);

    // Make sure the item is selected again, it will be de-selected by the rename operation
    _skinTreeView->Select(item);
    _skinTreeView->EnsureVisible(item);
    handleSkinSelectionChanged(); // also updates all controls

    nameEntry->SetFocus();
}

void SkinEditor::onSkinDeclarationChanged()
{
    if (_skinUpdateInProgress) return;

    // Refresh all controls
    updateSkinControlsFromSelection();
}

void SkinEditor::onModelTreeSelectionChanged(wxDataViewEvent& ev)
{
    if (_controlUpdateInProgress) return;

    updateModelButtonSensitivity();
}

void SkinEditor::onSkinModelSelectionChanged(wxDataViewEvent& ev)
{
    if (_controlUpdateInProgress) return;

    updateModelButtonSensitivity();
    updateModelPreview();
}

void SkinEditor::onAddModelToSkin(wxCommandEvent& ev)
{
    if (_controlUpdateInProgress) return;

    // Block declaration changed signals
    util::ScopedBoolLock lock(_skinUpdateInProgress);

    auto skin = getSelectedSkin();
    auto model = getSelectedModelFromTree();

    if (!skin || model.empty()) return;

    skin->addModel(model);

    updateModelControlsFromSkin(skin);
    updateSkinTreeItem();
    updateSkinButtonSensitivity();

    // Select the added model
    auto modelItem = _selectedModels->FindString(model, _selectedModelColumns.name);
    _selectedModelList->Select(modelItem);

    // Preview should show the newly added model
    updateModelPreview();

    // In case this is a newly created skin, all model additions auomatically
    // populate the skin tree view with suggestions
    if (skinHasBeenNewlyCreated())
    {
        populateSkinListWithModelMaterials();
    }
}

bool SkinEditor::skinHasBeenNewlyCreated()
{
    return _skin && _skin->getBlockSyntax().fileInfo.fullPath().empty();
}

void SkinEditor::onRemoveModelFromSkin(wxCommandEvent& ev)
{
    if (_controlUpdateInProgress) return;

    // Block declaration changed signals
    util::ScopedBoolLock lock(_skinUpdateInProgress);

    auto skin = getSelectedSkin();
    auto model = getSelectedSkinModel();

    if (!skin || model.empty()) return;

    skin->removeModel(model);

    updateModelControlsFromSkin(skin);
    updateSkinTreeItem();
    updateSkinButtonSensitivity();
}

void SkinEditor::onRemappingRowChanged(wxDataViewEvent& ev)
{
    if (_controlUpdateInProgress || !_skin) return;

    util::ScopedBoolLock lock(_skinUpdateInProgress);

    // Load all active remapping rows into the skin
    _skin->clearRemappings();

    _remappings->ForeachNode([&](wxutil::TreeModel::Row& row)
    {
        if (!row[_remappingColumns.active].getBool()) return;

        auto original = row[_remappingColumns.original].getString().ToStdString();
        auto replacement = row[_remappingColumns.replacement].getString().ToStdString();

        if (original == replacement) return;

        _skin->addRemapping(decl::ISkin::Remapping{ std::move(original), std::move(replacement) });
    });

    updateSkinTreeItem();
    updateSourceView(_skin);
    updateSkinButtonSensitivity();
}

void SkinEditor::onRemappingEditStarted(wxDataViewEvent& ev)
{
    // Save the previous values into the model row, to restore it on cancel
    wxutil::TreeModel::Row row(ev.GetItem(), *_remappings);

    row[_remappingColumns.unchangedOriginal] = row[_remappingColumns.original].getString();
    row[_remappingColumns.unchangedReplacement] = row[_remappingColumns.replacement].getString();
}

void SkinEditor::onRemappingEditDone(wxDataViewEvent& ev)
{
    wxutil::TreeModel::Row row(ev.GetItem(), *_remappings);

    if (ev.IsEditCancelled())
    {
        // Revert to previous value
        row[_remappingColumns.original] = row[_remappingColumns.unchangedOriginal].getString();
        row[_remappingColumns.replacement] = row[_remappingColumns.unchangedReplacement].getString();
        row.SendItemChanged();
    }

    row[_remappingColumns.unchangedOriginal] = std::string();
    row[_remappingColumns.unchangedReplacement] = std::string();
}

void SkinEditor::onRemappingSelectionChanged(wxCommandEvent& ev)
{
    updateRemappingButtonSensitivity();
}

void SkinEditor::onRemoveSelectedMapping(wxCommandEvent& ev)
{
    if (_controlUpdateInProgress || !_skin) return;

    util::ScopedBoolLock lock(_skinUpdateInProgress);

    auto selectedSource = getSelectedRemappingSourceMaterial();

    if (selectedSource.empty()) return;

    _skin->removeRemapping(selectedSource);

    _remappingList->CancelEditing(); // stop editing

    // Remove the selected item only
    auto item = _remappings->FindString(selectedSource, _remappingColumns.original);
    _remappings->RemoveItem(item);

    updateSkinTreeItem();
    updateSourceView(_skin);
}

void SkinEditor::populateSkinListWithModelMaterials()
{
    std::set<std::string> allMaterials;

    // Get all associated models and ask them for their materials
    for (const auto& modelPath : _skin->getModels())
    {
        // Check for modelDefs, and redirect to load the mesh instead
        auto eclass = GlobalEntityClassManager().findModel(modelPath);
        auto model = GlobalModelCache().getModel(eclass ? eclass->getMesh() : modelPath);

        if (!model) continue;

        for (const auto& material : model->getActiveMaterials())
        {
            allMaterials.insert(material);
        }
    }

    std::set<std::string> existingMappingSources;

    _remappings->ForeachNode([&](const wxutil::TreeModel::Row& row)
        {
            existingMappingSources.insert(row[_remappingColumns.original].getString().ToStdString());
        });

    // Ensure a mapping entry for each collected material
    for (const auto& material : allMaterials)
    {
        if (existingMappingSources.count(material) > 0) continue;

        auto row = _remappings->AddItem();

        row[_remappingColumns.active] = false;
        row[_remappingColumns.original] = material;
        row[_remappingColumns.replacement] = material; // use the original as replacement

        row.SendItemAdded();
    }
}

void SkinEditor::onPopulateMappingsFromModel(wxCommandEvent& ev)
{
    if (_controlUpdateInProgress || !_skin) return;

    util::ScopedBoolLock lock(_controlUpdateInProgress);

    _remappingList->CancelEditing(); // stop editing

    populateSkinListWithModelMaterials();
}

void SkinEditor::onReplacementEntryChanged(const std::string& material)
{
    wxutil::TreeModel::Row row(_remappingList->GetSelection(), *_remappings);

    row[_remappingColumns.replacement] = material;
    row[_remappingColumns.active] = true; // activate edited rows
    row.SendItemChanged();
}

void SkinEditor::onDiscardChanges(wxCommandEvent& ev)
{
    if (!_skin) return;

    // The material we're editing has been changed from the saved one
    wxutil::Messagebox box(_("Discard Changes"),
        fmt::format(_("Do you want to discard all the changes to the skin\n{0}?"), _skin->getDeclName()),
        IDialog::MESSAGE_ASK, this);

    if (box.run() == IDialog::RESULT_YES)
    {
        discardChanges();
    }
}

void SkinEditor::onNewSkin(wxCommandEvent& ev)
{
    auto candidate = decl::generateNonConflictingName(decl::Type::Skin, "new_skin");
    auto newSkin = GlobalDeclarationManager().findOrCreateDeclaration(decl::Type::Skin, candidate);

    _skinTreeView->SetSelectedDeclName(newSkin->getDeclName());

    // Mark this skin as modified
    if (_skin)
    {
        _skin->addModel("-");
        _skin->removeModel("-");
    }
}

void SkinEditor::onCopySkin(wxCommandEvent& ev)
{
    if (!_skin) return;

    auto newSkinName = _skin->getDeclName() + _("_copy");
    auto newSkin = GlobalModelSkinCache().copySkin(_skin->getDeclName(), newSkinName);

    _skinTreeView->SetSelectedDeclName(newSkin->getDeclName());
}

void SkinEditor::onDeleteSkin(wxCommandEvent& ev)
{
    if (!_skin) return;

    deleteSkin();
}

void SkinEditor::onSaveChanges(wxCommandEvent& ev)
{
    if (!_skin) return;

    saveChanges();
}

int SkinEditor::ShowModal()
{
    // Restore the position
    _windowPosition.applyPosition();

    _modelTreeView->Populate();
    _skinTreeView->Populate();

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
