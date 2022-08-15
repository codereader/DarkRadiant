#pragma once

#include <sigc++/connection.h>
#include <sigc++/trackable.h>
#include "icommandsystem.h"
#include "ishaders.h"

#include "wxutil/dialog/DialogBase.h"
#include "wxutil/WindowPosition.h"
#include "wxutil/PanedPosition.h"
#include "wxutil/XmlResourceBasedWidget.h"
#include "MaterialPreview.h"
#include "wxutil/sourceview/SourceView.h"

#include "../MaterialTreeView.h"
#include "Binding.h"

namespace ui
{

class MaterialEditor :
    public wxutil::DialogBase,
    private wxutil::XmlResourceBasedWidget,
    public sigc::trackable
{
private:
    MaterialTreeView* _treeView;
    wxDataViewItem _selectedMaterialItem;

    wxutil::TreeModel::Ptr _stageList;
    wxutil::TreeView* _stageView;
    wxBitmap _iconVisible;
    wxBitmap _iconInvisible;

    std::multimap<int, std::pair<wxWindow*, wxString>> _notebookPages;

    wxutil::WindowPosition _windowPosition;
    wxutil::PanedPosition _panedPosition;

    std::unique_ptr<MaterialPreview> _preview;
    wxutil::D3MaterialSourceViewCtrl* _sourceView;

    MaterialPtr _material;
    sigc::connection _materialChanged;

    std::set<std::shared_ptr<Binding<MaterialPtr>>> _materialBindings;
    std::set<std::shared_ptr<Binding<IShaderLayer::Ptr>>> _stageBindings;
    std::map<Material::DeformType, wxPanel*> _deformPanels;

    struct StageProgramColumns :
        public wxutil::TreeModel::ColumnRecord
    {
        StageProgramColumns() :
            type(add(wxutil::TreeModel::Column::String)),
            index(add(wxutil::TreeModel::Column::String)),
            expression(add(wxutil::TreeModel::Column::String))
        {}

        wxutil::TreeModel::Column type;
        wxutil::TreeModel::Column index;
        wxutil::TreeModel::Column expression;
    };

    struct StageTransformColumns :
        public wxutil::TreeModel::ColumnRecord
    {
        StageTransformColumns() :
            type(add(wxutil::TreeModel::Column::String)),
            index(add(wxutil::TreeModel::Column::String)),
            expression1(add(wxutil::TreeModel::Column::String)),
            expression2(add(wxutil::TreeModel::Column::String))
        {}

        wxutil::TreeModel::Column type;
        wxutil::TreeModel::Column index;
        wxutil::TreeModel::Column expression1;
        wxutil::TreeModel::Column expression2;
    };

    StageProgramColumns _stageProgramColumns;
    wxutil::TreeModel::Ptr _stageProgramParameters;

    StageTransformColumns _stageTransformationColumns;
    wxutil::TreeModel::Ptr _stageTransformations;
    wxutil::TreeView* _stageTransformView;

    bool _stageUpdateInProgress;
    bool _materialUpdateInProgress;
    bool _previewSceneUpdateInProgress;
    bool _updateFromSourceTextInProgress;
    bool _sourceTextUpdateInProgress;

    std::string _materialToPreselect;

private:
    MaterialEditor();
    ~MaterialEditor();

public:
    int ShowModal() override;
    int ShowModal(const std::string& materialToPreselect);

    static void ShowDialog(const cmd::ArgumentList& args);

protected:
    // Intercept close commands from pressing ESC keys
    bool _onDeleteEvent() override;

private:
    void loadSettings();
    void saveSettings();
    
    // Asks user to save each unmodified material.
    // Returns true if it is safe to go ahead and close the dialog
    bool okToCloseDialog();

    void setupBasicMaterialPage();
    void setupMaterialTreeView();
    void setupMaterialStageView();
    void setupMaterialStageProperties();
    void setupMaterialProperties();
    void setupMaterialSurfaceFlags();
    void setupMaterialShaderFlags();
    void setupMaterialLightFlags();
    void setupMaterialDeformPage();
    void setupSurfaceFlag(const std::string& controlName, Material::SurfaceFlags flag);
    void setupMaterialFlag(const std::string& controlName, Material::Flags flag);
    void setupStageFlag(const std::string& controlName, IShaderLayer::Flags flags);
    void setupSourceTextPanel(wxWindow* previewPanel);
    void setupPreviewLightProperties(wxWindow* previewPanel);

    void createExpressionBinding(const std::string& textCtrlName,
        const std::function<shaders::IShaderExpression::Ptr(const IShaderLayer::Ptr&)>& loadFunc,
        const std::function<void(const IEditableShaderLayer::Ptr&, const std::string&)>& saveFunc);
    void createRadioButtonBinding(const std::string& ctrlName,
        const std::function<bool(const IShaderLayer::Ptr&)>& loadFunc,
        const std::function<void(const IEditableShaderLayer::Ptr&, bool)>& saveFunc);

    void createSpinCtrlBinding(const std::string& ctrlName,
        const std::function<int(const IShaderLayer::Ptr&)>& loadFunc,
        const std::function<void(const IEditableShaderLayer::Ptr&, int)>& saveFunc);
    void createSpinCtrlDoubleBinding(const std::string& ctrlName,
        const std::function<double(const IShaderLayer::Ptr&)>& loadFunc,
        const std::function<void(const IEditableShaderLayer::Ptr&, double)>& saveFunc);

    void updateControlsFromMaterial();
    void updateDeformControlsFromMaterial();
    void updateStageListFromMaterial();
    void updateMaterialPropertiesFromMaterial();

    void selectStageByIndex(std::size_t index);
    void selectMaterial(const MaterialPtr& material);
    IShaderLayer::Ptr getSelectedStage();
    IEditableShaderLayer::Ptr getEditableStageForSelection();
    std::pair<IShaderLayer::Ptr, std::size_t> findMaterialStageByType(IShaderLayer::Type type);
    void assignDecalInfoToMaterial(const MaterialPtr& material, bool isEnabled);
    void createDecalColourBinding(const std::string& controlName, const std::function<double(const MaterialPtr&)>& loadFunc);

    void updateBasicPageFromMaterial();
    void updateBasicImagePreview();
    void updateBasicFrobStageControls();
    void updateStageControls();
    void updateStageBlendControls();
    void updateStageTextureControls();
    void updateStageProgramControls();
    void updateStageTransformControls();
    void updateStageColoredStatus();
    void updateNameOfSelectedStage();
    void moveStagePosition(int direction);
    void updateStageButtonSensitivity();
    void updateSettingsNotebook();
    void updateMaterialControlSensitivity();
    void updateSourceView();
    void updateMaterialTreeItem();
    void updateMaterialNameControl();
    void handleMaterialSelectionChange();

    bool isAllowedToChangeMaterial();
    bool askUserAboutModifiedMaterial();
    bool saveCurrentMaterial();
    void deleteCurrentMaterial();
    void revertCurrentMaterial();
    void copySelectedMaterial();

    void _onMaterialNameChanged(wxCommandEvent& ev);
    void _onMaterialSelectionChanged(wxDataViewEvent& ev);
    void _onNewMaterial(wxCommandEvent& ev);
    void _onSaveMaterial(wxCommandEvent& ev);
    void _onDeleteMaterial(wxCommandEvent& ev);
    void _onCopyMaterial(wxCommandEvent& ev);
    void _onRevertMaterial(wxCommandEvent& ev);
    void _onUnlockMaterial(wxCommandEvent& ev);
    void _onStageListSelectionChanged(wxDataViewEvent& ev);
    void _onStageListItemActivated(wxDataViewEvent& ev);
    void _onMaterialTypeChoice(wxCommandEvent& ev);
    void _onAddStageTransform(wxCommandEvent& ev);
    void _onRemoveStageTransform(wxCommandEvent& ev);
    void _onStageTransformEdited(wxDataViewEvent& ev);
    void _onStageColoredChecked(const IEditableShaderLayer::Ptr& layer, bool newValue);
    void _onStageMapTypeChanged(wxCommandEvent& ev);
    void _onStageBlendTypeChanged(wxCommandEvent& ev);
    void _onAddStage(wxCommandEvent& ev);
    void _onRemoveStage(wxCommandEvent& ev);
    void _onToggleStage(wxCommandEvent& ev);
    void _onDuplicateStage(wxCommandEvent& ev);
    void _onSortRequestChanged(wxCommandEvent& ev);
    void _onBasicMapEntryChanged(const std::string& entryName, IShaderLayer::Type type);
    void _onBasicAddFrobStages(wxCommandEvent& ev);
    void _onBasicRemoveFrobStages(wxCommandEvent& ev);
    void _onBasicTestFrobStages(wxMouseEvent& ev);
    void _onClose(wxCommandEvent& ev);
    void _onReloadImages(wxCommandEvent& ev);
    void _onSourceTextChanged(wxStyledTextEvent& ev);

    void applyMapExpressionToStage(const IEditableShaderLayer::Ptr& stage, const std::string& value);
    void toggleSelectedStage();
    void onMaterialChanged();
    void convertTextCtrlToMapExpressionEntry(const std::string& ctrlName);

    decl::DeclarationBlockSyntax getBlockSyntaxFromSourceView();

    // Shortcut
    template<typename ObjectClass>
    ObjectClass* getControl(const std::string& name)
    {
        return findNamedObject<ObjectClass>(this, name);
    }
};

}
