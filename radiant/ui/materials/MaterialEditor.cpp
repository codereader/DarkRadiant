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
    setupMaterialStageProperties();
    setupMaterialSurfaceFlags();
    setupMaterialShaderFlags();
    setupMaterialLightFlags();
    setupMaterialDeformPage();

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
    
    _materialBindings.emplace(std::make_shared<CheckBoxBinding<MaterialPtr>>(getControl<wxCheckBox>("MaterialHasSortValue"),
        [](const MaterialPtr& material) { return (material->getParseFlags() & Material::PF_HasSortDefined) != 0; }));
}

void MaterialEditor::setupSurfaceFlag(const std::string& controlName, Material::SurfaceFlags flag)
{
    _materialBindings.emplace(std::make_shared<CheckBoxBinding<MaterialPtr>>(getControl<wxCheckBox>(controlName),
        [=](const MaterialPtr& material)
    {
        return (material->getSurfaceFlags() & flag) != 0;
    }));
}

void MaterialEditor::setupMaterialFlag(const std::string& controlName, Material::Flags flag)
{
    _materialBindings.emplace(std::make_shared<CheckBoxBinding<MaterialPtr>>(getControl<wxCheckBox>(controlName),
        [=](const MaterialPtr& material)
    {
        return (material->getMaterialFlags() & flag) != 0;
    }));
}

