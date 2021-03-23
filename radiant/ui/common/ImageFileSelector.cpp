#include "ImageFileSelector.h"

namespace ui
{

namespace
{
    const char* const WINDOW_TITLE = N_("Select Image File");
}

ImageFileSelector::ImageFileSelector() :
    DialogBase(_(WINDOW_TITLE))
{
    SetSizer(new wxBoxSizer(wxVERTICAL));

    _treeView = new wxutil::ResourceTreeView(this, _columns, wxDV_NO_HEADER);

    GetSizer()->Add(_treeView, 1, wxALL, 12);
    GetSizer()->Add(CreateStdDialogButtonSizer(wxOK | wxCANCEL), 0, wxALIGN_RIGHT | wxBOTTOM | wxRIGHT, 12);
}

int ImageFileSelector::ShowModal()
{
    return DialogBase::ShowModal();
}

}
