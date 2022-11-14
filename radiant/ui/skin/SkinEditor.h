#pragma once

#include "icommandsystem.h"

#include "wxutil/WindowPosition.h"
#include "wxutil/PanedPosition.h"
#include "wxutil/XmlResourceBasedWidget.h"
#include "wxutil/dataview/DeclarationTreeView.h"
#include "wxutil/dialog/DialogBase.h"
#include "wxutil/preview/ModelPreview.h"

namespace ui
{

class ModelTreeView;

class SkinEditor final :
    public wxutil::DialogBase,
    private wxutil::XmlResourceBasedWidget
{
private:
    ModelTreeView* _modelTreeView;
    std::unique_ptr<wxutil::ModelPreview> _modelPreview;

    wxutil::DeclarationTreeView::Columns _columns;
    wxutil::DeclarationTreeView* _skinTreeView;

    struct SelectedModelColumns :
        public wxutil::TreeModel::ColumnRecord
    {
        SelectedModelColumns() :
            icon(add(wxutil::TreeModel::Column::Icon)),
            name(add(wxutil::TreeModel::Column::String))
        {}

        wxutil::TreeModel::Column icon;
        wxutil::TreeModel::Column name;
    };

    SelectedModelColumns _selectedModelColumns;
    wxutil::TreeModel::Ptr _selectedModels;
    wxutil::TreeView* _selectedModelList;

    wxutil::WindowPosition _windowPosition;
    wxutil::PanedPosition _leftPanePosition;
    wxutil::PanedPosition _rightPanePosition;

private:
    SkinEditor();
    ~SkinEditor() override;

public:
    int ShowModal() override;

    static void ShowDialog(const cmd::ArgumentList& args);

private:
    void setupModelTreeView();
    void setupSkinTreeView();
    void setupSelectedModelList();
    void setupPreview();

    template<typename ObjectClass>
    ObjectClass* getControl(const std::string& name)
    {
        return findNamedObject<ObjectClass>(this, name);
    }
};

}
