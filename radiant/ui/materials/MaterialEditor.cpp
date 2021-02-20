#include "MaterialEditor.h"

#include "i18n.h"
#include "ishaderexpression.h"

#include <wx/panel.h>
#include <wx/splitter.h>
#include <wx/textctrl.h>
#include <wx/collpane.h>
#include <wx/spinctrl.h>
#include <wx/combo.h>

#include "wxutil/SourceView.h"
#include "fmt/format.h"
#include "materials/ParseLib.h"

namespace ui
{

namespace
{
    const char* const DIALOG_TITLE = N_("Material Editor");
    const std::string RKEY_ROOT = "user/ui/materialEditor/";
    const std::string RKEY_SPLIT_POS = RKEY_ROOT + "splitPos";
    const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";

    // Columns for the stages list
    struct StageColumns :
        public wxutil::TreeModel::ColumnRecord
    {
        StageColumns() :
            name(add(wxutil::TreeModel::Column::String)),
            index(add(wxutil::TreeModel::Column::Integer)),
            visible(add(wxutil::TreeModel::Column::Boolean))
        {}

        wxutil::TreeModel::Column name;
        wxutil::TreeModel::Column index;
        wxutil::TreeModel::Column visible;
    };

    StageColumns& STAGE_COLS()
    {
        static StageColumns _i; 
        return _i; 
    }
}

MaterialEditor::MaterialEditor() :
    DialogBase(DIALOG_TITLE),
    _treeView(nullptr),
    _stageList(new wxutil::TreeModel(STAGE_COLS(), true)),
    _stageView(nullptr)
{
    loadNamedPanel(this, "MaterialEditorMainPanel");

    makeLabelBold(this, "MaterialEditorDefinitionLabel");
    makeLabelBold(this, "MaterialEditorMaterialPropertiesLabel");
    makeLabelBold(this, "MaterialEditorMaterialStagesLabel");
    makeLabelBold(this, "MaterialEditorStageSettingsLabel");

    // Wire up the close button
    getControl<wxButton>("MaterialEditorCloseButton")->Bind(wxEVT_BUTTON, &MaterialEditor::_onClose, this);

    // Add the treeview
    auto* panel = getControl<wxPanel>("MaterialEditorTreeView");
    _treeView = new MaterialTreeView(panel);
    _treeView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &MaterialEditor::_onTreeViewSelectionChanged, this);
    panel->GetSizer()->Add(_treeView, 1, wxEXPAND);

    // Setup the splitter and preview
    auto* splitter = getControl<wxSplitterWindow>("MaterialEditorSplitter");
    splitter->SetSashPosition(GetSize().GetWidth() * 0.6f);
    splitter->SetMinimumPaneSize(10); // disallow unsplitting

    // Set up the preview
    auto* previewPanel = getControl<wxPanel>("MaterialEditorPreviewPanel");
    _preview.reset(new wxutil::ModelPreview(previewPanel));

    _sourceView = new wxutil::D3MaterialSourceViewCtrl(previewPanel);

    previewPanel->GetSizer()->Add(_preview->getWidget(), 1, wxEXPAND);
    previewPanel->GetSizer()->Add(_sourceView, 1, wxEXPAND);

    setupMaterialProperties();
    setupMaterialStageView();
    setupMaterialSurfaceFlags();
    setupMaterialShaderFlags();
    setupMaterialLightFlags();

    // Set the default size of the window
    FitToScreen(0.8f, 0.6f);

    Layout();
    Fit();

    // Connect the window position tracker
    _windowPosition.loadFromPath(RKEY_WINDOW_STATE);
    _windowPosition.connect(this);
    _windowPosition.applyPosition();

    _panedPosition.connect(splitter);
    _panedPosition.loadFromPath(RKEY_SPLIT_POS);

    CenterOnParent();

    _treeView->Populate();

    updateControlsFromMaterial();
}

int MaterialEditor::ShowModal()
{
    // Restore the position
    _windowPosition.applyPosition();

    int returnCode = DialogBase::ShowModal();

    // Tell the position tracker to save the information
    _windowPosition.saveToPath(RKEY_WINDOW_STATE);

    return returnCode;
}

void MaterialEditor::_onClose(wxCommandEvent& ev)
{
    EndModal(wxID_CLOSE);
}

