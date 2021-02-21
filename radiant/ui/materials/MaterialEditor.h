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
#include "MaterialBinding.h"

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

    std::set<std::shared_ptr<MaterialBinding>> _bindings;

private:
    MaterialEditor();

    int ShowModal() override;

public:
    static void ShowDialog(const cmd::ArgumentList& args);

    void _onClose(wxCommandEvent& ev);

private:
    void setupMaterialStageView();
    void setupMaterialProperties();
    void setupMaterialSurfaceFlags();
    void setupMaterialShaderFlags();
    void setupMaterialLightFlags();
    void setupMaterialDeformPage();
    void setupSurfaceFlag(const std::string& controlName, Material::SurfaceFlags flag);
    void setupMaterialFlag(const std::string& controlName, Material::Flags flag);
    void updateControlsFromMaterial();
    void updateMaterialPropertiesFromMaterial();

    void _onTreeViewSelectionChanged(wxDataViewEvent& ev);

    // Shortcut
    template<typename ObjectClass>
    ObjectClass* getControl(const std::string& name)
    {
        return findNamedObject<ObjectClass>(this, name);
    }
};

}
