#include "DeclarationSelector.h"

#include <wx/sizer.h>
#include "wxutil/dataview/ResourceTreeViewToolbar.h"
#include "wxutil/DeclFileInfo.h"

namespace ui
{

DeclarationSelector::DeclarationSelector(wxWindow* parent, decl::Type declType) :
    DeclarationSelector(parent, declType, CreateDefaultColumns())
{}

DeclarationSelector::DeclarationSelector(wxWindow* parent, decl::Type declType, 
        const wxutil::DeclarationTreeView::Columns& columns) :
    wxPanel(parent),
    _declType(declType),
    _columns(columns),
    _treeView(nullptr),
    _horizontalSizer(nullptr),
    _treeViewSizerItem(nullptr)
{
    SetSizer(new wxBoxSizer(wxVERTICAL));
    createTreeView();

    auto* toolbar = new wxutil::ResourceTreeViewToolbar(this, _treeView);
    _declFileInfo = new wxutil::DeclFileInfo(this, _declType);

    auto treeVbox = new wxBoxSizer(wxVERTICAL);
    treeVbox->Add(toolbar, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 6);
    treeVbox->Add(_treeView, 1, wxEXPAND);
    treeVbox->Add(_declFileInfo, 0, wxEXPAND | wxTOP | wxBOTTOM, 6);

    _horizontalSizer = new wxBoxSizer(wxHORIZONTAL);
    _treeViewSizerItem = _horizontalSizer->Add(treeVbox, 1, wxEXPAND);
    // the horizontal sizer has room for a preview widget => AddPreviewToRightPane

    GetSizer()->Add(_horizontalSizer, 1, wxEXPAND);

    // a preview widget can be appended to the vertical sizer => AddPreviewToBottom
}

void DeclarationSelector::AddPreviewToRightPane(IDeclarationPreview* preview, int sizerProportion)
{
    auto widget = preview->GetPreviewWidget();

    // Tree view no longer takes full proportion after a full-size preview has been added
    if (sizerProportion == 1)
    {
        _treeViewSizerItem->SetProportion(0);
    }

    widget->Reparent(this);
    _horizontalSizer->Add(widget, sizerProportion, wxEXPAND | wxLEFT, 6);

    _previews.push_back(preview);
}

void DeclarationSelector::AddPreviewToBottom(IDeclarationPreview* preview, int sizerProportion)
{
    auto widget = preview->GetPreviewWidget();

    widget->Reparent(this);
    GetSizer()->Add(widget, sizerProportion, wxEXPAND | wxTOP, 3);

    _previews.push_back(preview);
}

void DeclarationSelector::createTreeView()
{
    _treeView = new wxutil::DeclarationTreeView(this, _declType, _columns, wxDV_NO_HEADER | wxDV_SINGLE);

    // Single visible column, containing the directory/decl name and the icon
    _treeView->AppendIconTextColumn(decl::getTypeName(_declType), _columns.iconAndName.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

    // Use the TreeModel's full string search function
    _treeView->AddSearchColumn(_columns.leafName);

    // Get selection and connect the changed callback
    _treeView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &DeclarationSelector::onTreeViewSelectionChanged, this);
    _treeView->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &DeclarationSelector::onTreeViewItemActivated, this);
}

wxutil::DeclarationTreeView* DeclarationSelector::GetTreeView() const
{
    return _treeView;
}

const wxutil::DeclarationTreeView::Columns& DeclarationSelector::GetColumns() const
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

void DeclarationSelector::PopulateTreeView(const wxutil::IResourceTreePopulator::Ptr& populator)
{
    _treeView->Populate(populator);
}

const wxutil::DeclarationTreeView::Columns& DeclarationSelector::CreateDefaultColumns()
{
    static wxutil::DeclarationTreeView::Columns _treeViewColumns;
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
    onTreeViewItemActivated();
    ev.Skip();
}

}