void MaterialEditor::ShowDialog(const cmd::ArgumentList& args)
{
    auto* editor = new MaterialEditor;

    editor->ShowModal();
    editor->Destroy();
}

void MaterialEditor::setupMaterialProperties()
{
    auto* typeDropdown = getControl<wxChoice>("MaterialType");

    typeDropdown->AppendString(""); // empty string for undefined

    for (const auto& pair : shaders::SurfaceTypeMapping)
    {
        typeDropdown->AppendString(pair.first);
    }

    auto* sortDropdown = getControl<wxComboBox>("MaterialSortValue");

    sortDropdown->AppendString(""); // empty string for undefined

    for (const auto& pair : shaders::PredefinedSortValues)
    {
        sortDropdown->AppendString(pair.first);
    }
    
    _bindings.emplace(std::make_shared<CheckBoxBinding>(getControl<wxCheckBox>("MaterialHasSortValue"),
        [](const MaterialPtr& material) { return (material->getParseFlags() & Material::PF_HasSortDefined) != 0; }));
}

void MaterialEditor::setupSurfaceFlag(const std::string& controlName, Material::SurfaceFlags flag)
{
    _bindings.emplace(std::make_shared<CheckBoxBinding>(getControl<wxCheckBox>(controlName),
        [=](const MaterialPtr& material)
    {
        return (material->getSurfaceFlags() & flag) != 0;
    }));
}

void MaterialEditor::setupMaterialFlag(const std::string& controlName, Material::Flags flag)
{
    _bindings.emplace(std::make_shared<CheckBoxBinding>(getControl<wxCheckBox>(controlName),
        [=](const MaterialPtr& material)
    {
        return (material->getMaterialFlags() & flag) != 0;
    }));
}

void MaterialEditor::setupMaterialLightFlags()
{
    _bindings.emplace(std::make_shared<CheckBoxBinding>(getControl<wxCheckBox>("MaterialIsAmbientLight"),
        [](const MaterialPtr& material) { return material->isAmbientLight(); }));

    _bindings.emplace(std::make_shared<CheckBoxBinding>(getControl<wxCheckBox>("MaterialIsAmbientCubicLight"),
        [](const MaterialPtr& material) { return material->isAmbientLight() && material->isCubicLight(); }));

    _bindings.emplace(std::make_shared<CheckBoxBinding>(getControl<wxCheckBox>("MaterialIsFogLight"),
        [](const MaterialPtr& material) { return material->isFogLight(); }));

    _bindings.emplace(std::make_shared<CheckBoxBinding>(getControl<wxCheckBox>("MaterialIsCubicLight"),
        [](const MaterialPtr& material) { return material->isCubicLight(); })); 
    
    _bindings.emplace(std::make_shared<CheckBoxBinding>(getControl<wxCheckBox>("MaterialIsBlendLight"),
        [](const MaterialPtr& material) { return material->isBlendLight(); }));
}

