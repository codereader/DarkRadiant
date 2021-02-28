#pragma once

#include "icommandsystem.h"
#include "ishaders.h"

#include "wxutil/dialog/DialogBase.h"
#include "wxutil/WindowPosition.h"
#include "wxutil/PanedPosition.h"
#include "wxutil/XmlResourceBasedWidget.h"
#include "wxutil/preview/ModelPreview.h"
#include "wxutil/SourceView.h"

#include "ui/common/MaterialTreeView.h"
#include "Binding.h"

namespace ui
{

class MaterialEditor :
    public wxutil::DialogBase,
    private wxutil::XmlResourceBasedWidget
{
private:
    MaterialTreeView* _treeView;

    wxutil::TreeModel::Ptr _stageList;
    wxutil::TreeView* _stageView;

    wxutil::WindowPosition _windowPosition;
    wxutil::PanedPosition _panedPosition;

    std::shared_ptr<wxutil::ModelPreview> _preview;
    wxutil::D3MaterialSourceViewCtrl* _sourceView;

    MaterialPtr _material;

    std::set<std::shared_ptr<Binding<MaterialPtr>>> _materialBindings;
    std::set<std::shared_ptr<Binding<ShaderLayerPtr>>> _stageBindings;
    std::map<Material::DeformType, wxPanel*> _deformPanels;

    struct StageProgramParmsColumns :
        public wxutil::TreeModel::ColumnRecord
    {
        StageProgramParmsColumns() :
            type(add(wxutil::TreeModel::Column::String)),
            index(add(wxutil::TreeModel::Column::String)),
            expression(add(wxutil::TreeModel::Column::String))
        {}

        wxutil::TreeModel::Column type;
        wxutil::TreeModel::Column index;
        wxutil::TreeModel::Column expression;
    };

    StageProgramParmsColumns _stageProgramColumns;
    wxutil::TreeModel::Ptr _stageProgramParameters;

private:
    MaterialEditor();

    int ShowModal() override;

public:
    static void ShowDialog(const cmd::ArgumentList& args);

    void _onClose(wxCommandEvent& ev);

private:
    void setupMaterialStageView();
    void setupMaterialStageProperties();
    void setupMaterialProperties();
    void setupMaterialSurfaceFlags();
    void setupMaterialShaderFlags();
    void setupMaterialLightFlags();
    void setupMaterialDeformPage();
    void setupSurfaceFlag(const std::string& controlName, Material::SurfaceFlags flag);
    void setupMaterialFlag(const std::string& controlName, Material::Flags flag);
    void setupStageFlag(const std::string& controlName, int flags);

    void updateControlsFromMaterial();
    void updateDeformControlsFromMaterial();
    void updateStageListFromMaterial();
    void updateMaterialPropertiesFromMaterial();

    void selectStageByIndex(std::size_t index);
    ShaderLayerPtr getSelectedStage();

    void updateStageControls();
    void updateStageBlendControls();
    void updateStageTexgenControls();

    void _onTreeViewSelectionChanged(wxDataViewEvent& ev);
    void _onStageListSelectionChanged(wxDataViewEvent& ev);

    // Shortcut
    template<typename ObjectClass>
    ObjectClass* getControl(const std::string& name)
    {
        return findNamedObject<ObjectClass>(this, name);
    }
};

}
