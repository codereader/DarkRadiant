#pragma once

#include "wxutil/dialog/DialogBase.h"
#include "wxutil/dataview/ResourceTreeView.h"
#include <wx/button.h>
#include <wx/textctrl.h>

namespace ui
{

class ImageFileTreeView;

/**
 * Modal dialog to select an image file (TGA, DDS, etc.) from the VFS
 */
class ImageFileSelector :
    public wxutil::DialogBase
{
public:
    // The texture type to look for
    struct TextureType
    {
        enum
        {
            Map     = 1 << 0,
            CubeMap = 1 << 1,
        };
    };

    struct Columns :
        public wxutil::ResourceTreeView::Columns
    {
        Columns() :
            wxutil::ResourceTreeView::Columns(),
            isCubeMapTexture(add(wxutil::TreeModel::Column::Boolean))
        {}

        wxutil::TreeModel::Column isCubeMapTexture;
    };

private:
    Columns _columns;
    ImageFileTreeView* _treeView;

    wxButton* _okButton;
    wxTextCtrl* _targetControl;
    std::string _previousValue;

public:
    ImageFileSelector(wxWindow* parent, wxTextCtrl* targetControl);

    int ShowModal() override;
    int ShowModal(const std::string& preselectItem);

    // Returns the path of the selected image file
    std::string GetSelectedImageFilePath();

    void SetVisibleTextureTypes(int typeToShow);

private:
    void onTreeSelectionChanged(wxDataViewEvent& ev);
};

}
