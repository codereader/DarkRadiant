#include "ImageFileSelector.h"

#include "ImageFilePopulator.h"

namespace ui
{

namespace
{
    const char* const WINDOW_TITLE = N_("Select Image File");
}

ImageFileSelector::ImageFileSelector(wxWindow* parent) :
    DialogBase(_(WINDOW_TITLE), parent)
{
    SetSizer(new wxBoxSizer(wxVERTICAL));

    _treeView = new wxutil::ResourceTreeView(this, _columns, wxDV_NO_HEADER);

    _treeView->AppendIconTextColumn(_("File"), _columns.iconAndName.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

    _treeView->AddSearchColumn(_columns.iconAndName);
    _treeView->SetExpandTopLevelItemsAfterPopulation(true);

    GetSizer()->Add(_treeView, 1, wxALL|wxEXPAND, 12);
    GetSizer()->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_RIGHT | wxBOTTOM | wxRIGHT, 12);

    FitToScreen(0.5f, 0.7f);
}

int ImageFileSelector::ShowModal()
{
    _treeView->Populate(std::make_shared<ImageFilePopulator>(_columns));

    return DialogBase::ShowModal();
}

}
