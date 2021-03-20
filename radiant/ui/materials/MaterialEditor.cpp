#include "MaterialEditor.h"

#include "i18n.h"
#include "ishaderexpression.h"

#include <wx/panel.h>
#include <wx/splitter.h>
#include <wx/textctrl.h>
#include <wx/collpane.h>
#include <wx/spinctrl.h>
#include <wx/combo.h>
#include <wx/dataview.h>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/combobox.h>
#include <wx/notebook.h>
#include <wx/radiobut.h>

#include "wxutil/SourceView.h"
#include "fmt/format.h"
#include "string/join.h"
#include "materials/ParseLib.h"
#include "ExpressionBinding.h"
#include "RadioButtonBinding.h"
#include "SpinCtrlBinding.h"

namespace ui
{

namespace
{
    const char* const DIALOG_TITLE = N_("Material Editor");
    const std::string RKEY_ROOT = "user/ui/materialEditor/";
    const std::string RKEY_SPLIT_POS = RKEY_ROOT + "splitPos";
    const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";

    const char* const SPECIAL_MAP_TYPE = N_("Special");

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

    bool stageQualifiesAsColoured(const IShaderLayer::Ptr& layer)
    {
        auto red = layer->getColourExpression(IShaderLayer::COMP_RED);
        auto green = layer->getColourExpression(IShaderLayer::COMP_GREEN);
        auto blue = layer->getColourExpression(IShaderLayer::COMP_BLUE);
        auto alpha = layer->getColourExpression(IShaderLayer::COMP_ALPHA);

        return red && red->getExpressionString() == "parm0" &&
               green && green->getExpressionString() == "parm1" &&
               blue && blue->getExpressionString() == "parm2" &&
               alpha && alpha->getExpressionString() == "parm3";
    }
}

MaterialEditor::MaterialEditor() :
    DialogBase(DIALOG_TITLE),
    _treeView(nullptr),
    _stageList(new wxutil::TreeModel(STAGE_COLS(), true)),
    _stageView(nullptr),
    _stageUpdateInProgress(false),
    _materialUpdateInProgress(false)
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
    _preview.reset(new MaterialPreview(previewPanel));

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

    typeDropdown->Bind(wxEVT_CHOICE, &MaterialEditor::_onMaterialTypeChoice, this);

    auto* sortDropdown = getControl<wxComboBox>("MaterialSortValue");

    sortDropdown->AppendString(""); // empty string for undefined

    for (const auto& pair : shaders::PredefinedSortValues)
    {
        sortDropdown->AppendString(pair.first);
    }

    auto description = getControl<wxTextCtrl>("MaterialDescription");
    description->Bind(wxEVT_TEXT, [description, this](wxCommandEvent& ev)
    {
        if (_material && !_materialUpdateInProgress)
        {
            _material->setDescription(description->GetValue().ToStdString());
        }
    });
    
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
    },
        [=](const MaterialPtr& material, bool newValue)
    {
        if (newValue)
        {
            material->setMaterialFlag(flag);
        }
        else
        {
            material->clearMaterialFlag(flag);
        }
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
    setupMaterialFlag("MaterialNoSelfShadow", Material::FLAG_NOSELFSHADOW);
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
    setupSurfaceFlag("MaterialHasGuiSurf", Material::SurfaceFlags::SURF_GUISURF);
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
    _stageBindings.emplace(std::make_shared<CheckBoxBinding<IShaderLayer::Ptr>>(getControl<wxCheckBox>(controlName),
        [=](const IShaderLayer::Ptr& layer)
    {
        return (layer->getStageFlags() & flags) == flags;
    }));
}

void MaterialEditor::createExpressionBinding(const std::string& textCtrlName,
    const std::function<shaders::IShaderExpression::Ptr(const IShaderLayer::Ptr&)>& loadFunc,
    const std::function<void(const IEditableShaderLayer::Ptr&, const std::string&)>& saveFunc)
{
    _stageBindings.emplace(std::make_shared<ExpressionBinding>(
        getControl<wxTextCtrl>(textCtrlName),
        [loadFunc] (const IShaderLayer::Ptr& layer) 
        {
            auto expr = loadFunc(layer);
            return expr ? expr->getExpressionString() : std::string();
        },
        std::bind(&MaterialEditor::getEditableStageForSelection, this),
        saveFunc,
        std::bind(&MaterialEditor::onMaterialChanged, this)));
}

void MaterialEditor::createRadioButtonBinding(const std::string& ctrlName,
    const std::function<bool(const IShaderLayer::Ptr&)>& loadFunc,
    const std::function<void(const IEditableShaderLayer::Ptr&, bool)>& saveFunc)
{
    _stageBindings.emplace(std::make_shared<RadioButtonBinding>(
        getControl<wxRadioButton>(ctrlName),
        loadFunc,
        std::bind(&MaterialEditor::getEditableStageForSelection, this),
        saveFunc,
        std::bind(&MaterialEditor::onMaterialChanged, this)));
}

void MaterialEditor::createSpinCtrlBinding(const std::string& ctrlName,
    const std::function<int(const IShaderLayer::Ptr&)>& loadFunc,
    const std::function<void(const IEditableShaderLayer::Ptr&, int)>& saveFunc)
{
    _stageBindings.emplace(std::make_shared<SpinCtrlBinding<wxSpinCtrl>>(
        getControl<wxSpinCtrl>(ctrlName),
        loadFunc,
        std::bind(&MaterialEditor::getEditableStageForSelection, this),
        saveFunc,
        std::bind(&MaterialEditor::onMaterialChanged, this)));
}