void MaterialEditor::setupMaterialShaderFlags()
{
    setupMaterialFlag("MaterialNoShadows", Material::FLAG_NOSHADOWS);
    setupMaterialFlag("MaterialNoSelfShadows", Material::FLAG_NOSELFSHADOW);
    setupMaterialFlag("MaterialForceShadows", Material::FLAG_FORCESHADOWS);
    setupMaterialFlag("MaterialTranslucent", Material::FLAG_TRANSLUCENT);
    setupMaterialFlag("MaterialNoFog", Material::FLAG_NOFOG);
    setupMaterialFlag("MaterialFlagNoOverlays", Material::FLAG_NOOVERLAYS);
    setupMaterialFlag("MaterialFlagForceOverlays", Material::FLAG_FORCEOVERLAYS);
    setupMaterialFlag("MaterialFlagForceOpaque", Material::FLAG_FORCEOPAQUE);
    setupMaterialFlag("MaterialFlagNoPortalFog", Material::FLAG_NOPORTALFOG);
    setupMaterialFlag("MaterialFlagUnsmoothedTangents", Material::FLAG_UNSMOOTHEDTANGENTS);
    setupMaterialFlag("MaterialFlagMirror", Material::FLAG_MIRROR);
    setupMaterialFlag("MaterialFlagHasPolygonOffset", Material::FLAG_POLYGONOFFSET);

    // Cull types
    _bindings.emplace(std::make_shared<CheckBoxBinding>(getControl<wxCheckBox>("MaterialTwoSided"),
        [](const MaterialPtr& material)
    {
        return material->getCullType() == Material::CULL_NONE;
    }));

    _bindings.emplace(std::make_shared<CheckBoxBinding>(getControl<wxCheckBox>("MaterialBackSided"),
        [](const MaterialPtr& material)
    {
        return material->getCullType() == Material::CULL_FRONT;
    }));

    // Global Clamping
    _bindings.emplace(std::make_shared<CheckBoxBinding>(getControl<wxCheckBox>("MaterialFlagClamp"),
        [](const MaterialPtr& material)
    {
        return material->getClampType() == CLAMP_NOREPEAT;
    }));

    _bindings.emplace(std::make_shared<CheckBoxBinding>(getControl<wxCheckBox>("MaterialFlagZeroClamp"),
        [](const MaterialPtr& material)
    {
        return material->getClampType() == CLAMP_ZEROCLAMP;
    }));

    _bindings.emplace(std::make_shared<CheckBoxBinding>(getControl<wxCheckBox>("MaterialFlagAlphaZeroClamp"),
        [](const MaterialPtr& material)
    {
        return material->getClampType() == CLAMP_ALPHAZEROCLAMP;
    }));

    // DECAL_MACRO
    _bindings.emplace(std::make_shared<CheckBoxBinding>(getControl<wxCheckBox>("MaterialHasDecalMacro"),
        [](const MaterialPtr& material) { return material->getParseFlags() & Material::PF_HasDecalMacro; }));

    // TWOSIDED_DECAL_MACRO
    _bindings.emplace(std::make_shared<CheckBoxBinding>(getControl<wxCheckBox>("MaterialHasTwoSidedDecalMacro"),
        [](const MaterialPtr& material) { return material->getParseFlags() & Material::PF_HasTwoSidedDecalMacro; }));

    // GLASS_MACRO
    _bindings.emplace(std::make_shared<CheckBoxBinding>(getControl<wxCheckBox>("MaterialHasGlassMacro"),
        [](const MaterialPtr& material) { return material->getParseFlags() & Material::PF_HasGlassMacro; }));

    // PARTICLE_MACRO
    _bindings.emplace(std::make_shared<CheckBoxBinding>(getControl<wxCheckBox>("MaterialHasParticleMacro"),
        [](const MaterialPtr& material) { return material->getParseFlags() & Material::PF_HasParticleMacro; }));
}

void MaterialEditor::setupMaterialSurfaceFlags()
{
    setupSurfaceFlag("MaterialSurfaceFlagSolid", Material::SurfaceFlags::SURF_SOLID);
    setupSurfaceFlag("MaterialSurfaceFlagWater", Material::SurfaceFlags::SURF_WATER);
    setupSurfaceFlag("MaterialSurfaceFlagPlayerclip", Material::SurfaceFlags::SURF_PLAYERCLIP);
    setupSurfaceFlag("MaterialSurfaceFlagMonsterclip", Material::SurfaceFlags::SURF_MONSTERCLIP);
    setupSurfaceFlag("MaterialSurfaceFlagMoveableClip", Material::SurfaceFlags::SURF_MOVEABLECLIP);
    setupSurfaceFlag("MaterialSurfaceFlagIkclip", Material::SurfaceFlags::SURF_IKCLIP);
    setupSurfaceFlag("MaterialSurfaceFlagBlood", Material::SurfaceFlags::SURF_BLOOD);
    setupSurfaceFlag("MaterialSurfaceFlagTrigger", Material::SurfaceFlags::SURF_TRIGGER);
    setupSurfaceFlag("MaterialSurfaceFlagAassolid", Material::SurfaceFlags::SURF_AASSOLID);
    setupSurfaceFlag("MaterialSurfaceFlagAasobstacle", Material::SurfaceFlags::SURF_AASOBSTACLE);
    setupSurfaceFlag("MaterialSurfaceFlagFlashlighttrigger", Material::SurfaceFlags::SURF_FLASHLIGHT_TRIGGER);
    setupSurfaceFlag("MaterialSurfaceFlagNonsolid", Material::SurfaceFlags::SURF_NONSOLID);
    setupSurfaceFlag("MaterialSurfaceFlagNullnormal", Material::SurfaceFlags::SURF_NULLNORMAL);
    setupSurfaceFlag("MaterialSurfaceFlagAreaportal", Material::SurfaceFlags::SURF_AREAPORTAL);
    setupSurfaceFlag("MaterialSurfaceFlagQernocarve", Material::SurfaceFlags::SURF_NOCARVE);
    setupSurfaceFlag("MaterialSurfaceFlagDiscrete", Material::SurfaceFlags::SURF_DISCRETE);
    setupSurfaceFlag("MaterialSurfaceFlagNofragment", Material::SurfaceFlags::SURF_NOFRAGMENT);
    setupSurfaceFlag("MaterialSurfaceFlagSlick", Material::SurfaceFlags::SURF_SLICK);
    setupSurfaceFlag("MaterialSurfaceFlagCollision", Material::SurfaceFlags::SURF_COLLISION);
    setupSurfaceFlag("MaterialSurfaceFlagNoimpact", Material::SurfaceFlags::SURF_NOIMPACT);
    setupSurfaceFlag("MaterialSurfaceFlagNodamage", Material::SurfaceFlags::SURF_NODAMAGE);
    setupSurfaceFlag("MaterialSurfaceFlagLadder", Material::SurfaceFlags::SURF_LADDER);
    setupSurfaceFlag("MaterialSurfaceFlagNosteps", Material::SurfaceFlags::SURF_NOSTEPS);
}

