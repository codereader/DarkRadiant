#include "ImageFileSelector.h"

#include "i18n.h"
#include "ImageFilePopulator.h"
#include "wxutil/dataview/ResourceTreeViewToolbar.h"
#include <wx/sizer.h>

namespace ui
{

namespace
{
    const char* const WINDOW_TITLE = N_("Select Image File");
}

class ImageFileTreeView final :
    public wxutil::ResourceTreeView
{
private:
    const ImageFileSelector::Columns& _columns;

    int _textureTypesToShow;

public:
    ImageFileTreeView(wxWindow* parent, const ImageFileSelector::Columns& columns) :
        ResourceTreeView(parent, columns, wxDV_NO_HEADER),
        _columns(columns)
    {}

    void SetVisibleTextureTypes(int typesToShow)
    {
        _textureTypesToShow = typesToShow;
    }

protected:
    bool IsTreeModelRowVisible(wxutil::TreeModel::Row& row) override
    {
        bool isCubeMap = row[_columns.isCubeMapTexture].getBool();

        if (!row[_columns.isFolder].getBool() &&
            isCubeMap && (_textureTypesToShow & ImageFileSelector::TextureType::CubeMap) == 0 ||
            !isCubeMap && (_textureTypesToShow & ImageFileSelector::TextureType::Map) == 0)
        {
            return false;
        }

        return ResourceTreeView::IsTreeModelRowVisible(row);
    }
};

ImageFileSelector::ImageFileSelector(wxWindow* parent, wxTextCtrl* targetControl) :
    DialogBase(_(WINDOW_TITLE), parent),
    _okButton(nullptr),
    _targetControl(targetControl)
{
    SetSizer(new wxBoxSizer(wxVERTICAL));

    _treeView = new ImageFileTreeView(this, _columns);

    _treeView->AppendIconTextColumn(_("File"), _columns.iconAndName.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

    _treeView->AddSearchColumn(_columns.iconAndName);
    _treeView->SetExpandTopLevelItemsAfterPopulation(true);
    _treeView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &ImageFileSelector::onTreeSelectionChanged, this);

    auto dialogButtons = CreateStdDialogButtonSizer(wxOK | wxCANCEL);
    _okButton = dialogButtons->GetAffirmativeButton();
    _okButton->Disable();

    auto toolbar = new wxutil::ResourceTreeViewToolbar(this, _treeView);
    toolbar->EnableFavouriteManagement(false);

    auto treeViewSizer = new wxBoxSizer(wxVERTICAL);
    treeViewSizer->Add(toolbar, 0, wxEXPAND | wxBOTTOM, 6);
    treeViewSizer->Add(_treeView, 1, wxALL | wxEXPAND, 0);

    GetSizer()->Add(treeViewSizer, 1, wxALL|wxEXPAND, 12);
    GetSizer()->Add(dialogButtons, 0, wxALIGN_RIGHT | wxBOTTOM | wxRIGHT, 12);

    FitToScreen(0.5f, 0.7f);
}

int ImageFileSelector::ShowModal(const std::string& preselectItem)
{
    _treeView->Populate(std::make_shared<ImageFilePopulator>(_columns));

    if (!preselectItem.empty())
    {
        _treeView->SetSelectedFullname(preselectItem);
    }

    _previousValue = _targetControl->GetValue().ToStdString();

    auto result = DialogBase::ShowModal();

    if (result != wxID_OK && _targetControl->GetValue() != _previousValue)
    {
        // Restore the previous value on cancel
        _targetControl->SetValue(_previousValue);
    }

    return result;
}

int ImageFileSelector::ShowModal()
{
    return ShowModal(std::string());
}

std::string ImageFileSelector::GetSelectedImageFilePath()
{
    auto item = _treeView->GetSelection();
    if (!item.IsOk()) return std::string();

    wxutil::TreeModel::Row row(item, *_treeView->GetModel());

    if (row[_columns.isFolder].getBool()) return std::string();

    return row[_columns.fullName];
}

void ImageFileSelector::SetVisibleTextureTypes(int typesToShow)
{
    _treeView->SetVisibleTextureTypes(typesToShow);
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

    // Only update the target control if the value differs
    auto newValue = row[_columns.fullName].getString();
    if (!isFolder && _targetControl && _targetControl->GetValue() != newValue)
    {
        _targetControl->SetValue(newValue);
    }
}

}