void MaterialEditor::setupMaterialLightFlags()
{
    _materialBindings.emplace(std::make_shared<CheckBoxBinding<MaterialPtr>>(getControl<wxCheckBox>("MaterialIsAmbientLight"),
        [](const MaterialPtr& material) { return material->isAmbientLight(); }));

    _materialBindings.emplace(std::make_shared<CheckBoxBinding<MaterialPtr>>(getControl<wxCheckBox>("MaterialIsAmbientCubicLight"),
        [](const MaterialPtr& material) { return material->isAmbientLight() && material->isCubicLight(); }));

    _materialBindings.emplace(std::make_shared<CheckBoxBinding<MaterialPtr>>(getControl<wxCheckBox>("MaterialIsFogLight"),
        [](const MaterialPtr& material) { return material->isFogLight(); }));

    _materialBindings.emplace(std::make_shared<CheckBoxBinding<MaterialPtr>>(getControl<wxCheckBox>("MaterialIsCubicLight"),
        [](const MaterialPtr& material) { return material->isCubicLight(); })); 
    
    _materialBindings.emplace(std::make_shared<CheckBoxBinding<MaterialPtr>>(getControl<wxCheckBox>("MaterialIsBlendLight"),
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
    setupMaterialFlag("MaterialFlagIsLightGemSurf", Material::FLAG_ISLIGHTGEMSURF);

    // Cull types
    _materialBindings.emplace(std::make_shared<CheckBoxBinding<MaterialPtr>>(getControl<wxCheckBox>("MaterialTwoSided"),
        [](const MaterialPtr& material)
    {
        return material->getCullType() == Material::CULL_NONE;
    }));

    _materialBindings.emplace(std::make_shared<CheckBoxBinding<MaterialPtr>>(getControl<wxCheckBox>("MaterialBackSided"),
        [](const MaterialPtr& material)
    {
        return material->getCullType() == Material::CULL_FRONT;
    }));

    // Global Clamping
    _materialBindings.emplace(std::make_shared<CheckBoxBinding<MaterialPtr>>(getControl<wxCheckBox>("MaterialFlagClamp"),
        [](const MaterialPtr& material)
    {
        return material->getClampType() == CLAMP_NOREPEAT;
    }));

    _materialBindings.emplace(std::make_shared<CheckBoxBinding<MaterialPtr>>(getControl<wxCheckBox>("MaterialFlagZeroClamp"),
        [](const MaterialPtr& material)
    {
        return material->getClampType() == CLAMP_ZEROCLAMP;
    }));

    _materialBindings.emplace(std::make_shared<CheckBoxBinding<MaterialPtr>>(getControl<wxCheckBox>("MaterialFlagAlphaZeroClamp"),
        [](const MaterialPtr& material)
    {
        return material->getClampType() == CLAMP_ALPHAZEROCLAMP;
    }));

    // DECAL_MACRO
    _materialBindings.emplace(std::make_shared<CheckBoxBinding<MaterialPtr>>(getControl<wxCheckBox>("MaterialHasDecalMacro"),
        [](const MaterialPtr& material) { return material->getParseFlags() & Material::PF_HasDecalMacro; }));

    // TWOSIDED_DECAL_MACRO
    _materialBindings.emplace(std::make_shared<CheckBoxBinding<MaterialPtr>>(getControl<wxCheckBox>("MaterialHasTwoSidedDecalMacro"),
        [](const MaterialPtr& material) { return material->getParseFlags() & Material::PF_HasTwoSidedDecalMacro; }));

    // GLASS_MACRO
    _materialBindings.emplace(std::make_shared<CheckBoxBinding<MaterialPtr>>(getControl<wxCheckBox>("MaterialHasGlassMacro"),
        [](const MaterialPtr& material) { return material->getParseFlags() & Material::PF_HasGlassMacro; }));

    // PARTICLE_MACRO
    _materialBindings.emplace(std::make_shared<CheckBoxBinding<MaterialPtr>>(getControl<wxCheckBox>("MaterialHasParticleMacro"),
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

void MaterialEditor::setupMaterialDeformPage()
{
    auto* dropdown = getControl<wxChoice>("MaterialEditorDeformChoice");

    dropdown->AppendString(_("None"));

    for (const auto& pair : shaders::DeformTypeNames)
    {
        dropdown->AppendString(pair.first);
    }

    _deformPanels[Material::DEFORM_FLARE] = getControl<wxPanel>("DeformFlarePanel");
    _deformPanels[Material::DEFORM_EXPAND] = getControl<wxPanel>("DeformExpandPanel");
    _deformPanels[Material::DEFORM_MOVE] = getControl<wxPanel>("DeformMovePanel");
    _deformPanels[Material::DEFORM_TURBULENT] = getControl<wxPanel>("DeformTurbulentPanel");
    _deformPanels[Material::DEFORM_PARTICLE] = getControl<wxPanel>("DeformParticlePanel");
    _deformPanels[Material::DEFORM_PARTICLE2] = getControl<wxPanel>("DeformParticle2Panel");
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

    _stageView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &MaterialEditor::_onStageListSelectionChanged, this);
}

void MaterialEditor::setupStageFlag(const std::string& controlName, int flags)
{
    _stageBindings.emplace(std::make_shared<CheckBoxBinding<ShaderLayerPtr>>(getControl<wxCheckBox>(controlName),
        [=](const ShaderLayerPtr& layer)
    {
        return (layer->getStageFlags() & flags) == flags;
    }));
}

void MaterialEditor::setupMaterialStageProperties()
{
    setupStageFlag("MaterialStageFlagMaskRed", ShaderLayer::FLAG_MASK_RED);
    setupStageFlag("MaterialStageFlagMaskGreen", ShaderLayer::FLAG_MASK_GREEN);
    setupStageFlag("MaterialStageFlagMaskBlue", ShaderLayer::FLAG_MASK_BLUE);
    setupStageFlag("MaterialStageFlagMaskAlpha", ShaderLayer::FLAG_MASK_ALPHA);
    setupStageFlag("MaterialStageFlagMaskColour", ShaderLayer::FLAG_MASK_RED | ShaderLayer::FLAG_MASK_GREEN | ShaderLayer::FLAG_MASK_BLUE);
    setupStageFlag("MaterialStageFlagMaskDepth", ShaderLayer::FLAG_MASK_DEPTH);
    setupStageFlag("MaterialStageIgnoreAlphaTest", ShaderLayer::FLAG_IGNORE_ALPHATEST);

    _stageBindings.emplace(std::make_shared<CheckBoxBinding<ShaderLayerPtr>>(getControl<wxCheckBox>("MaterialStageHasAlphaTest"),
        [=](const ShaderLayerPtr& layer)
    {
        return layer->hasAlphaTest();
    }));

    getControl<wxChoice>("MaterialStageBlendType")->Append(std::vector<wxString>({
        "diffusemap", "bumpmap", "specularmap", "blend", "add", "filter", "modulate", "none", "Custom"
    }));
   
    getControl<wxChoice>("MaterialStageBlendTypeSrc")->Append(std::vector<wxString>({
        "", "gl_one", "gl_zero", "gl_dst_color", "gl_one_minus_dst_color", "gl_src_alpha", 
        "gl_one_minus_src_alpha", "gl_dst_alpha", "gl_one_minus_dst_alpha", "gl_src_alpha_saturate"
    }));

    getControl<wxChoice>("MaterialStageBlendTypeDest")->Append(std::vector<wxString>({
        "", "gl_one", "gl_zero", "gl_src_color", "gl_one_minus_src_color", "gl_src_alpha", 
        "gl_one_minus_src_alpha", "gl_dst_alpha", "gl_one_minus_dst_alpha"
    }));

    getControl<wxChoice>("MaterialStageMapType")->Append(std::vector<wxString>({ 
        "map", "cubeMap", "cameraCubeMap", "Special"
    }));

    // Texture
    _stageBindings.emplace(std::make_shared<CheckBoxBinding<ShaderLayerPtr>>(getControl<wxCheckBox>("MaterialStageClamp"),
        [=](const ShaderLayerPtr& layer) { return layer->getClampType() == CLAMP_NOREPEAT; }));
    _stageBindings.emplace(std::make_shared<CheckBoxBinding<ShaderLayerPtr>>(getControl<wxCheckBox>("MaterialStageNoclamp"),
        [=](const ShaderLayerPtr& layer)
    { 
        return layer->getClampType() == CLAMP_REPEAT && (layer->getParseFlags() & ShaderLayer::PF_HasNoclampKeyword) != 0; 
    }));
    _stageBindings.emplace(std::make_shared<CheckBoxBinding<ShaderLayerPtr>>(getControl<wxCheckBox>("MaterialStageZeroClamp"),
        [=](const ShaderLayerPtr& layer) { return layer->getClampType() == CLAMP_ZEROCLAMP; }));
    _stageBindings.emplace(std::make_shared<CheckBoxBinding<ShaderLayerPtr>>(getControl<wxCheckBox>("MaterialStageAlphaZeroClamp"),
        [=](const ShaderLayerPtr& layer) { return layer->getClampType() == CLAMP_ALPHAZEROCLAMP; }));

    setupStageFlag("MaterialStageFilterNearest", ShaderLayer::FLAG_FILTER_NEAREST);
    setupStageFlag("MaterialStageFilterLinear", ShaderLayer::FLAG_FILTER_LINEAR);
    setupStageFlag("MaterialStageHighQuality", ShaderLayer::FLAG_HIGHQUALITY);
    setupStageFlag("MaterialStageForceHighQuality", ShaderLayer::FLAG_FORCE_HIGHQUALITY);
    setupStageFlag("MaterialStageNoPicMip", ShaderLayer::FLAG_NO_PICMIP);

    auto texgenDropdown = getControl<wxChoice>("MaterialStageTexGenType");
    texgenDropdown->AppendString("");

    for (const auto& pair : shaders::TexGenTypeNames)
    {
        texgenDropdown->AppendString(pair.first);
    }
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

void MaterialEditor::_onStageListSelectionChanged(wxDataViewEvent& ev)
{
    updateStageControls();
}

void MaterialEditor::updateControlsFromMaterial()
{
    updateMaterialPropertiesFromMaterial();
    updateStageListFromMaterial();
}

inline std::string getDeformExpressionSafe(const MaterialPtr& material, std::size_t index)
{
    auto expr = material->getDeformExpression(index);
    return expr ? expr->getExpressionString() : std::string();
}

void MaterialEditor::updateDeformControlsFromMaterial()
{
    if (_material)
    {
        // Deform
        auto deformTypeName = shaders::getStringForDeformType(_material->getDeformType());
        auto deformDropdown = getControl<wxChoice>("MaterialEditorDeformChoice");

        if (!deformTypeName.empty())
        {
            deformDropdown->Select(deformDropdown->FindString(deformTypeName));

            switch (_material->getDeformType())
            {
            case Material::DEFORM_FLARE:
                getControl<wxTextCtrl>("MaterialDeformFlareSize")->SetValue(getDeformExpressionSafe(_material, 0));
                break;
            case Material::DEFORM_EXPAND:
                getControl<wxTextCtrl>("MaterialDeformExpandAmount")->SetValue(getDeformExpressionSafe(_material, 0));
                break;
            case Material::DEFORM_MOVE:
                getControl<wxTextCtrl>("MaterialDeformMoveAmount")->SetValue(getDeformExpressionSafe(_material, 0));
                break;
            case Material::DEFORM_TURBULENT:
                getControl<wxTextCtrl>("MaterialDeformTurbulentTableName")->SetValue(_material->getDeformDeclName());
                getControl<wxTextCtrl>("MaterialDeformTurbulentRange")->SetValue(getDeformExpressionSafe(_material, 0));
                getControl<wxTextCtrl>("MaterialDeformTurbulentTimeOffset")->SetValue(getDeformExpressionSafe(_material, 1));
                getControl<wxTextCtrl>("MaterialDeformTurbulentDomain")->SetValue(getDeformExpressionSafe(_material, 2));
                break;
            case Material::DEFORM_PARTICLE:
                getControl<wxTextCtrl>("MaterialDeformParticleDeclName")->SetValue(_material->getDeformDeclName());
                break;
            case Material::DEFORM_PARTICLE2:
                getControl<wxTextCtrl>("MaterialDeformParticle2DeclName")->SetValue(_material->getDeformDeclName());
                break;
            }
        }
        else
        {
            deformDropdown->Select(0);
        }

        // Hide the unrelated deform controls
        for (const auto& pair : _deformPanels)
        {
            pair.second->Show(_material->getDeformType() == pair.first);
        }

        getControl<wxPanel>("DeformPage")->Layout();
    }
    else // no material
    {
        getControl<wxChoice>("MaterialEditorDeformChoice")->Select(0);

        for (const auto& pair : _deformPanels)
        {
            pair.second->Hide();
        }

        getControl<wxPanel>("DeformPage")->Layout();
    }
}

inline std::string getBlendFuncString(const std::pair<std::string, std::string>& pair)
{
    return !pair.second.empty() ? fmt::format("{0}, {1}", pair.first, pair.second) : pair.first;
}

inline std::string getNameForLayer(const ShaderLayer& layer)
{
    switch (layer.getType())
    {
    case ShaderLayer::DIFFUSE:
        return "diffuse";
    case ShaderLayer::BUMP:
        return "bump";
    case ShaderLayer::SPECULAR:
        return "specular";
    case ShaderLayer::BLEND:
        return fmt::format("blend {0}", getBlendFuncString(layer.getBlendFuncStrings()));
    }

    return std::string("-");
}

void MaterialEditor::updateStageListFromMaterial()
{
    _stageList->Clear();

    getControl<wxPanel>("MaterialEditorStageListPanel")->Enable(_material != nullptr);
    getControl<wxPanel>("MaterialEditorStageSettingsPanel")->Enable(_material != nullptr);

    if (!_material) return;

    const auto& layers = _material->getAllLayers();

    int index = 0;
    for (const auto& layer : layers)
    {
        auto row = _stageList->AddItem();

        row[STAGE_COLS().index] = index;
        row[STAGE_COLS().name] = getNameForLayer(*layer);
        row[STAGE_COLS().visible] = true;

        row.SendItemAdded();

        ++index;
    }

    // Pre-select the first stage (it's ok if there are no stages)
    selectStageByIndex(0);
}

void MaterialEditor::updateMaterialPropertiesFromMaterial()
{
    getControl<wxPanel>("MaterialEditorMaterialPropertiesPanel")->Enable(_material != nullptr);
    
    // Update all registered bindings
    for (const auto& binding : _materialBindings)
    {
        binding->setSource(_material);
    }

    updateDeformControlsFromMaterial();

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

void MaterialEditor::selectStageByIndex(std::size_t index)
{
    auto item = _stageList->FindInteger(index, STAGE_COLS().index);

    if (item.IsOk())
    {
        _stageView->Select(item);
    }
    else
    {
        _stageView->UnselectAll();
    }

    updateStageControls();
}

ShaderLayerPtr MaterialEditor::getSelectedStage()
{
    auto selectedStageItem = _stageView->GetSelection();

    if (!selectedStageItem.IsOk() || !_material) return ShaderLayerPtr();

    const auto& layers = _material->getAllLayers();
    wxutil::TreeModel::Row stageRow(selectedStageItem, *_stageList);
    int stageIndex = stageRow[STAGE_COLS().index].getInteger();

    if (stageIndex >= 0 && stageIndex < layers.size())
    {
        return layers[stageIndex];
    }

    return ShaderLayerPtr();
}

void MaterialEditor::updateStageBlendControls()
{
    auto selectedStage = getSelectedStage();

    auto blendType = getControl<wxChoice>("MaterialStageBlendType");
    auto blendTypeSrc = getControl<wxChoice>("MaterialStageBlendTypeSrc");
    auto blendTypeDest = getControl<wxChoice>("MaterialStageBlendTypeDest");

    if (selectedStage)
    {
        blendType->Enable();
        auto blendTypeStrings = selectedStage->getBlendFuncStrings();

        switch (selectedStage->getType())
        {
        case ShaderLayer::DIFFUSE:
            blendTypeStrings.first = "diffusemap";
            blendTypeStrings.second.clear();
            break;
        case ShaderLayer::BUMP:
            blendTypeStrings.first = "bumpmap";
            blendTypeStrings.second.clear();
            break;
        case ShaderLayer::SPECULAR:
            blendTypeStrings.first = "specularmap";
            blendTypeStrings.second.clear();
            break;
        };

        blendTypeSrc->Enable(!blendTypeStrings.second.empty());
        blendTypeDest->Enable(!blendTypeStrings.second.empty());

        if (blendTypeStrings.second.empty())
        {
            blendType->SetStringSelection(blendTypeStrings.first);

            blendTypeSrc->SetStringSelection("");
            blendTypeDest->SetStringSelection("");

            // Get the actual src and dest blend types this shortcut is working with
            for (const auto& pair : shaders::BlendTypeShortcuts)
            {
                if (blendTypeStrings.first == pair.first)
                {
                    blendTypeSrc->SetStringSelection(pair.second.first);
                    blendTypeDest->SetStringSelection(pair.second.second);
                    break;
                }
            }
        }
        else
        {
            blendType->SetStringSelection("Custom");
            blendTypeSrc->SetStringSelection(blendTypeStrings.first);
            blendTypeDest->SetStringSelection(blendTypeStrings.second);
        }
    }
    else
    {
        blendType->Disable();
        blendTypeSrc->Disable();
        blendTypeDest->Disable();
    }
}

void MaterialEditor::updateStageControls()
{
    auto selectedStage = getSelectedStage();

    // Update all registered bindings
    for (const auto& binding : _stageBindings)
    {
        binding->setSource(selectedStage);
    }

    getControl<wxPanel>("MaterialEditorStageSettingsPanel")->Enable(selectedStage != nullptr);

    updateStageBlendControls();

    if (selectedStage)
    {
        selectedStage->evaluateExpressions(0); // initialise the values of this stage

        getControl<wxSpinCtrlDouble>("MaterialStageAlphaTestValue")->SetValue(selectedStage->getAlphaTest());
        getControl<wxSpinCtrlDouble>("MaterialStagePrivatePolygonOffset")->SetValue(selectedStage->getPrivatePolygonOffset());

        auto mapExpr = selectedStage->getMapExpression();
        auto imageMap = getControl<wxTextCtrl>("MaterialStageImageMap");
        imageMap->SetValue(mapExpr ? mapExpr->getExpressionString() : "");

        auto mapTypeNotSpecial = getControl<wxRadioButton>("MaterialStageMapTypeNotSpecial");
        auto specialMapPanel = getControl<wxPanel>("MaterialStageSpecialMapPanel");

        auto videoMapFile = getControl<wxTextCtrl>("MaterialStageVideoMapFile");
        auto videoMapLoop = getControl<wxCheckBox>("MaterialStageVideoMapLoop");
        auto soundMapWave = getControl<wxCheckBox>("MaterialStageSoundMapWaveform");

        auto remoteRenderMapWidth = getControl<wxSpinCtrl>("MaterialStageRemoteRenderMapWidth");
        auto remoteRenderMapHeight = getControl<wxSpinCtrl>("MaterialStageRemoteRenderMapHeight");

        auto mirrorRenderMapWidth = getControl<wxSpinCtrl>("MaterialStageMirrorRenderMapWidth");
        auto mirrorRenderMapHeight = getControl<wxSpinCtrl>("MaterialStageMirrorRenderMapHeight");

        auto mapType = getControl<wxChoice>("MaterialStageMapType");

        if (selectedStage->getMapType() == ShaderLayer::MapType::VideoMap ||
            selectedStage->getMapType() == ShaderLayer::MapType::SoundMap ||
            selectedStage->getMapType() == ShaderLayer::MapType::RemoteRenderMap ||
            selectedStage->getMapType() == ShaderLayer::MapType::MirrorRenderMap)
        {
            mapType->SetStringSelection("Special");
            mapTypeNotSpecial->SetValue(false);
            specialMapPanel->Enable();
            imageMap->Disable();
        }
        else
        {
            mapTypeNotSpecial->SetValue(true);
            specialMapPanel->Disable();
            imageMap->Enable();
            videoMapFile->SetValue("");
            videoMapLoop->SetValue(false);
            soundMapWave->SetValue(false);
            remoteRenderMapWidth->SetValue(0);
            remoteRenderMapHeight->SetValue(0);
            mirrorRenderMapWidth->SetValue(0);
            mirrorRenderMapHeight->SetValue(0);
        }

        switch (selectedStage->getMapType())
        {
        case ShaderLayer::MapType::Map:
            mapType->SetStringSelection("map");
            break;
        case ShaderLayer::MapType::CameraCubeMap:
            mapType->SetStringSelection("cameraCubeMap");
            break;
        case ShaderLayer::MapType::CubeMap:
            mapType->SetStringSelection("cubeMap");
            break;
        case ShaderLayer::MapType::VideoMap:
            {
                auto videoMapExpression = std::dynamic_pointer_cast<shaders::IVideoMapExpression>(selectedStage->getMapExpression());
                videoMapFile->SetValue(videoMapExpression ? videoMapExpression->getExpressionString() : "");
                videoMapLoop->SetValue(videoMapExpression ? videoMapExpression->isLooping() : false);
            }
            break;
        case ShaderLayer::MapType::SoundMap:
            {
                auto soundMapExpression = std::dynamic_pointer_cast<shaders::ISoundMapExpression>(selectedStage->getMapExpression());
                soundMapWave->SetValue(soundMapExpression ? soundMapExpression->isWaveform() : false);
            }
            break;
        case ShaderLayer::MapType::RemoteRenderMap:
            remoteRenderMapWidth->SetValue(static_cast<int>(selectedStage->getRenderMapSize().x()));
            remoteRenderMapHeight->SetValue(static_cast<int>(selectedStage->getRenderMapSize().y()));
            break;
        case ShaderLayer::MapType::MirrorRenderMap:
            mirrorRenderMapWidth->SetValue(static_cast<int>(selectedStage->getRenderMapSize().x()));
            mirrorRenderMapHeight->SetValue(static_cast<int>(selectedStage->getRenderMapSize().y()));
            break;
        }

        getControl<wxRadioButton>("MaterialStageVideoMap")->SetValue(selectedStage->getMapType() == ShaderLayer::MapType::VideoMap);
        getControl<wxRadioButton>("MaterialStageSoundMap")->SetValue(selectedStage->getMapType() == ShaderLayer::MapType::SoundMap);
        getControl<wxRadioButton>("MaterialStageRemoteRenderMap")->SetValue(selectedStage->getMapType() == ShaderLayer::MapType::RemoteRenderMap);
        getControl<wxRadioButton>("MaterialStageMirrorRenderMap")->SetValue(selectedStage->getMapType() == ShaderLayer::MapType::MirrorRenderMap);

        auto texgenDropdown = getControl<wxChoice>("MaterialStageTexGenType");

        if (selectedStage->getParseFlags() & ShaderLayer::PF_HasTexGenKeyword)
        {
            auto texgenName = shaders::getStringForTexGenType(selectedStage->getTexGenType());
            texgenDropdown->Select(texgenDropdown->FindString(texgenName));

            if (selectedStage->getTexGenType() == ShaderLayer::TEXGEN_WOBBLESKY)
            {
                getControl<wxPanel>("MaterialStageWobblySkyPanel")->Show();

                auto wobbleSkyX = selectedStage->getTexGenExpression(0);
                getControl<wxTextCtrl>("MaterialStageWobbleSkyX")->SetValue(wobbleSkyX ? wobbleSkyX->getExpressionString() : "");

                auto wobbleSkyY = selectedStage->getTexGenExpression(1);
                getControl<wxTextCtrl>("MaterialStageWobbleSkyY")->SetValue(wobbleSkyY ? wobbleSkyY->getExpressionString() : "");

                auto wobbleSkyZ = selectedStage->getTexGenExpression(2);
                getControl<wxTextCtrl>("MaterialStageWobbleSkyZ")->SetValue(wobbleSkyZ ? wobbleSkyZ->getExpressionString() : "");
            }
            else
            {
                getControl<wxPanel>("MaterialStageWobblySkyPanel")->Hide();
            }
        }
        else
        {
            texgenDropdown->Select(0);
            getControl<wxPanel>("MaterialStageWobblySkyPanel")->Hide();
        }
    }
    else
    {
        getControl<wxTextCtrl>("MaterialStageImageMap")->SetValue("");
        getControl<wxTextCtrl>("MaterialStageVideoMapFile")->SetValue("");
    }
}

}