void MaterialEditor::createSpinCtrlDoubleBinding(const std::string& ctrlName,
    const std::function<double(const IShaderLayer::Ptr&)>& loadFunc,
    const std::function<void(const IEditableShaderLayer::Ptr&, double)>& saveFunc)
{
    _stageBindings.emplace(std::make_shared<SpinCtrlBinding<wxSpinCtrlDouble>>(
        getControl<wxSpinCtrlDouble>(ctrlName),
        loadFunc,
        std::bind(&MaterialEditor::getEditableStageForSelection, this),
        saveFunc,
        std::bind(&MaterialEditor::onMaterialChanged, this)));
}

void MaterialEditor::setupMaterialStageProperties()
{
    setupStageFlag("MaterialStageFlagMaskRed", IShaderLayer::FLAG_MASK_RED);
    setupStageFlag("MaterialStageFlagMaskGreen", IShaderLayer::FLAG_MASK_GREEN);
    setupStageFlag("MaterialStageFlagMaskBlue", IShaderLayer::FLAG_MASK_BLUE);
    setupStageFlag("MaterialStageFlagMaskAlpha", IShaderLayer::FLAG_MASK_ALPHA);
    setupStageFlag("MaterialStageFlagMaskColour", IShaderLayer::FLAG_MASK_RED | IShaderLayer::FLAG_MASK_GREEN | IShaderLayer::FLAG_MASK_BLUE);
    setupStageFlag("MaterialStageFlagMaskDepth", IShaderLayer::FLAG_MASK_DEPTH);
    setupStageFlag("MaterialStageIgnoreAlphaTest", IShaderLayer::FLAG_IGNORE_ALPHATEST);

    _stageBindings.emplace(std::make_shared<CheckBoxBinding<IShaderLayer::Ptr>>(getControl<wxCheckBox>("MaterialStageHasAlphaTest"),
        [](const IShaderLayer::Ptr& layer)
    {
        return layer->hasAlphaTest();
    }));

    createExpressionBinding("MaterialStageAlphaTestExpression",
        [](const IShaderLayer::Ptr& layer) { return layer->getAlphaTestExpression(); },
        [this](const IEditableShaderLayer::Ptr& layer, const std::string& value) 
        { 
            layer->setAlphaTestExpressionFromString(value);
            getControl<wxCheckBox>("MaterialStageHasAlphaTest")->SetValue(layer->hasAlphaTest());
        });

    for (const auto& value : {
        "diffusemap", "bumpmap", "specularmap", "blend", "add", "filter", "modulate", "none", "Custom"
    })
    {
        getControl<wxChoice>("MaterialStageBlendType")->Append(value);
    }
   
   for (const auto& value : {
        "", "gl_one", "gl_zero", "gl_dst_color", "gl_one_minus_dst_color", "gl_src_alpha", 
        "gl_one_minus_src_alpha", "gl_dst_alpha", "gl_one_minus_dst_alpha", "gl_src_alpha_saturate"
    })
    {
        getControl<wxChoice>("MaterialStageBlendTypeSrc")->Append(value);
    }

    for (const auto& value : {
        "", "gl_one", "gl_zero", "gl_src_color", "gl_one_minus_src_color", "gl_src_alpha", 
        "gl_one_minus_src_alpha", "gl_dst_alpha", "gl_one_minus_dst_alpha"
    })
    {
        getControl<wxChoice>("MaterialStageBlendTypeDest")->Append(value);
    } 

    auto mapTypeDropdown = getControl<wxChoice>("MaterialStageMapType");

    for (const auto& value : { "map", "cubeMap", "cameraCubeMap" })
    {
        mapTypeDropdown->Append(value);
    }

    mapTypeDropdown->Append(_(SPECIAL_MAP_TYPE));
    mapTypeDropdown->Bind(wxEVT_CHOICE, &MaterialEditor::_onStageMapTypeChanged, this);

    auto mapTypeNotSpecial = getControl<wxRadioButton>("MaterialStageMapTypeNotSpecial");
    mapTypeNotSpecial->Bind(wxEVT_RADIOBUTTON, [this, mapTypeDropdown](wxCommandEvent& ev)
    {
        auto stage = getEditableStageForSelection();
        
        if (stage && ev.IsChecked())
        {
            stage->setMapType(IShaderLayer::MapType::Map);
            updateStageControls();
        }
    });

    createRadioButtonBinding("MaterialStageVideoMap",
        [](const IShaderLayer::Ptr& layer) { return layer->getMapType() == IShaderLayer::MapType::VideoMap; },
        [this](const IEditableShaderLayer::Ptr& layer, bool value)
        { 
            if (!value) return;
            layer->setMapType(IShaderLayer::MapType::VideoMap);
            updateStageControls();
        });
    createRadioButtonBinding("MaterialStageSoundMap",
        [](const IShaderLayer::Ptr& layer) { return layer->getMapType() == IShaderLayer::MapType::SoundMap; },
        [this](const IEditableShaderLayer::Ptr& layer, bool value)
    {
        if (!value) return;
        layer->setMapType(IShaderLayer::MapType::SoundMap);
        updateStageControls();
    });
    createRadioButtonBinding("MaterialStageRemoteRenderMap",
        [](const IShaderLayer::Ptr& layer) { return layer->getMapType() == IShaderLayer::MapType::RemoteRenderMap; },
        [this](const IEditableShaderLayer::Ptr& layer, bool value)
    {
        if (!value) return;
        layer->setMapType(IShaderLayer::MapType::RemoteRenderMap);
        updateStageControls();
    });
    createRadioButtonBinding("MaterialStageMirrorRenderMap",
        [](const IShaderLayer::Ptr& layer) { return layer->getMapType() == IShaderLayer::MapType::MirrorRenderMap; },
        [this](const IEditableShaderLayer::Ptr& layer, bool value)
    {
        if (!value) return;
        layer->setMapType(IShaderLayer::MapType::MirrorRenderMap);
        updateStageControls();
    }); 

    createSpinCtrlBinding("MaterialStageRemoteRenderMapWidth",
        [](const IShaderLayer::Ptr& layer) { return static_cast<int>(layer->getRenderMapSize().x()); },
        [this](const IEditableShaderLayer::Ptr& layer, const int& value)
    {
        auto currentSize = layer->getRenderMapSize();
        layer->setRenderMapSize(Vector2(value, currentSize.y()));
    });
    createSpinCtrlBinding("MaterialStageRemoteRenderMapHeight",
        [](const IShaderLayer::Ptr& layer) { return static_cast<int>(layer->getRenderMapSize().y()); },
        [this](const IEditableShaderLayer::Ptr& layer, const int& value)
    {
        auto currentSize = layer->getRenderMapSize();
        layer->setRenderMapSize(Vector2(currentSize.x(), value));
    });
    createSpinCtrlBinding("MaterialStageMirrorRenderMapWidth",
        [](const IShaderLayer::Ptr& layer) { return static_cast<int>(layer->getRenderMapSize().x()); },
        [this](const IEditableShaderLayer::Ptr& layer, const int& value)
    {
        auto currentSize = layer->getRenderMapSize();
        layer->setRenderMapSize(Vector2(value, currentSize.y()));
    });
    createSpinCtrlBinding("MaterialStageMirrorRenderMapHeight",
        [](const IShaderLayer::Ptr& layer) { return static_cast<int>(layer->getRenderMapSize().y()); },
        [this](const IEditableShaderLayer::Ptr& layer, const int& value)
    {
        auto currentSize = layer->getRenderMapSize();
        layer->setRenderMapSize(Vector2(currentSize.x(), value));
    });

    createSpinCtrlDoubleBinding("MaterialStagePrivatePolygonOffset",
        [](const IShaderLayer::Ptr& layer) { return layer->getPrivatePolygonOffset(); },
        [this](const IEditableShaderLayer::Ptr& layer, const double& value)
        {
            layer->setPrivatePolygonOffset(value);
        });

    // Texture
    setupStageFlag("MaterialStageFilterNearest", IShaderLayer::FLAG_FILTER_NEAREST);
    setupStageFlag("MaterialStageFilterLinear", IShaderLayer::FLAG_FILTER_LINEAR);
    setupStageFlag("MaterialStageHighQuality", IShaderLayer::FLAG_HIGHQUALITY);
    setupStageFlag("MaterialStageForceHighQuality", IShaderLayer::FLAG_FORCE_HIGHQUALITY);
    setupStageFlag("MaterialStageNoPicMip", IShaderLayer::FLAG_NO_PICMIP);
    setupStageFlag("MaterialStageIgnoreDepth", IShaderLayer::FLAG_IGNORE_DEPTH);

    auto texgenDropdown = getControl<wxChoice>("MaterialStageTexGenType");
    texgenDropdown->AppendString("");

    for (const auto& pair : shaders::TexGenTypeNames)
    {
        texgenDropdown->AppendString(pair.first);
    }

    texgenDropdown->Bind(wxEVT_CHOICE, [this, texgenDropdown](wxCommandEvent& ev)
    {
        auto stage = getEditableStageForSelection();

        if (stage)
        {
            stage->setTexGenType(shaders::getTexGenTypeForString(texgenDropdown->GetStringSelection().ToStdString()));
            updateStageTextureControls();
        }
    });

    createExpressionBinding("MaterialStageWobbleSkyX",
        [](const IShaderLayer::Ptr& layer) { return layer->getExpression(IShaderLayer::Expression::TexGenParam1); },
        [this](const IEditableShaderLayer::Ptr& layer, const std::string& value)
    {
        layer->setTexGenExpressionFromString(0, value);
    });
    createExpressionBinding("MaterialStageWobbleSkyY",
        [](const IShaderLayer::Ptr& layer) { return layer->getExpression(IShaderLayer::Expression::TexGenParam2); },
        [this](const IEditableShaderLayer::Ptr& layer, const std::string& value)
    {
        layer->setTexGenExpressionFromString(1, value);
    });
    createExpressionBinding("MaterialStageWobbleSkyZ",
        [](const IShaderLayer::Ptr& layer) { return layer->getExpression(IShaderLayer::Expression::TexGenParam3); },
        [this](const IEditableShaderLayer::Ptr& layer, const std::string& value)
    {
        layer->setTexGenExpressionFromString(2, value);
    });

    // Clamp Type
    auto clampDropdown = getControl<wxChoice>("MaterialStageClampType");
    for (const auto& pair : shaders::ClampTypeNames)
    {
        clampDropdown->AppendString(pair.first);
    }

    clampDropdown->Bind(wxEVT_CHOICE, [this, clampDropdown](wxCommandEvent& ev)
    {
        auto stage = getEditableStageForSelection();

        if (stage)
        {
            stage->setClampType(shaders::getClampTypeForString(clampDropdown->GetStringSelection().ToStdString()));
            onMaterialChanged();
        }
    });

    auto transformDropdown = getControl<wxChoice>("MaterialStageAddTransformChoice");
    for (const auto& pair : shaders::TransformTypeNames)
    {
        transformDropdown->AppendString(pair.first);
    }

    getControl<wxButton>("MaterialStageAddTransformButton")->Bind(wxEVT_BUTTON, &MaterialEditor::_onAddStageTransform, this);
    getControl<wxButton>("MaterialStageRemoveTransformButton")->Bind(wxEVT_BUTTON, &MaterialEditor::_onRemoveStageTransform, this);

    // Add the transformation listview
    auto transformationPanel = getControl<wxPanel>("MaterialStageTransformations");
    _stageTransformations = wxutil::TreeModel::Ptr(new wxutil::TreeModel(_stageTransformationColumns, true));

    _stageTransformView = wxutil::TreeView::CreateWithModel(transformationPanel, _stageTransformations.get(), wxDV_NO_HEADER);
    _stageTransformView->AppendTextColumn("Type", _stageTransformationColumns.type.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);
    _stageTransformView->AppendTextColumn("Index", _stageTransformationColumns.index.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);
    _stageTransformView->AppendTextColumn("Expr1", _stageTransformationColumns.expression1.getColumnIndex(),
        wxDATAVIEW_CELL_EDITABLE, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);
    _stageTransformView->AppendTextColumn("Expr2", _stageTransformationColumns.expression2.getColumnIndex(),
        wxDATAVIEW_CELL_EDITABLE, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

    _stageTransformView->Bind(wxEVT_DATAVIEW_ITEM_EDITING_DONE, &MaterialEditor::_onStageTransformEdited, this);
    _stageTransformView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, [this] (wxDataViewEvent&)
    {
        getControl<wxButton>("MaterialStageRemoveTransformButton")->Enable(_stageTransformView->GetSelection().IsOk());
    });

    transformationPanel->GetSizer()->Add(_stageTransformView, 1, wxEXPAND);

    createExpressionBinding("MaterialStageCondition",
        [](const IShaderLayer::Ptr& layer) { return layer->getConditionExpression(); },
        [](const IEditableShaderLayer::Ptr& layer, const std::string& value) { layer->setConditionExpressionFromString(value); });

    _stageBindings.emplace(std::make_shared<CheckBoxBinding<IShaderLayer::Ptr>>(getControl<wxCheckBox>("MaterialStageColored"),
        [](const IShaderLayer::Ptr& layer) { return stageQualifiesAsColoured(layer); }));

    getControl<wxCheckBox>("MaterialStageColored")->Bind(wxEVT_CHECKBOX, &MaterialEditor::_onStageColoredChecked, this);

    createRadioButtonBinding("MaterialStageNoVertexColourFlag",
        [](const IShaderLayer::Ptr& layer) { return layer->getVertexColourMode() == IShaderLayer::VERTEX_COLOUR_NONE; },
        [](const IEditableShaderLayer::Ptr& layer, bool value) { if (value) layer->setVertexColourMode(IShaderLayer::VERTEX_COLOUR_NONE); });
    createRadioButtonBinding("MaterialStageVertexColourFlag",
        [](const IShaderLayer::Ptr& layer) { return layer->getVertexColourMode() == IShaderLayer::VERTEX_COLOUR_MULTIPLY; },
        [](const IEditableShaderLayer::Ptr& layer, bool value) { if (value) layer->setVertexColourMode(IShaderLayer::VERTEX_COLOUR_MULTIPLY); });
    createRadioButtonBinding("MaterialStageInverseVertexColourFlag",
        [](const IShaderLayer::Ptr& layer) { return layer->getVertexColourMode() == IShaderLayer::VERTEX_COLOUR_INVERSE_MULTIPLY; },
        [](const IEditableShaderLayer::Ptr& layer, bool value) { if (value) layer->setVertexColourMode(IShaderLayer::VERTEX_COLOUR_INVERSE_MULTIPLY); });

    createExpressionBinding("MaterialStageRed",
        [](const IShaderLayer::Ptr& layer) { return layer->getColourExpression(IShaderLayer::COMP_RED); },
        [this](const IEditableShaderLayer::Ptr& layer, const std::string& value)
        { 
            layer->setColourExpressionFromString(IShaderLayer::COMP_RED, value);
            updateStageColoredStatus();
        });
    createExpressionBinding("MaterialStageGreen",
        [](const IShaderLayer::Ptr& layer) { return layer->getColourExpression(IShaderLayer::COMP_GREEN); },
        [this](const IEditableShaderLayer::Ptr& layer, const std::string& value) 
        { 
            layer->setColourExpressionFromString(IShaderLayer::COMP_GREEN, value);
            updateStageColoredStatus();
        });
    createExpressionBinding("MaterialStageBlue",
        [](const IShaderLayer::Ptr& layer) { return layer->getColourExpression(IShaderLayer::COMP_BLUE); },
        [this](const IEditableShaderLayer::Ptr& layer, const std::string& value)
        {
            layer->setColourExpressionFromString(IShaderLayer::COMP_BLUE, value);
            updateStageColoredStatus();
        });
    createExpressionBinding("MaterialStageAlpha",
        [](const IShaderLayer::Ptr& layer) { return layer->getColourExpression(IShaderLayer::COMP_ALPHA); },
        [this](const IEditableShaderLayer::Ptr& layer, const std::string& value)
        {
            layer->setColourExpressionFromString(IShaderLayer::COMP_ALPHA, value);
            updateStageColoredStatus();
        });

    auto parameterPanel = getControl<wxPanel>("MaterialStageProgramParameters");
    _stageProgramParameters = wxutil::TreeModel::Ptr(new wxutil::TreeModel(_stageProgramColumns, true));

    auto paramsView = wxutil::TreeView::CreateWithModel(parameterPanel, _stageProgramParameters.get(), wxDV_NO_HEADER);
    paramsView->AppendTextColumn("Index", _stageProgramColumns.index.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);
    paramsView->AppendTextColumn("Type", _stageProgramColumns.type.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);
    paramsView->AppendTextColumn("Expression", _stageProgramColumns.expression.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

    parameterPanel->GetSizer()->Add(paramsView, 1, wxEXPAND);
}

void MaterialEditor::updateStageColoredStatus()
{
    auto selectedStage = getSelectedStage();

    if (!selectedStage) return;

    bool stageIsColoured = stageQualifiesAsColoured(selectedStage);

    getControl<wxCheckBox>("MaterialStageColored")->SetValue(stageIsColoured);

    getControl<wxTextCtrl>("MaterialStageRed")->Enable(!stageIsColoured);
    getControl<wxTextCtrl>("MaterialStageGreen")->Enable(!stageIsColoured);
    getControl<wxTextCtrl>("MaterialStageBlue")->Enable(!stageIsColoured);
    getControl<wxTextCtrl>("MaterialStageAlpha")->Enable(!stageIsColoured);
}

void MaterialEditor::_onTreeViewSelectionChanged(wxDataViewEvent& ev)
{
    // Update the preview if a texture is selected
    if (!_treeView->IsDirectorySelected())
    {
        _material = GlobalMaterialManager().getMaterial(_treeView->GetSelectedFullname());
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
    util::ScopedBoolLock lock(_materialUpdateInProgress);

    _preview->setMaterial(_material);

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

inline std::string getNameForLayer(const IShaderLayer& layer)
{
    switch (layer.getType())
    {
    case IShaderLayer::DIFFUSE:
        return "diffuse";
    case IShaderLayer::BUMP:
        return "bump";
    case IShaderLayer::SPECULAR:
        return "specular";
    case IShaderLayer::BLEND:
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
        binding->updateFromSource();
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

        // guisurf
        auto guisurfArgument = _material->getGuiSurfArgument();
        getControl<wxTextCtrl>("MaterialGuiSurfPath")->SetValue(guisurfArgument);

        bool isEntityGui = (_material->getSurfaceFlags() & (Material::SURF_ENTITYGUI | Material::SURF_ENTITYGUI2 | Material::SURF_ENTITYGUI3)) != 0;

        getControl<wxTextCtrl>("MaterialGuiSurfPath")->Enable(!isEntityGui);
        getControl<wxPanel>("MaterialGuiSurfPanel")->Enable(_material->getSurfaceFlags() & Material::SURF_GUISURF);

        getControl<wxRadioButton>("MaterialGuiSurfRegular")->SetValue(!isEntityGui);
        getControl<wxRadioButton>("MaterialGuiSurfEntity")->SetValue(_material->getSurfaceFlags() & Material::SURF_ENTITYGUI);
        getControl<wxRadioButton>("MaterialGuiSurfEntity2")->SetValue(_material->getSurfaceFlags() & Material::SURF_ENTITYGUI2);
        getControl<wxRadioButton>("MaterialGuiSurfEntity3")->SetValue(_material->getSurfaceFlags() & Material::SURF_ENTITYGUI3);

        // Surround the definition with curly braces, these are not included
        auto definition = fmt::format("{0}\n{{{1}}}", _material->getName(), _material->getDefinition());
        _sourceView->SetValue(definition);
    }
    else
    {
        getControl<wxTextCtrl>("MaterialGuiSurfPath")->SetValue("");

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

IShaderLayer::Ptr MaterialEditor::getSelectedStage()
{
    auto selectedStageItem = _stageView->GetSelection();

    if (!selectedStageItem.IsOk() || !_material) return IShaderLayer::Ptr();

    const auto& layers = _material->getAllLayers();
    wxutil::TreeModel::Row stageRow(selectedStageItem, *_stageList);
    int stageIndex = stageRow[STAGE_COLS().index].getInteger();

    if (stageIndex >= 0 && stageIndex < layers.size())
    {
        return layers[stageIndex];
    }

    return IShaderLayer::Ptr();
}

IEditableShaderLayer::Ptr MaterialEditor::getEditableStageForSelection()
{
    auto selectedStageItem = _stageView->GetSelection();

    if (!selectedStageItem.IsOk() || !_material) return IEditableShaderLayer::Ptr();

    wxutil::TreeModel::Row stageRow(selectedStageItem, *_stageList);
    int stageIndex = stageRow[STAGE_COLS().index].getInteger();

    return _material->getEditableLayer(stageIndex);
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
        case IShaderLayer::DIFFUSE:
            blendTypeStrings.first = "diffusemap";
            blendTypeStrings.second.clear();
            break;
        case IShaderLayer::BUMP:
            blendTypeStrings.first = "bumpmap";
            blendTypeStrings.second.clear();
            break;
        case IShaderLayer::SPECULAR:
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

void MaterialEditor::updateStageTextureControls()
{
    auto selectedStage = getSelectedStage();

    auto texgenDropdown = getControl<wxChoice>("MaterialStageTexGenType");

    if (selectedStage)
    {
        // Clamp Type
        auto clampDropdown = getControl<wxChoice>("MaterialStageClampType");
        auto clampTypeString = shaders::getStringForClampType(selectedStage->getClampType());
        clampDropdown->SetStringSelection(clampTypeString);

        auto texgenName = shaders::getStringForTexGenType(selectedStage->getTexGenType());
        texgenDropdown->Select(texgenDropdown->FindString(texgenName));

        getControl<wxPanel>("MaterialStageWobblySkyPanel")->Show(selectedStage->getTexGenType() == IShaderLayer::TEXGEN_WOBBLESKY);
    }
    else
    {
        texgenDropdown->Select(0);
        getControl<wxPanel>("MaterialStageWobblySkyPanel")->Hide();
    }
}

void MaterialEditor::updateStageProgramControls()
{
    _stageProgramParameters->Clear();

    auto selectedStage = getSelectedStage();

    if (selectedStage)
    {
        getControl<wxTextCtrl>("MaterialStageVertexProgram")->SetValue(selectedStage->getVertexProgram());
        getControl<wxTextCtrl>("MaterialStageFragmentProgram")->SetValue(selectedStage->getFragmentProgram());

        _stageProgramParameters->Clear();

        for (int i = 0; i < selectedStage->getNumVertexParms(); ++i)
        {
            const auto& parm = selectedStage->getVertexParm(i);

            if (parm.index == -1) continue; // undefined param

            auto row = _stageProgramParameters->AddItem();

            row[_stageProgramColumns.type] = "vertexParm";
            row[_stageProgramColumns.index] = string::to_string(parm.index);

            std::string expression;

            for (int expr = 0; parm.expressions[expr] && expr < 4; ++expr)
            {
                expression += expression.empty() ? "" : " ";
                expression += parm.expressions[expr]->getExpressionString();
            }

            row[_stageProgramColumns.expression] = expression;

            row.SendItemAdded();
        }

        for (int i = 0; i < selectedStage->getNumFragmentMaps(); ++i)
        {
            const auto& parm = selectedStage->getFragmentMap(i);

            if (parm.index == -1) continue; // undefined param

            auto row = _stageProgramParameters->AddItem();

            row[_stageProgramColumns.type] = "fragmentMap";
            row[_stageProgramColumns.index] = string::to_string(parm.index);

            std::string expression = string::join(parm.options, " ");

            expression += expression.empty() ? "" : " ";
            expression += parm.map ? parm.map->getExpressionString() : "";

            row[_stageProgramColumns.expression] = expression;

            row.SendItemAdded();
        }
    }
    else
    {
        getControl<wxTextCtrl>("MaterialStageVertexProgram")->SetValue("");
        getControl<wxTextCtrl>("MaterialStageFragmentProgram")->SetValue("");
    }
}

void MaterialEditor::updateStageTransformControls()
{
    _stageTransformations->Clear();

    auto selectedStage = getSelectedStage();

    if (!selectedStage)
    {
        return;
    }

    getControl<wxButton>("MaterialStageRemoveTransformButton")->Enable(_stageTransformView->GetSelection().IsOk());

    const auto& transformations = selectedStage->getTransformations();

    for (int i = 0; i < transformations.size(); ++i)
    {
        const auto& transformation = transformations[i];

        auto row = _stageTransformations->AddItem();

        row[_stageTransformationColumns.type] = shaders::getStringForTransformType(transformation.type);
        row[_stageTransformationColumns.index] = string::to_string(i);

        row[_stageTransformationColumns.expression1] = transformation.expression1 ?
            transformation.expression1->getExpressionString() : std::string();
        row[_stageTransformationColumns.expression2] = transformation.expression2 ?
            transformation.expression2->getExpressionString() : std::string();

        row.SendItemAdded();
    }
}

void MaterialEditor::updateStageControls()
{
    util::ScopedBoolLock lock(_stageUpdateInProgress);

    auto selectedStage = getSelectedStage();

    // Update all registered bindings
    for (const auto& binding : _stageBindings)
    {
        binding->setSource(selectedStage);
        binding->updateFromSource();
    }

    getControl<wxPanel>("MaterialEditorStageSettingsPanel")->Enable(selectedStage != nullptr);

    updateStageBlendControls();
    updateStageTextureControls();
    updateStageProgramControls(); 
    updateStageTransformControls();
    updateStageColoredStatus();

    if (selectedStage)
    {
        selectedStage->evaluateExpressions(0); // initialise the values of this stage

        auto mapExpr = selectedStage->getMapExpression();
        auto imageMap = getControl<wxTextCtrl>("MaterialStageImageMap");
        imageMap->SetValue(mapExpr ? mapExpr->getExpressionString() : "");

        imageMap->Bind(wxEVT_TEXT, [imageMap, this](wxCommandEvent&)
        {
            if (_stageUpdateInProgress) return;

            auto stage = getEditableStageForSelection();
            stage->setMapExpressionFromString(imageMap->GetValue().ToStdString());
            onMaterialChanged();
        });

        auto mapTypeNotSpecial = getControl<wxRadioButton>("MaterialStageMapTypeNotSpecial");

        auto videoMapFile = getControl<wxTextCtrl>("MaterialStageVideoMapFile");
        auto videoMapLoop = getControl<wxCheckBox>("MaterialStageVideoMapLoop");
        auto soundMapWave = getControl<wxCheckBox>("MaterialStageSoundMapWaveform");

        auto remoteRenderMapWidth = getControl<wxSpinCtrl>("MaterialStageRemoteRenderMapWidth");
        auto remoteRenderMapHeight = getControl<wxSpinCtrl>("MaterialStageRemoteRenderMapHeight");

        auto mirrorRenderMapWidth = getControl<wxSpinCtrl>("MaterialStageMirrorRenderMapWidth");
        auto mirrorRenderMapHeight = getControl<wxSpinCtrl>("MaterialStageMirrorRenderMapHeight");

        auto mapType = getControl<wxChoice>("MaterialStageMapType");

        if (selectedStage->getMapType() == IShaderLayer::MapType::VideoMap ||
            selectedStage->getMapType() == IShaderLayer::MapType::SoundMap ||
            selectedStage->getMapType() == IShaderLayer::MapType::RemoteRenderMap ||
            selectedStage->getMapType() == IShaderLayer::MapType::MirrorRenderMap)
        {
            mapType->SetStringSelection(_(SPECIAL_MAP_TYPE));
            mapTypeNotSpecial->SetValue(false);
            imageMap->Disable();
        }
        else
        {
            mapTypeNotSpecial->SetValue(true);
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
        case IShaderLayer::MapType::Map:
            mapType->SetStringSelection("map");
            break;
        case IShaderLayer::MapType::CameraCubeMap:
            mapType->SetStringSelection("cameraCubeMap");
            break;
        case IShaderLayer::MapType::CubeMap:
            mapType->SetStringSelection("cubeMap");
            break;
        case IShaderLayer::MapType::VideoMap:
            {
                auto videoMapExpression = std::dynamic_pointer_cast<shaders::IVideoMapExpression>(selectedStage->getMapExpression());
                videoMapFile->SetValue(videoMapExpression ? videoMapExpression->getExpressionString() : "");
                videoMapLoop->SetValue(videoMapExpression ? videoMapExpression->isLooping() : false);
            }
            break;
        case IShaderLayer::MapType::SoundMap:
            {
                auto soundMapExpression = std::dynamic_pointer_cast<shaders::ISoundMapExpression>(selectedStage->getMapExpression());
                soundMapWave->SetValue(soundMapExpression ? soundMapExpression->isWaveform() : false);
            }
            break;
        case IShaderLayer::MapType::RemoteRenderMap:
            remoteRenderMapWidth->SetValue(static_cast<int>(selectedStage->getRenderMapSize().x()));
            remoteRenderMapHeight->SetValue(static_cast<int>(selectedStage->getRenderMapSize().y()));
            break;
        case IShaderLayer::MapType::MirrorRenderMap:
            mirrorRenderMapWidth->SetValue(static_cast<int>(selectedStage->getRenderMapSize().x()));
            mirrorRenderMapHeight->SetValue(static_cast<int>(selectedStage->getRenderMapSize().y()));
            break;
        }

        // Activate the controls next to the radio button
        getControl<wxPanel>("MaterialStageVideoMapPanel")->Enable(getControl<wxRadioButton>("MaterialStageVideoMap")->GetValue());
        getControl<wxCheckBox>("MaterialStageSoundMapWaveform")->Enable(getControl<wxRadioButton>("MaterialStageSoundMap")->GetValue());
        getControl<wxPanel>("MaterialStageRemoteRenderMapPanel")->Enable(getControl<wxRadioButton>("MaterialStageRemoteRenderMap")->GetValue());
        getControl<wxPanel>("MaterialStageMirrorRenderMapPanel")->Enable(getControl<wxRadioButton>("MaterialStageMirrorRenderMap")->GetValue());

        // Vertex Colours
        getControl<wxRadioButton>("MaterialStageNoVertexColourFlag")->SetValue(selectedStage->getVertexColourMode() == IShaderLayer::VERTEX_COLOUR_NONE);
        getControl<wxRadioButton>("MaterialStageVertexColourFlag")->SetValue(selectedStage->getVertexColourMode() == IShaderLayer::VERTEX_COLOUR_MULTIPLY);
        getControl<wxRadioButton>("MaterialStageInverseVertexColourFlag")->SetValue(selectedStage->getVertexColourMode() == IShaderLayer::VERTEX_COLOUR_INVERSE_MULTIPLY);
    }
    else
    {
        getControl<wxRadioButton>("MaterialStageNoVertexColourFlag")->SetValue(true);
        getControl<wxTextCtrl>("MaterialStageImageMap")->SetValue("");
        getControl<wxTextCtrl>("MaterialStageVideoMapFile")->SetValue("");
    }
}

void MaterialEditor::_onMaterialTypeChoice(wxCommandEvent& ev)
{
    if (!_material) return;
    
    auto selectedString = getControl<wxChoice>("MaterialType")->GetStringSelection();
    _material->setSurfaceType(shaders::getSurfaceTypeForString(selectedString.ToStdString()));
}

void MaterialEditor::_onAddStageTransform(wxCommandEvent& ev)
{
    auto selectedStage = getSelectedStage();

    if (selectedStage)
    {
        auto typeString = getControl<wxChoice>("MaterialStageAddTransformChoice")->GetStringSelection();
        auto type = shaders::getTransformTypeForString(typeString.ToStdString());

        selectedStage->appendTransformation(IShaderLayer::Transformation{ type });

        updateStageControls();
    }
}

void MaterialEditor::_onRemoveStageTransform(wxCommandEvent& ev)
{
    auto stage = getEditableStageForSelection();

    if (stage && _stageTransformView->GetSelection().IsOk())
    {
        wxutil::TreeModel::Row row(_stageTransformView->GetSelection(), *_stageTransformations);

        auto index = row[_stageTransformationColumns.index].getInteger();
        stage->removeTransformation(index);

        updateStageControls();
    }
}

void MaterialEditor::_onStageTransformEdited(wxDataViewEvent& ev)
{
    if (ev.IsEditCancelled()) return;

    auto stage = getEditableStageForSelection();

    if (!stage) return;

    wxutil::TreeModel::Row row(ev.GetItem(), *_stageTransformations);

    // The iter points to the edited cell now, get the actor number
    int transformIndex = row[_stageTransformationColumns.index].getInteger();
    std::string transformTypeString = row[_stageTransformationColumns.type];

    if (transformIndex < 0 || transformIndex > stage->getTransformations().size())
    {
        return;
    }

    std::string expression1, expression2;
    auto type = shaders::getTransformTypeForString(transformTypeString);

#if wxCHECK_VERSION(3, 1, 0)
    // wx 3.1+ delivers the new value through the event
    if (ev.GetColumn() == _stageTransformationColumns.expression1.getColumnIndex())
    {
        expression1 = ev.GetValue().GetString().ToStdString();
        expression2 = row[_stageTransformationColumns.expression2];
    }
    else if (ev.GetColumn() == _stageTransformationColumns.expression2.getColumnIndex())
    {
        expression1 = row[_stageTransformationColumns.expression1];
        expression2 = ev.GetValue().GetString().ToStdString();
    }
#else
    // wx 3.0.x has the values set in the model
    expression1 = row[_stageTransformationColumns.expression1];
    expression2 = row[_stageTransformationColumns.expression2];
#endif

    stage->updateTransformation(transformIndex, type, expression1, expression2);
}

void MaterialEditor::_onStageColoredChecked(wxCommandEvent& ev)
{
    auto selectedStage = getEditableStageForSelection();
    if (!selectedStage) return;

    bool stageIsCurrentlyColoured = stageQualifiesAsColoured(selectedStage);

    if (ev.IsChecked() && !stageIsCurrentlyColoured)
    {
        selectedStage->setColourExpressionFromString(IShaderLayer::COMP_RED, "parm0");
        selectedStage->setColourExpressionFromString(IShaderLayer::COMP_GREEN, "parm1");
        selectedStage->setColourExpressionFromString(IShaderLayer::COMP_BLUE, "parm2");
        selectedStage->setColourExpressionFromString(IShaderLayer::COMP_ALPHA, "parm3");
    }
    else if (!ev.IsChecked() && stageIsCurrentlyColoured)
    {
        selectedStage->setColourExpressionFromString(IShaderLayer::COMP_RED, "");
        selectedStage->setColourExpressionFromString(IShaderLayer::COMP_GREEN, "");
        selectedStage->setColourExpressionFromString(IShaderLayer::COMP_BLUE, "");
        selectedStage->setColourExpressionFromString(IShaderLayer::COMP_ALPHA, "");
    }

    updateStageControls();
}

void MaterialEditor::_onStageMapTypeChanged(wxCommandEvent& ev)
{
    auto stage = getEditableStageForSelection();
    if (!stage) return;

    auto mapTypeString = getControl<wxChoice>("MaterialStageMapType")->GetStringSelection();

    if (mapTypeString == _(SPECIAL_MAP_TYPE))
    {
        stage->setMapType(IShaderLayer::MapType::MirrorRenderMap);

        // Switch pages
        auto notebook = getControl<wxNotebook>("MaterialStageSettingsNotebook");
        notebook->SetSelection(notebook->FindPage(getControl<wxPanel>("SpecialMapPanel")));
    }
    else
    {
        stage->setMapType(shaders::getMapTypeForString(mapTypeString.ToStdString()));
    }

    updateStageControls();
}

void MaterialEditor::onMaterialChanged()
{
    _preview->onMaterialChanged();
}

}
