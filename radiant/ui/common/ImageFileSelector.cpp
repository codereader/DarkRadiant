#include "ImageFileSelector.h"

#include "ImageFilePopulator.h"

namespace ui
{

namespace
{
    const char* const WINDOW_TITLE = N_("Select Image File");
}

ImageFileSelector::ImageFileSelector(wxWindow* parent, wxTextCtrl* targetControl) :
    DialogBase(_(WINDOW_TITLE), parent),
    _okButton(nullptr),
    _targetControl(targetControl)
{
    SetSizer(new wxBoxSizer(wxVERTICAL));

    _treeView = new wxutil::ResourceTreeView(this, _columns, wxDV_NO_HEADER);

    _treeView->AppendIconTextColumn(_("File"), _columns.iconAndName.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

    _treeView->AddSearchColumn(_columns.iconAndName);
    _treeView->SetExpandTopLevelItemsAfterPopulation(true);
    _treeView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &ImageFileSelector::onTreeSelectionChanged, this);

    auto dialogButtons = CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    _okButton = dialogButtons->GetAffirmativeButton();
    _okButton->Disable();

    GetSizer()->Add(_treeView, 1, wxALL|wxEXPAND, 12);
    GetSizer()->Add(dialogButtons, 0, wxALIGN_RIGHT | wxBOTTOM | wxRIGHT, 12);

    FitToScreen(0.5f, 0.7f);
}

int ImageFileSelector::ShowModal()
{
    _treeView->Populate(std::make_shared<ImageFilePopulator>(_columns));

    _previousValue = _targetControl->GetValue().ToStdString();

    auto result = DialogBase::ShowModal();

    if (result != wxID_OK)
    {
        // Restore the previous value on cancel
        _targetControl->SetValue(_previousValue);
    }

    return result;
}

std::string ImageFileSelector::GetSelection()
{
    auto item = _treeView->GetSelection();
    if (!item.IsOk()) return std::string();

    wxutil::TreeModel::Row row(item, *_treeView->GetModel());

    if (row[_columns.isFolder].getBool()) return std::string();

    return row[_columns.fullName];
}

void ImageFileSelector::onTreeSelectionChanged(wxDataViewEvent& ev)
{
    auto item = _treeView->GetSelection();
    
    if (!item.IsOk())
    {
        _okButton->Disable();
        return;
    }

    wxutil::TreeModel::Row row(item, *_treeView->GetModel());

    bool isFolder = row[_columns.isFolder].getBool();
    _okButton->Enable(!isFolder);

    if (!isFolder)
    {
        _targetControl->SetValue(row[_columns.fullName].getString());
    }
}

}
