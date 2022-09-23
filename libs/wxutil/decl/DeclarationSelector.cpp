#include "DeclarationSelector.h"

#include <wx/sizer.h>
#include <wx/splitter.h>
#include "../dataview/ResourceTreeViewToolbar.h"
#include "DeclFileInfo.h"
#include "ui/iuserinterface.h"

namespace wxutil
{

DeclarationSelector::DeclarationSelector(wxWindow* parent, decl::Type declType) :
    DeclarationSelector(parent, declType, CreateDefaultColumns())
{}

DeclarationSelector::DeclarationSelector(wxWindow* parent, decl::Type declType, 
        const DeclarationTreeView::Columns& columns) :
    wxPanel(parent),
    _declType(declType),
    _columns(columns),
    _treeView(nullptr),
    _leftPanel(nullptr),
    _rightPanel(nullptr)
{
    SetSizer(new wxBoxSizer(wxVERTICAL));

    _leftPanel = new wxPanel(this);
    _leftPanel->SetSizer(new wxBoxSizer(wxVERTICAL));

    GetSizer()->Add(_leftPanel, 1, wxEXPAND);

    createTreeView(_leftPanel);

    auto* toolbar = new ResourceTreeViewToolbar(_leftPanel, _treeView);
    _declFileInfo = new DeclFileInfo(_leftPanel, _declType);

    _treeVbox = new wxBoxSizer(wxVERTICAL);
    _treeVbox->Add(toolbar, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
    _treeVbox->Add(_treeView, 1, wxEXPAND);
    _treeVbox->Add(_declFileInfo, 0, wxEXPAND | wxTOP | wxBOTTOM, 6);
    // a preview widget can be appended to the vertical sizer => AddPreviewToBottom

    _leftPanel->GetSizer()->Add(_treeVbox, 1, wxEXPAND);
    // the right panel has room for a preview widget => AddPreviewToRightPane

    // Listen for decls-reloaded signal to refresh the tree
    _declsReloaded = GlobalDeclarationManager().signal_DeclsReloaded(_declType).connect(
        [this]() { GlobalUserInterface().dispatch([this]() { Populate(); }); }
    );
}

DeclarationSelector::~DeclarationSelector()
{
    _declsReloaded.disconnect();
}

void DeclarationSelector::AddPreviewToRightPane(ui::IDeclarationPreview* preview, int sizerProportion)
{
    if (_rightPanel)
    {
        throw std::logic_error("A preview is already present in the right panel");
    }

    // Split the window
    auto splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D | wxSP_LIVE_UPDATE);

    // Remove items from sizer, then add the splitter
    GetSizer()->Clear(false);
    GetSizer()->Add(splitter, 1, wxEXPAND);

    // Move the left panel as child to the splitter
    _leftPanel->Reparent(splitter);
    _rightPanel = new wxPanel(splitter);
    _rightPanel->SetSizer(new wxBoxSizer(wxVERTICAL));

    splitter->SetMinimumPaneSize(200); // no unsplitting
    splitter->SplitVertically(_leftPanel, _rightPanel, 350);

    _panedPosition = std::make_unique<PanedPosition>("selectorSplitter");
    _panedPosition->connect(splitter);

    auto widget = preview->GetPreviewWidget();

    widget->Reparent(_rightPanel);
    _rightPanel->GetSizer()->Add(widget, sizerProportion, wxEXPAND | wxLEFT, 6);

    _previews.push_back(preview);
}

void DeclarationSelector::AddWidgetToBottom(wxWindow* widget, int sizerProportion)
{
    widget->Reparent(_leftPanel);
    _treeVbox->Add(widget, sizerProportion, wxEXPAND | wxTOP, 3);
}

void DeclarationSelector::AddPreviewToBottom(ui::IDeclarationPreview* preview, int sizerProportion)
{
    auto widget = preview->GetPreviewWidget();

    AddWidgetToBottom(widget, sizerProportion);

    _previews.push_back(preview);
}

void DeclarationSelector::createTreeView(wxWindow* parent)
{
    _treeView = new DeclarationTreeView(parent, _declType, _columns, wxDV_NO_HEADER | wxDV_SINGLE);

    // Single visible column, containing the directory/decl name and the icon
    _treeView->AppendIconTextColumn(decl::getTypeName(_declType), _columns.iconAndName.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

    // Use the TreeModel's full string search function
    _treeView->AddSearchColumn(_columns.leafName);

    // Get selection and connect the changed callback
    _treeView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &DeclarationSelector::onTreeViewSelectionChanged, this);
    _treeView->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &DeclarationSelector::onTreeViewItemActivated, this);
}

void DeclarationSelector::FocusTreeView()
{
    _treeView->SetFocus();
}

void DeclarationSelector::loadFromPath(const std::string& registryKey)
{
    if (_panedPosition)
    {
        _panedPosition->loadFromPath(registryKey);
    }
}

void DeclarationSelector::saveToPath(const std::string& registryKey)
{
    if (_panedPosition)
    {
        _panedPosition->saveToPath(registryKey);
    }
}

DeclarationTreeView* DeclarationSelector::GetTreeView() const
{
    return _treeView;
}

const DeclarationTreeView::Columns& DeclarationSelector::GetColumns() const
{
    return _columns;
}

std::string DeclarationSelector::GetSelectedDeclName() const
{
    return _treeView->GetSelectedDeclName();
}

void DeclarationSelector::SetSelectedDeclName(const std::string& declName)
{
    _treeView->SetSelectedDeclName(declName);
}

void DeclarationSelector::PopulateTreeView(const IResourceTreePopulator::Ptr& populator)
{
    _treeView->Populate(populator);
}

const DeclarationTreeView::Columns& DeclarationSelector::CreateDefaultColumns()
{
    static DeclarationTreeView::Columns _treeViewColumns;
    return _treeViewColumns;
}

void DeclarationSelector::onTreeViewSelectionChanged(wxDataViewEvent& ev)
{
    auto declName = _treeView->GetSelectedDeclName();

    // Notify all previews
    for (auto preview : _previews)
    {
        preview->SetPreviewDeclName(declName);
    }

    // Update the info labels
    if (!declName.empty())
    {
        _declFileInfo->SetDeclarationName(declName);
    }
    else
    {
        _declFileInfo->Clear();
    }

    // Invoke the virtual method
    onTreeViewSelectionChanged();
    ev.Skip();
}

void DeclarationSelector::onTreeViewItemActivated(wxDataViewEvent& ev)
{
    // Invoke the virtual method
    if (!onTreeViewItemActivated())
    {
        ev.Skip();
    }
}

}