void MaterialEditor::setupMaterialStageView()
{
    // Stage view
    auto* panel = getControl<wxPanel>("MaterialEditorStageView");

    _stageView = wxutil::TreeView::CreateWithModel(panel, _stageList.get(), wxDV_NO_HEADER);
    panel->GetSizer()->Add(_stageView, 1, wxEXPAND);

    // Single text column
    _stageView->AppendTextColumn(_("Stage"), STAGE_COLS().name.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);
}

void MaterialEditor::_onTreeViewSelectionChanged(wxDataViewEvent& ev)
{
    // Update the preview if a texture is selected
    if (!_treeView->IsDirectorySelected())
    {
        _material = GlobalMaterialManager().getMaterialForName(_treeView->GetSelectedFullname());
    }
    else
    {
        _material.reset();
    }

    updateControlsFromMaterial();
}

void MaterialEditor::updateControlsFromMaterial()
{
    updateMaterialPropertiesFromMaterial();
}

void MaterialEditor::updateMaterialPropertiesFromMaterial()
{
    getControl<wxPanel>("MaterialEditorMaterialPropertiesPanel")->Enable(_material != nullptr);
    
    // Update all registered bindings
    for (const auto& binding : _bindings)
    {
        binding->setSource(_material);
    }

    if (_material)
    {
        getControl<wxTextCtrl>("MaterialDescription")->SetValue(_material->getDescription());

        // Type dropdown
        auto* materialTypeDropdown = getControl<wxChoice>("MaterialType");
        if (_material->getSurfaceType() == Material::SURFTYPE_DEFAULT)
        {
            materialTypeDropdown->Select(0);
        }
        else
        {
            auto surfType = shaders::getStringForSurfaceType(_material->getSurfaceType());
            materialTypeDropdown->Select(materialTypeDropdown->FindString(surfType));
        }

        // Polygon offset
        if (_material->getMaterialFlags() & Material::FLAG_POLYGONOFFSET)
        {
            getControl<wxSpinCtrlDouble>("MaterialPolygonOffsetValue")->SetValue(_material->getPolygonOffset());
        }
        else
        {
            getControl<wxSpinCtrlDouble>("MaterialPolygonOffsetValue")->SetValue(0.0);
        }

        // Sort dropdown
        auto* materialSortDropdown = getControl<wxComboBox>("MaterialSortValue");
        if (_material->getParseFlags() & Material::PF_HasSortDefined)
        {
            auto predefinedName = shaders::getStringForSortRequestValue(_material->getSortRequest());

            if (!predefinedName.empty())
            {
                materialSortDropdown->Select(materialSortDropdown->FindString(predefinedName));
            }
            else
            {
                materialSortDropdown->SetValue(string::to_string(_material->getSortRequest()));
            }
        }
        else
        {
            materialSortDropdown->Select(0);
        }

        // Light Falloff
        auto lightFalloffMap = _material->getLightFalloffExpression();
        getControl<wxTextCtrl>("MaterialLightFalloffMap")->SetValue(lightFalloffMap ? lightFalloffMap->getExpressionString() : "");

        auto lightFalloffCubeMap = _material->getLightFalloffCubeMapExpression();
        getControl<wxTextCtrl>("MaterialLightFalloffCubeMap")->SetValue(lightFalloffCubeMap ? lightFalloffCubeMap->getExpressionString() : "");
        
        // Spectrum
        bool hasSpectrum = _material->getParseFlags() & Material::PF_HasSpectrum;
        getControl<wxCheckBox>("MaterialHasSpectrum")->SetValue(hasSpectrum);
        getControl<wxSpinCtrl>("MaterialSpectrumValue")->Enable(hasSpectrum);
        getControl<wxSpinCtrl>("MaterialSpectrumValue")->SetValue(_material->getSpectrum());

        // DecalInfo
        bool hasDecalInfo = _material->getParseFlags() & Material::PF_HasDecalInfo;
        getControl<wxCheckBox>("MaterialHasDecalInfo")->SetValue(hasDecalInfo);
        getControl<wxPanel>("MaterialDecalInfoPanel")->Enable(hasDecalInfo);

        const auto& decalInfo = _material->getDecalInfo();
        getControl<wxSpinCtrlDouble>("MaterialEditorDecalInfoStaySeconds")->SetValue(decalInfo.stayMilliSeconds / 1000.0f);
        getControl<wxSpinCtrlDouble>("MaterialEditorDecalInfoFadeSeconds")->SetValue(decalInfo.fadeMilliSeconds / 1000.0f);
        getControl<wxTextCtrl>("MaterialDecalInfoStartRgb")->SetValue(fmt::format("({0} {1} {2} {3})", 
            decalInfo.startColour.x(), decalInfo.startColour.y(), decalInfo.startColour.z(), decalInfo.startColour.w()));
        getControl<wxTextCtrl>("MaterialDecalInfoEndRgb")->SetValue(fmt::format("({0} {1} {2} {3})",
            decalInfo.endColour.x(), decalInfo.endColour.y(), decalInfo.endColour.z(), decalInfo.endColour.w()));

        getControl<wxCheckBox>("MaterialHasRenderBump")->SetValue(!_material->getRenderBumpArguments().empty());
        getControl<wxTextCtrl>("MaterialRenderBumpArguments")->Enable(!_material->getRenderBumpArguments().empty());
        getControl<wxTextCtrl>("MaterialRenderBumpArguments")->SetValue(_material->getRenderBumpArguments());

        getControl<wxCheckBox>("MaterialHasRenderBumpFlat")->SetValue(!_material->getRenderBumpFlatArguments().empty());
        getControl<wxTextCtrl>("MaterialRenderBumpFlatArguments")->Enable(!_material->getRenderBumpFlatArguments().empty());
        getControl<wxTextCtrl>("MaterialRenderBumpFlatArguments")->SetValue(_material->getRenderBumpFlatArguments());

        // Surround the definition with curly braces, these are not included
        auto definition = fmt::format("{0}\n{{{1}}}", _material->getName(), _material->getDefinition());
        _sourceView->SetValue(definition);
    }
    else
    {
        getControl<wxCheckBox>("MaterialHasRenderBump")->SetValue(false);
        getControl<wxTextCtrl>("MaterialRenderBumpArguments")->SetValue("");

        getControl<wxCheckBox>("MaterialHasRenderBumpFlat")->SetValue(false);
        getControl<wxTextCtrl>("MaterialRenderBumpFlatArguments")->SetValue("");

        getControl<wxCheckBox>("MaterialHasDecalInfo")->SetValue(false);
        getControl<wxSpinCtrlDouble>("MaterialEditorDecalInfoStaySeconds")->SetValue(0);
        getControl<wxSpinCtrlDouble>("MaterialEditorDecalInfoFadeSeconds")->SetValue(0);
        getControl<wxTextCtrl>("MaterialDecalInfoStartRgb")->SetValue("");
        getControl<wxTextCtrl>("MaterialDecalInfoEndRgb")->SetValue("");

        getControl<wxCheckBox>("MaterialHasSpectrum")->SetValue(false);
        getControl<wxSpinCtrl>("MaterialSpectrumValue")->SetValue(0);
        getControl<wxTextCtrl>("MaterialLightFalloffMap")->SetValue("");
        getControl<wxTextCtrl>("MaterialDescription")->SetValue("");
        _sourceView->SetValue("");
    }
}

}
