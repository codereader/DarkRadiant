#include "MaterialEditor.h"

#include "i18n.h"
#include "ishaderexpression.h"
#include "ishaderclipboard.h"

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
#include <wx/collpane.h>
#include <wx/statbmp.h>
#include <wx/clrpicker.h>

#include "wxutil/SourceView.h"
#include "wxutil/FileChooser.h"
#include "wxutil/dialog/MessageBox.h"
#include "wxutil/dataview/ResourceTreeViewToolbar.h"
#include "wxutil/dataview/TreeViewItemStyle.h"
#include "wxutil/EntityClassChooser.h"
#include "wxutil/Bitmap.h"
#include "materials/FrobStageSetup.h"
#include <fmt/format.h>
#include "gamelib.h"
#include "string/join.h"
#include "materials/ParseLib.h"
#include "ExpressionBinding.h"
#include "RadioButtonBinding.h"
#include "SpinCtrlBinding.h"
#include "CheckBoxBinding.h"
#include "MapExpressionEntry.h"
#include "TexturePreview.h"

namespace ui
{

namespace
{
    const char* const DIALOG_TITLE = N_("Material Editor");

    const char* const ICON_STAGE_VISIBLE = "visible.png";
    const char* const ICON_STAGE_INVISIBLE = "invisible.png";
    const char* const ICON_GLOBAL_SETTINGS = "icon_texture.png";

    const std::string RKEY_ROOT = "user/ui/materialEditor/";
    const std::string RKEY_SPLIT_POS = RKEY_ROOT + "splitPos";
    const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";

    const char* const CUSTOM_BLEND_TYPE = N_("Custom");

    // Columns for the stages list
    struct StageColumns :
        public wxutil::TreeModel::ColumnRecord
    {
        StageColumns() :
            enabled(add(wxutil::TreeModel::Column::Boolean)),
            icon(add(wxutil::TreeModel::Column::Icon)),
            name(add(wxutil::TreeModel::Column::String)),
            index(add(wxutil::TreeModel::Column::Integer)),
            visible(add(wxutil::TreeModel::Column::Boolean)),
            global(add(wxutil::TreeModel::Column::Boolean))
        {}

        wxutil::TreeModel::Column enabled;
        wxutil::TreeModel::Column icon;
        wxutil::TreeModel::Column name;
        wxutil::TreeModel::Column index;
        wxutil::TreeModel::Column visible;
        wxutil::TreeModel::Column global;
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

    IShaderLayer::Type determineStageTypeToCreate(const MaterialPtr& material)
    {
        bool hasDiffuse = false;
        bool hasBump = false;
        bool hasSpecular = false;

        for (const auto& layer : material->getAllLayers())
        {
            hasDiffuse |= layer->getType() == IShaderLayer::DIFFUSE;
            hasBump |= layer->getType() == IShaderLayer::BUMP;
            hasSpecular |= layer->getType() == IShaderLayer::SPECULAR;
        }

        return !hasDiffuse ? IShaderLayer::DIFFUSE :
               !hasBump ? IShaderLayer::BUMP :
               !hasSpecular ? IShaderLayer::SPECULAR : 
               IShaderLayer::BLEND;
    }
}

MaterialEditor::MaterialEditor() :
    DialogBase(DIALOG_TITLE),
    _treeView(nullptr),
    _stageList(new wxutil::TreeModel(STAGE_COLS(), true)),
    _stageView(nullptr),
    _stageUpdateInProgress(false),
    _materialUpdateInProgress(false),
    _lightUpdateInProgress(false)
{
    loadNamedPanel(this, "MaterialEditorMainPanel");

    makeLabelBold(this, "MaterialEditorDefinitionLabel");
    makeLabelBold(this, "MaterialEditorStagePropertiesLabel");
    makeLabelBold(this, "MaterialEditorMaterialStagesLabel");

    // Wire up the buttons
    getControl<wxButton>("MaterialEditorCloseButton")->Bind(wxEVT_BUTTON, &MaterialEditor::_onClose, this);
    getControl<wxButton>("MaterialEditorReloadImagesButton")->Bind(wxEVT_BUTTON, &MaterialEditor::_onReloadImages, this);

    // Add the treeview
    setupMaterialTreeView();

    // Setup the splitter and preview
    auto* splitter = getControl<wxSplitterWindow>("MaterialEditorSplitter");
    splitter->SetSashPosition(GetSize().GetWidth() * 0.6f);
    splitter->SetMinimumPaneSize(10); // disallow unsplitting

    // Set up the preview
    auto* previewPanel = getControl<wxPanel>("MaterialEditorPreviewPanel");
    _preview.reset(new MaterialPreview(previewPanel));

    previewPanel->GetSizer()->Add(_preview->getWidget(), 1, wxEXPAND);
    setupSourceTextPanel(previewPanel);
    setupPreviewLightProperties(previewPanel);

    setupBasicMaterialPage();
    setupMaterialProperties();
    setupMaterialStageView();
    setupMaterialStageProperties();
    setupMaterialSurfaceFlags();
    setupMaterialShaderFlags();
    setupMaterialLightFlags();
    setupMaterialDeformPage();

    // Set the default size of the window
    FitToScreen(0.8f, 0.9f);

    Layout();
    Fit();

    // Connect the window position tracker
    _windowPosition.loadFromPath(RKEY_WINDOW_STATE);
    _windowPosition.connect(this);
    _windowPosition.applyPosition();

    _panedPosition.connect(splitter);
    _panedPosition.loadFromPath(RKEY_SPLIT_POS);

    CenterOnParent();

    updateControlsFromMaterial();
}

MaterialEditor::~MaterialEditor()
{
    _materialChanged.disconnect();
    _materialBindings.clear();
    _stageBindings.clear();

    auto notebook = getControl<wxNotebook>("MaterialStageSettingsNotebook");

    // Remove all umapped pages to avoid memory leaks
    for (const auto& pair : _notebookPages)
    {
        if (notebook->FindPage(pair.second.first) == -1)
        {
            pair.second.first->Destroy();
        }
    }

    _notebookPages.clear();
}

int MaterialEditor::ShowModal()
{
    // Restore the position
    _windowPosition.applyPosition();

    _treeView->Populate();

    if (!_materialToPreselect.empty())
    {
        _treeView->SetSelectedFullname(_materialToPreselect);
    }

    int returnCode = DialogBase::ShowModal();

    // Tell the position tracker to save the information
    _windowPosition.saveToPath(RKEY_WINDOW_STATE);

    return returnCode;
}

int MaterialEditor::ShowModal(const std::string& materialToPreselect)
{
    _materialToPreselect = materialToPreselect;

    // If we don't get any input from the arguments, use the
    // currently selected material for starters
    if (_materialToPreselect.empty())
    {
        _materialToPreselect = GlobalShaderClipboard().getShaderName();
    }

    return ShowModal();
}

void MaterialEditor::setupSourceTextPanel(wxWindow* previewPanel)
{
    auto sourceTextPanel = new wxCollapsiblePane(previewPanel, wxID_ANY, _("Material Source Text"));
    _sourceView = new wxutil::D3MaterialSourceViewCtrl(sourceTextPanel->GetPane());
    _sourceView->SetMinSize(wxSize(-1, 400));

    sourceTextPanel->Bind(wxEVT_COLLAPSIBLEPANE_CHANGED, [=](wxCollapsiblePaneEvent& ev)
    {
        previewPanel->Layout();
    });

    auto paneSizer = new wxBoxSizer(wxVERTICAL);
    paneSizer->Add(_sourceView, 1, wxGROW | wxEXPAND);
    sourceTextPanel->GetPane()->SetSizer(paneSizer);
    sourceTextPanel->Collapse();

    previewPanel->GetSizer()->Add(sourceTextPanel, 0, wxEXPAND);
}

void MaterialEditor::setupPreviewLightProperties(wxWindow* previewPanel)
{
    auto collapsiblePane = new wxCollapsiblePane(previewPanel, wxID_ANY, _("Light Properties"));
    
    auto propertyPanel = getControl<wxPanel>("MaterialPreviewLightPanel");
    propertyPanel->GetContainingSizer()->Detach(propertyPanel);
    propertyPanel->Reparent(collapsiblePane->GetPane());
    propertyPanel->SetMinSize(wxSize(-1, 100));

    collapsiblePane->Bind(wxEVT_COLLAPSIBLEPANE_CHANGED, [=](wxCollapsiblePaneEvent& ev)
    {
        previewPanel->Layout();
    });

    auto paneSizer = new wxBoxSizer(wxVERTICAL);
    paneSizer->Add(propertyPanel, 1, wxGROW | wxEXPAND);
    collapsiblePane->GetPane()->SetSizer(paneSizer);
    collapsiblePane->Collapse();

    previewPanel->GetSizer()->Add(collapsiblePane, 0, wxEXPAND);

    // Wire up the signals
    _preview->signal_LightChanged().connect([this] ()
    {
        util::ScopedBoolLock lock(_lightUpdateInProgress);

        getControl<wxTextCtrl>("MaterialPreviewLightClassname")->SetValue(_preview->getLightClassname());

        auto colour = _preview->getLightColour() * 255.0;
        getControl<wxColourPickerCtrl>("MaterialPreviewLightColour")->SetColour(
            wxColour(static_cast<wxColour::ChannelType>(colour.x()), 
                     static_cast<wxColour::ChannelType>(colour.y()), 
                     static_cast<wxColour::ChannelType>(colour.z())));
    });

    getControl<wxTextCtrl>("MaterialPreviewLightClassname")->Bind(wxEVT_TEXT, [this](wxCommandEvent& ev)
    {
        if (_lightUpdateInProgress) return;
        _preview->setLightClassname(ev.GetString().ToStdString());
    });

#if defined(__WXMSW__) && wxCHECK_VERSION(3,1,3)
    const auto& colourEvtType = wxEVT_COLOURPICKER_CURRENT_CHANGED;
#else
    const auto& colourEvtType = wxEVT_COLOURPICKER_CHANGED;
#endif
    getControl<wxColourPickerCtrl>("MaterialPreviewLightColour")->Bind(colourEvtType,
    [this](wxColourPickerEvent& ev)
    {
        if (_lightUpdateInProgress) return;
        auto colour = ev.GetColour();
        _preview->setLightColour(Vector3(colour.Red() / 255.0, colour.Green() / 255.0, colour.Blue() / 255.0));
    });

    getControl<wxButton>("MaterialPreviewLightChooseClassnameButton")->Bind(wxEVT_BUTTON, [this](wxCommandEvent& ev)
    {
        auto textCtrl = getControl<wxTextCtrl>("MaterialPreviewLightClassname");
        auto newClassName = wxutil::EntityClassChooser::ChooseEntityClass(
            wxutil::EntityClassChooser::Purpose::SelectClassname, textCtrl->GetValue().ToStdString());
        textCtrl->SetValue(newClassName);
    });
    
    getControl<wxButton>("MaterialPreviewLightResetColourButton")->Bind(wxEVT_BUTTON, [this](wxCommandEvent& ev)
    {
        _preview->resetLightColour();
    });
}

void MaterialEditor::_onReloadImages(wxCommandEvent& ev)
{
    if (!_material) return;

    _material->refreshImageMaps();
}

void MaterialEditor::_onClose(wxCommandEvent& ev)
{
    // Check all unsaved materials
    std::list<MaterialPtr> modifiedMaterials;
    GlobalMaterialManager().foreachMaterial([&](const MaterialPtr& material)
    {
        if (material->isModified())
        {
            modifiedMaterials.push_back(material);
        }
    });

    for (const auto& material : modifiedMaterials)
    {
        selectMaterial(material);

        // Prompt user to save or discard
        if (!askUserAboutModifiedMaterial())
        {
            return; // cancel the close event
        }
    }

    // At this point, everything is saved
    EndModal(wxID_CLOSE);
}

void MaterialEditor::ShowDialog(const cmd::ArgumentList& args)
{
    auto* editor = new MaterialEditor;

    std::string materialToPreselect;

    if (!args.empty())
    {
        materialToPreselect = args[0].getString();
    }

    editor->ShowModal(materialToPreselect);
    editor->Destroy();
}

void MaterialEditor::setupMaterialTreeView()
{
    auto* panel = getControl<wxPanel>("MaterialEditorTreeView");
    _treeView = new MaterialTreeView(panel);
    _treeView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &MaterialEditor::_onMaterialSelectionChanged, this);

    auto* treeToolbar = new wxutil::ResourceTreeViewToolbar(panel, _treeView);
    treeToolbar->EnableFavouriteManagement(false);

    auto definitionLabel = getControl<wxStaticText>("MaterialEditorDefinitionLabel");
    definitionLabel->GetContainingSizer()->Detach(definitionLabel);
    definitionLabel->Reparent(treeToolbar);
    treeToolbar->GetLeftSizer()->Add(definitionLabel, 0, wxALIGN_LEFT);

    panel->GetSizer()->Add(treeToolbar, 0, wxEXPAND | wxBOTTOM, 6);
    panel->GetSizer()->Add(_treeView, 1, wxEXPAND);

    auto newButton = getControl<wxButton>("MaterialEditorNewDefButton");
    newButton->Bind(wxEVT_BUTTON, &MaterialEditor::_onNewMaterial, this);
    
    auto saveButton = getControl<wxButton>("MaterialEditorSaveDefButton");
    saveButton->Disable();
    saveButton->Bind(wxEVT_BUTTON, &MaterialEditor::_onSaveMaterial, this);

    auto copyButton = getControl<wxButton>("MaterialEditorCopyDefButton");
    copyButton->Disable();
    copyButton->Bind(wxEVT_BUTTON, &MaterialEditor::_onCopyMaterial, this);

    auto revertButton = getControl<wxButton>("MaterialEditorRevertButton");
    revertButton->Disable();
    revertButton->Bind(wxEVT_BUTTON, &MaterialEditor::_onRevertMaterial, this);

    auto unlockButton = getControl<wxButton>("MaterialEditorUnlockButton");
    unlockButton->Disable();
    unlockButton->Bind(wxEVT_BUTTON, &MaterialEditor::_onUnlockMaterial, this);
    unlockButton->Bind(wxEVT_ENTER_WINDOW, [unlockButton](wxMouseEvent&)
    {
        if (!unlockButton->IsEnabled()) { return; }
        unlockButton->SetForegroundColour(wxNullColour);
    });
    unlockButton->Bind(wxEVT_LEAVE_WINDOW, [unlockButton](wxMouseEvent&)
    {
        if (!unlockButton->IsEnabled()) { return; }
        unlockButton->SetForegroundColour(*wxWHITE);
    });
}

void MaterialEditor::_onBasicMapEntryChanged(const std::string& entryName, IShaderLayer::Type type)
{
    if (!_material || _materialUpdateInProgress)
    {
        return;
    }

    auto textCtrl = getControl<MapExpressionEntry>(entryName)->GetTextCtrl();
    
    auto newValue = textCtrl->GetValue().ToStdString();

    // Clearing the text control is the same as removing the stage
    if (newValue.empty())
    {
        // Remove the corresponding stage
        auto stageToRemove = findMaterialStageByType(type);

        if (stageToRemove.first)
        {
            _material->removeLayer(stageToRemove.second);
            updateStageListFromMaterial(); 
            onMaterialChanged();
            updateBasicImagePreview();
            updateBasicFrobStageControls();
        }

        return;
    }

    // Non-empty text, create the stage if necessary
    auto existingStage = findMaterialStageByType(type);
    IEditableShaderLayer::Ptr stage;
    bool stageCreated = false;

    if (existingStage.first)
    {
        stage = _material->getEditableLayer(existingStage.second);
    }

    if (!stage)
    {
        auto index = _material->addLayer(type);

        stage = _material->getEditableLayer(index);
        stageCreated = true;
    }

    stage->setMapExpressionFromString(textCtrl->GetValue().ToStdString());
    
    if (stageCreated)
    {
        updateStageListFromMaterial();
    }

    updateStageControls();
    updateBasicImagePreview();
    updateBasicFrobStageControls();
    onMaterialChanged();
}

void MaterialEditor::setupBasicMaterialPage()
{
    convertTextCtrlToMapExpressionEntry("BasicEditorImageEntry");
    convertTextCtrlToMapExpressionEntry("BasicDiffuseImageEntry");
    convertTextCtrlToMapExpressionEntry("BasicBumpImageEntry");
    convertTextCtrlToMapExpressionEntry("BasicSpecularImageEntry");

    auto nameEntry = getControl<wxTextCtrl>("BasicName");
    nameEntry->Bind(wxEVT_TEXT, &MaterialEditor::_onMaterialNameChanged, this);

    auto editorImageEntry = getControl<MapExpressionEntry>("BasicEditorImageEntry")->GetTextCtrl();
    editorImageEntry->Bind(wxEVT_TEXT, [this, editorImageEntry](wxCommandEvent&)
    { 
        if (!_material || _materialUpdateInProgress) return;
        
        _material->setEditorImageExpressionFromString(editorImageEntry->GetValue().ToStdString());
        updateStageBlendControls();
        updateBasicImagePreview();
        updateBasicFrobStageControls();
        onMaterialChanged();
    });

    auto description = getControl<wxTextCtrl>("BasicDescription");
    description->Bind(wxEVT_TEXT, [description, this](wxCommandEvent& ev)
    {
        if (_material && !_materialUpdateInProgress)
        {
            _material->setDescription(description->GetValue().ToStdString());
            updateMaterialPropertiesFromMaterial();
            onMaterialChanged();
        }
    });

    auto* typeDropdown = getControl<wxChoice>("BasicMaterialType");

    typeDropdown->AppendString(""); // empty string for undefined

    for (const auto& pair : shaders::SurfaceTypeMapping)
    {
        typeDropdown->AppendString(pair.first);
    }

    typeDropdown->Bind(wxEVT_CHOICE, &MaterialEditor::_onMaterialTypeChoice, this);

    auto diffuseMapEntry = getControl<MapExpressionEntry>("BasicDiffuseImageEntry")->GetTextCtrl();
    diffuseMapEntry->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { _onBasicMapEntryChanged("BasicDiffuseImageEntry", IShaderLayer::DIFFUSE); });
    auto bumpMapEntry = getControl<MapExpressionEntry>("BasicBumpImageEntry")->GetTextCtrl();
    bumpMapEntry->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { _onBasicMapEntryChanged("BasicBumpImageEntry", IShaderLayer::BUMP); });
    auto specularMapEntry = getControl<MapExpressionEntry>("BasicSpecularImageEntry")->GetTextCtrl();
    specularMapEntry->Bind(wxEVT_TEXT, [this](wxCommandEvent&) { _onBasicMapEntryChanged("BasicSpecularImageEntry", IShaderLayer::SPECULAR); });

    auto editorImgTabImage = getControl<wxStaticBitmap>("BasicEditorImageTabImage");
    replaceControl(editorImgTabImage, new TexturePreview(editorImgTabImage->GetParent(), TexturePreview::ImageType::EditorImage));

    auto diffuseTabImage = getControl<wxStaticBitmap>("BasicDiffuseTabImage");
    replaceControl(diffuseTabImage, new TexturePreview(diffuseTabImage->GetParent(), TexturePreview::ImageType::Diffuse));

    auto bumpTabImage = getControl<wxStaticBitmap>("BasicBumpTabImage");
    replaceControl(bumpTabImage, new TexturePreview(bumpTabImage->GetParent(), TexturePreview::ImageType::Bump));

    auto specularTabImage = getControl<wxStaticBitmap>("BasicSpecularTabImage");
    replaceControl(specularTabImage, new TexturePreview(specularTabImage->GetParent(), TexturePreview::ImageType::Specular));

    auto addFrob = getControl<wxButton>("BasicAddFrobStages");
    auto removeFrob = getControl<wxButton>("BasicRemoveFrobStages");
    auto testFrob = getControl<wxButton>("BasicTestFrobStages");

    addFrob->Bind(wxEVT_BUTTON, &MaterialEditor::_onBasicAddFrobStages, this);
    removeFrob->Bind(wxEVT_BUTTON, &MaterialEditor::_onBasicRemoveFrobStages, this);

    testFrob->Bind(wxEVT_LEFT_DOWN, &MaterialEditor::_onBasicTestFrobStages, this);
    testFrob->Bind(wxEVT_LEFT_UP, &MaterialEditor::_onBasicTestFrobStages, this);
}

void MaterialEditor::setupMaterialProperties()
{
    // Convert int-valued spinctrls to double-valued ones
    convertToSpinCtrlDouble(this, "MaterialPolygonOffsetValue", -100, 100, 0.1, 1);
    convertToSpinCtrlDouble(this, "MaterialEditorDecalInfoStaySeconds", 0, 999999, 0.1, 2);
    convertToSpinCtrlDouble(this, "MaterialEditorDecalInfoFadeSeconds", 0, 999999, 0.1, 2);
    convertToSpinCtrlDouble(this, "MaterialStagePrivatePolygonOffset", -100, 100, 0.1, 1);

    // Place map expression controls where needed
    convertTextCtrlToMapExpressionEntry("MaterialStageImageMap");
    convertTextCtrlToMapExpressionEntry("MaterialLightFalloffMap");
    convertTextCtrlToMapExpressionEntry("MaterialEditorImage");

    auto nameEntry = getControl<wxTextCtrl>("MaterialName");
    nameEntry->Bind(wxEVT_TEXT, &MaterialEditor::_onMaterialNameChanged, this);

    auto editorImage = getControl<MapExpressionEntry>("MaterialEditorImage");
    _materialBindings.emplace(std::make_shared<ExpressionBinding<MaterialPtr>>(editorImage->GetTextCtrl(),
        [](const MaterialPtr& material)
        {
            auto expr = material->getEditorImageExpression();
            return expr ? expr->getExpressionString() : std::string();
        },
        [this](const MaterialPtr& material, const std::string& value)
        {
            if (_materialUpdateInProgress || !_material) return;
            material->setEditorImageExpressionFromString(value);
        },
        [this]() { onMaterialChanged(); }));

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

    sortDropdown->Bind(wxEVT_COMBOBOX, &MaterialEditor::_onSortRequestChanged, this);
    sortDropdown->Bind(wxEVT_TEXT, &MaterialEditor::_onSortRequestChanged, this);

    auto description = getControl<wxTextCtrl>("MaterialDescription");
    description->Bind(wxEVT_TEXT, [description, this](wxCommandEvent& ev)
    {
        if (_material && !_materialUpdateInProgress)
        {
            _material->setDescription(description->GetValue().ToStdString());
            updateBasicPageFromMaterial();
            onMaterialChanged();
        }
    });
    
    _materialBindings.emplace(std::make_shared<CheckBoxBinding<MaterialPtr>>(getControl<wxCheckBox>("MaterialHasSortValue"),
        [](const MaterialPtr& material)
        { 
            return (material->getMaterialFlags() & Material::FLAG_HAS_SORT_DEFINED) != 0;
        },
        [this](const MaterialPtr& material, const bool& value)
        {
            if (!value)
            {
                material->resetSortRequest();
            }
            else
            {
                auto sortValue = getControl<wxComboBox>("MaterialSortValue")->GetValue().ToStdString();
                material->setSortRequest(!sortValue.empty() ? 
                    shaders::getSortRequestValueForString(sortValue) : 
                    static_cast<float>(Material::SORT_OPAQUE));
            }
        },
        [this]() { onMaterialChanged(); }));

    _materialBindings.emplace(std::make_shared<SpinCtrlBinding<wxSpinCtrlDouble, MaterialPtr>>(
        getControl<wxSpinCtrlDouble>("MaterialPolygonOffsetValue"),
        [](const MaterialPtr& material) { return material->getPolygonOffset(); },
        [this](const MaterialPtr& material, const double& value)
        {
            material->setPolygonOffset(value);
            getControl<wxCheckBox>("MaterialFlagHasPolygonOffset")->SetValue((material->getMaterialFlags() & Material::FLAG_POLYGONOFFSET) != 0);
        },
        [this]() { onMaterialChanged(); }));

    getControl<wxCheckBox>("MaterialHasSpectrum")->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& ev)
    {
        getControl<wxSpinCtrl>("MaterialSpectrumValue")->Enable(ev.IsChecked());
    });

    // For light fall off images, only cameracubemap and map are allowed
    auto lightFallOffCubeMapType = getControl<wxChoice>("MaterialLightFalloffCubeMapType");
    lightFallOffCubeMapType->AppendString(shaders::getStringForMapType(IShaderLayer::MapType::Map));
    lightFallOffCubeMapType->AppendString(shaders::getStringForMapType(IShaderLayer::MapType::CameraCubeMap));

    lightFallOffCubeMapType->Bind(wxEVT_CHOICE, [this, lightFallOffCubeMapType] (wxCommandEvent& ev)
    {
        if (_materialUpdateInProgress || !_material) return;
        _material->setLightFalloffCubeMapType(shaders::getMapTypeForString(lightFallOffCubeMapType->GetStringSelection().ToStdString()));
    });

    auto lightFalloffMap = getControl<MapExpressionEntry>("MaterialLightFalloffMap");
    _materialBindings.emplace(std::make_shared<ExpressionBinding<MaterialPtr>>(lightFalloffMap->GetTextCtrl(),
        [](const MaterialPtr& material) 
        { 
            auto expr = material->getLightFalloffExpression();
            return expr ? expr->getExpressionString() : std::string(); 
        },
        [this](const MaterialPtr& material, const std::string& value)
        {
            if (_materialUpdateInProgress || !_material) return;
            material->setLightFalloffExpressionFromString(value);
        },
        [this]() { onMaterialChanged(); }));

    _materialBindings.emplace(std::make_shared<SpinCtrlBinding<wxSpinCtrl, MaterialPtr>>(getControl<wxSpinCtrl>("MaterialSpectrumValue"),
        [](const MaterialPtr& material) { return material->getSpectrum(); },
        [this](const MaterialPtr& material, const int& value)
        {
            if (_materialUpdateInProgress || !_material) return;
            material->setSpectrum(value);
        },
        [this]() { onMaterialChanged(); }));
}

void MaterialEditor::setupSurfaceFlag(const std::string& controlName, Material::SurfaceFlags flag)
{
    _materialBindings.emplace(std::make_shared<CheckBoxBinding<MaterialPtr>>(getControl<wxCheckBox>(controlName),
        [=](const MaterialPtr& material)
        {
            return (material->getSurfaceFlags() & flag) != 0;
        },
        [this, flag](const MaterialPtr& material, const bool& value)
        {
            if (_materialUpdateInProgress || !_material) return;

            if (value)
            {
                _material->setSurfaceFlag(flag);
            }
            else
            {
                _material->clearSurfaceFlag(flag);
            }
        },
        [this]() { onMaterialChanged(); }));
}

void MaterialEditor::setupMaterialFlag(const std::string& controlName, Material::Flags flag)
{
    _materialBindings.emplace(std::make_shared<CheckBoxBinding<MaterialPtr>>(getControl<wxCheckBox>(controlName),
    [=](const MaterialPtr& material)
    {
        return (material->getMaterialFlags() & flag) != 0;
    },
    [=](const MaterialPtr& material, const bool& newValue)
    {
        if (newValue)
        {
            material->setMaterialFlag(flag);
        }
        else
        {
            material->clearMaterialFlag(flag);
        }
    },
    [this]() // post-update
    { 
        onMaterialChanged();
        updateMaterialPropertiesFromMaterial();
    }));
}

void MaterialEditor::setupMaterialLightFlags()
{
    _materialBindings.emplace(std::make_shared<CheckBoxBinding<MaterialPtr>>(getControl<wxCheckBox>("MaterialIsAmbientLight"),
        [](const MaterialPtr& material) { return material->isAmbientLight(); },
        [=](const MaterialPtr& material, const bool& newValue)
        {
            material->setIsAmbientLight(newValue);
        },
        [this]() // post-update
        {
            onMaterialChanged();
            updateMaterialPropertiesFromMaterial();
        }));

    _materialBindings.emplace(std::make_shared<CheckBoxBinding<MaterialPtr>>(getControl<wxCheckBox>("MaterialIsAmbientCubicLight"),
        [](const MaterialPtr& material) { return material->isAmbientLight() && material->isCubicLight(); },
        [=](const MaterialPtr& material, const bool& newValue)
        {
            material->setIsAmbientLight(newValue);
            material->setIsCubicLight(newValue);
        },
        [this]() // post-update
        {
            onMaterialChanged();
            updateMaterialPropertiesFromMaterial();
        }));

    _materialBindings.emplace(std::make_shared<CheckBoxBinding<MaterialPtr>>(getControl<wxCheckBox>("MaterialIsFogLight"),
        [](const MaterialPtr& material) { return material->isFogLight(); },
        [=](const MaterialPtr& material, const bool& newValue)
        {
            material->setIsFogLight(newValue);
        },
        [this]() // post-update
        {
            onMaterialChanged();
            updateMaterialPropertiesFromMaterial();
        }));

    _materialBindings.emplace(std::make_shared<CheckBoxBinding<MaterialPtr>>(getControl<wxCheckBox>("MaterialIsCubicLight"),
        [](const MaterialPtr& material) { return material->isCubicLight(); },
        [=](const MaterialPtr& material, const bool& newValue)
        {
            material->setIsCubicLight(newValue);
        },
        [this]() // post-update
        {
            onMaterialChanged();
            updateMaterialPropertiesFromMaterial();
        }));
    
    _materialBindings.emplace(std::make_shared<CheckBoxBinding<MaterialPtr>>(getControl<wxCheckBox>("MaterialIsBlendLight"),
        [](const MaterialPtr& material) { return material->isBlendLight(); },
        [=](const MaterialPtr& material, const bool& newValue)
        {
            material->setIsBlendLight(newValue);
        },
        [this]() // post-update
        {
            onMaterialChanged();
            updateMaterialPropertiesFromMaterial();
        }));
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
    auto cullTypes = getControl<wxChoice>("MaterialCullType");
    
    for (const auto& pair : shaders::CullTypes)
    {
        cullTypes->AppendString(pair.first);
    }

    cullTypes->Bind(wxEVT_CHOICE, [this, cullTypes](wxCommandEvent& ev)
    {
        if (!_material || _materialUpdateInProgress) return;
        _material->setCullType(shaders::getCullTypeForString(cullTypes->GetStringSelection().ToStdString()));
        onMaterialChanged();
    });

    // Global Clamping
    auto clampDropdown = getControl<wxChoice>("MaterialClampType");
    for (const auto& pair : shaders::ClampTypeNames)
    {
        clampDropdown->AppendString(pair.first);
    }

    clampDropdown->Bind(wxEVT_CHOICE, [this, clampDropdown](wxCommandEvent& ev)
    {
        if (!_material || _materialUpdateInProgress) return;

        if (_material)
        {
            _material->setClampType(shaders::getClampTypeForString(clampDropdown->GetStringSelection().ToStdString()));
            onMaterialChanged();
        }
    });

    // DECAL_MACRO
    _materialBindings.emplace(std::make_shared<CheckBoxBinding<MaterialPtr>>(getControl<wxCheckBox>("MaterialHasDecalMacro"),
        [](const MaterialPtr& material) { return material->getParseFlags() & Material::PF_HasDecalMacro; },
        [](const MaterialPtr& material, const bool& newValue) {}));

    // TWOSIDED_DECAL_MACRO
    _materialBindings.emplace(std::make_shared<CheckBoxBinding<MaterialPtr>>(getControl<wxCheckBox>("MaterialHasTwoSidedDecalMacro"),
        [](const MaterialPtr& material) { return material->getParseFlags() & Material::PF_HasTwoSidedDecalMacro; },
        [](const MaterialPtr& material, const bool& newValue) {}));

    // GLASS_MACRO
    _materialBindings.emplace(std::make_shared<CheckBoxBinding<MaterialPtr>>(getControl<wxCheckBox>("MaterialHasGlassMacro"),
        [](const MaterialPtr& material) { return material->getParseFlags() & Material::PF_HasGlassMacro; },
        [](const MaterialPtr& material, const bool& newValue) {}));

    // PARTICLE_MACRO
    _materialBindings.emplace(std::make_shared<CheckBoxBinding<MaterialPtr>>(getControl<wxCheckBox>("MaterialHasParticleMacro"),
        [](const MaterialPtr& material) { return material->getParseFlags() & Material::PF_HasParticleMacro; },
        [](const MaterialPtr& material, const bool& newValue) {}));
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
    _iconVisible = wxutil::GetLocalBitmap(ICON_STAGE_VISIBLE);
    _iconInvisible = wxutil::GetLocalBitmap(ICON_STAGE_INVISIBLE);

    // Stage view
    auto* panel = getControl<wxPanel>("MaterialEditorStageView");

    _stageView = wxutil::TreeView::CreateWithModel(panel, _stageList.get(), wxDV_NO_HEADER);
    panel->GetSizer()->Add(_stageView, 1, wxEXPAND);

    _stageView->AppendBitmapColumn(_("Enabled"), STAGE_COLS().icon.getColumnIndex(),
        wxDATAVIEW_CELL_ACTIVATABLE, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);
    _stageView->AppendTextColumn(_("Stage"), STAGE_COLS().name.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

    _stageView->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &MaterialEditor::_onStageListSelectionChanged, this);
    _stageView->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &MaterialEditor::_onStageListItemActivated, this);

    getControl<wxButton>("MaterialEditorAddStageButton")->Bind(wxEVT_BUTTON, &MaterialEditor::_onAddStage, this);
    getControl<wxButton>("MaterialEditorRemoveStageButton")->Bind(wxEVT_BUTTON, &MaterialEditor::_onRemoveStage, this);
    getControl<wxButton>("MaterialEditorToggleStageButton")->Bind(wxEVT_BUTTON, &MaterialEditor::_onToggleStage, this);
    getControl<wxButton>("MaterialEditorMoveUpStageButton")->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { moveStagePosition(-1); });
    getControl<wxButton>("MaterialEditorMoveDownStageButton")->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { moveStagePosition(+1); });
    getControl<wxButton>("MaterialEditorDuplicateStageButton")->Bind(wxEVT_BUTTON, &MaterialEditor::_onDuplicateStage, this);

    auto row = _stageList->AddItem();

    row[STAGE_COLS().global] = true;
    row[STAGE_COLS().index] = -1;
    row[STAGE_COLS().icon] = wxVariant(wxutil::GetLocalBitmap(ICON_GLOBAL_SETTINGS));
    row[STAGE_COLS().name] = _("Global Settings");
    row[STAGE_COLS().enabled] = true;

    wxDataViewItemAttr globalStyle;
    globalStyle.SetColour(wxColor(15, 15, 15));
    globalStyle.SetBold(true);
    row[STAGE_COLS().name] = globalStyle;

    row.SendItemAdded();

    auto notebook = getControl<wxNotebook>("MaterialStageSettingsNotebook");
    for (int i = 0; i < notebook->GetPageCount(); ++i)
    {
        auto page = notebook->GetPage(i);
        _notebookPages.emplace(i, std::make_pair(page, notebook->GetPageText(i)));
    }
}

void MaterialEditor::setupStageFlag(const std::string& controlName, IShaderLayer::Flags flag)
{
    _stageBindings.emplace(std::make_shared<CheckBoxBinding<IShaderLayer::Ptr>>(getControl<wxCheckBox>(controlName),
        [=](const IShaderLayer::Ptr& layer)
        {
            return (layer->getStageFlags() & flag) == flag;
        },
        [=](const IEditableShaderLayer::Ptr& layer, const bool& value)
        {
            if (value)
            {
                layer->setStageFlag(flag);
            }
            else
            {
                layer->clearStageFlag(flag);
            }
        },
        std::bind(&MaterialEditor::onMaterialChanged, this),
        std::bind(&MaterialEditor::getEditableStageForSelection, this)));
}

void MaterialEditor::createExpressionBinding(const std::string& textCtrlName,
    const std::function<shaders::IShaderExpression::Ptr(const IShaderLayer::Ptr&)>& loadFunc,
    const std::function<void(const IEditableShaderLayer::Ptr&, const std::string&)>& saveFunc)
{
    _stageBindings.emplace(std::make_shared<ExpressionBinding<IShaderLayer::Ptr>>(
        getControl<wxTextCtrl>(textCtrlName),
        [loadFunc] (const IShaderLayer::Ptr& layer) 
        {
            auto expr = loadFunc(layer);
            return expr ? expr->getExpressionString() : std::string();
        },
        saveFunc,
        std::bind(&MaterialEditor::onMaterialChanged, this),
        std::bind(&MaterialEditor::getEditableStageForSelection, this)));
}

void MaterialEditor::createRadioButtonBinding(const std::string& ctrlName,
    const std::function<bool(const IShaderLayer::Ptr&)>& loadFunc,
    const std::function<void(const IEditableShaderLayer::Ptr&, bool)>& saveFunc)
{
    _stageBindings.emplace(std::make_shared<RadioButtonBinding<IShaderLayer::Ptr>>(
        getControl<wxRadioButton>(ctrlName),
        loadFunc,
        saveFunc,
        std::bind(&MaterialEditor::onMaterialChanged, this),
        std::bind(&MaterialEditor::getEditableStageForSelection, this)));
}

void MaterialEditor::createSpinCtrlBinding(const std::string& ctrlName,
    const std::function<int(const IShaderLayer::Ptr&)>& loadFunc,
    const std::function<void(const IEditableShaderLayer::Ptr&, int)>& saveFunc)
{
    _stageBindings.emplace(std::make_shared<SpinCtrlBinding<wxSpinCtrl, IShaderLayer::Ptr>>(
        getControl<wxSpinCtrl>(ctrlName),
        loadFunc,
        saveFunc,
        std::bind(&MaterialEditor::onMaterialChanged, this),
        std::bind(&MaterialEditor::getEditableStageForSelection, this)));
}

void MaterialEditor::createSpinCtrlDoubleBinding(const std::string& ctrlName,
    const std::function<double(const IShaderLayer::Ptr&)>& loadFunc,
    const std::function<void(const IEditableShaderLayer::Ptr&, double)>& saveFunc)
{
    _stageBindings.emplace(std::make_shared<SpinCtrlBinding<wxSpinCtrlDouble, IShaderLayer::Ptr>>(
        getControl<wxSpinCtrlDouble>(ctrlName),
        loadFunc,
        saveFunc,
        std::bind(&MaterialEditor::onMaterialChanged, this),
        std::bind(&MaterialEditor::getEditableStageForSelection, this)));
}

void MaterialEditor::setupMaterialStageProperties()
{
    auto stageImageMap = getControl<MapExpressionEntry>("MaterialStageImageMap");
    _stageBindings.emplace(std::make_shared<ExpressionBinding<IShaderLayer::Ptr>>(stageImageMap->GetTextCtrl(),
        [](const IShaderLayer::Ptr& stage)
        {
            auto expr = stage->getMapExpression();
            return expr ? expr->getExpressionString() : std::string();
        },
        [this](const IEditableShaderLayer::Ptr& stage, const std::string& value)
        {
            if (_stageUpdateInProgress) return;
            stage->setMapExpressionFromString(value);
        },
        [this]() { onMaterialChanged(); updateBasicPageFromMaterial(); },
        std::bind(&MaterialEditor::getEditableStageForSelection, this)));

    setupStageFlag("MaterialStageFlagMaskRed", IShaderLayer::FLAG_MASK_RED);
    setupStageFlag("MaterialStageFlagMaskGreen", IShaderLayer::FLAG_MASK_GREEN);
    setupStageFlag("MaterialStageFlagMaskBlue", IShaderLayer::FLAG_MASK_BLUE);
    setupStageFlag("MaterialStageFlagMaskAlpha", IShaderLayer::FLAG_MASK_ALPHA);

    _stageBindings.emplace(std::make_shared<CheckBoxBinding<IShaderLayer::Ptr>>(getControl<wxCheckBox>("MaterialStageFlagMaskColour"),
        [=](const IShaderLayer::Ptr& layer)
        {
            auto colourFlags = IShaderLayer::FLAG_MASK_RED | IShaderLayer::FLAG_MASK_GREEN | IShaderLayer::FLAG_MASK_BLUE;
            return (layer->getStageFlags() & colourFlags) == colourFlags;
        },
        [=](const IEditableShaderLayer::Ptr& layer, const bool& value)
        {
            if (value)
            {
                layer->setStageFlag(IShaderLayer::FLAG_MASK_RED);
                layer->setStageFlag(IShaderLayer::FLAG_MASK_GREEN);
                layer->setStageFlag(IShaderLayer::FLAG_MASK_BLUE);
            }
            else
            {
                layer->clearStageFlag(IShaderLayer::FLAG_MASK_RED);
                layer->clearStageFlag(IShaderLayer::FLAG_MASK_GREEN);
                layer->clearStageFlag(IShaderLayer::FLAG_MASK_BLUE);
            }
        },
        std::bind(&MaterialEditor::onMaterialChanged, this),
        std::bind(&MaterialEditor::getEditableStageForSelection, this)));

    setupStageFlag("MaterialStageFlagMaskDepth", IShaderLayer::FLAG_MASK_DEPTH);
    setupStageFlag("MaterialStageIgnoreAlphaTest", IShaderLayer::FLAG_IGNORE_ALPHATEST);

    createExpressionBinding("MaterialStageAlphaTestExpression",
        [](const IShaderLayer::Ptr& layer) { return layer->getAlphaTestExpression(); },
        [this](const IEditableShaderLayer::Ptr& layer, const std::string& value) 
        { 
            layer->setAlphaTestExpressionFromString(value);
        });

    for (const auto& value : { "diffusemap", "bumpmap", "specularmap", "blend", "add", "filter", "modulate", "none" })
    {
        getControl<wxChoice>("MaterialStageBlendType")->Append(value);
    }

    getControl<wxChoice>("MaterialStageBlendType")->Append(_(CUSTOM_BLEND_TYPE));
   
   for (const auto& value : {
        "gl_one", "gl_zero", "gl_dst_color", "gl_one_minus_dst_color", "gl_src_alpha", 
        "gl_one_minus_src_alpha", "gl_dst_alpha", "gl_one_minus_dst_alpha", "gl_src_alpha_saturate"
    })
    {
        getControl<wxChoice>("MaterialStageBlendTypeSrc")->Append(value);
    }

    for (const auto& value : {
        "gl_one", "gl_zero", "gl_src_color", "gl_one_minus_src_color", "gl_src_alpha", 
        "gl_one_minus_src_alpha", "gl_dst_alpha", "gl_one_minus_dst_alpha"
    })
    {
        getControl<wxChoice>("MaterialStageBlendTypeDest")->Append(value);
    } 

    getControl<wxChoice>("MaterialStageBlendType")->Bind(wxEVT_CHOICE, &MaterialEditor::_onStageBlendTypeChanged, this);;
    getControl<wxChoice>("MaterialStageBlendTypeSrc")->Bind(wxEVT_CHOICE, &MaterialEditor::_onStageBlendTypeChanged, this);;
    getControl<wxChoice>("MaterialStageBlendTypeDest")->Bind(wxEVT_CHOICE, &MaterialEditor::_onStageBlendTypeChanged, this);;

    auto mapTypeDropdown = getControl<wxChoice>("MaterialStageMapType");

    for (const auto& value : shaders::MapTypeNames)
    {
        mapTypeDropdown->Append(value.first);
    }

    mapTypeDropdown->Bind(wxEVT_CHOICE, &MaterialEditor::_onStageMapTypeChanged, this);

    createSpinCtrlBinding("MaterialStageRenderMapWidth",
        [](const IShaderLayer::Ptr& layer) { return static_cast<int>(layer->getRenderMapSize().x()); },
        [this](const IEditableShaderLayer::Ptr& layer, const int& value)
    {
        auto currentSize = layer->getRenderMapSize();
        layer->setRenderMapSize(Vector2(value, currentSize.y()));
    });
    createSpinCtrlBinding("MaterialStageRenderMapHeight",
        [](const IShaderLayer::Ptr& layer) { return static_cast<int>(layer->getRenderMapSize().y()); },
        [this](const IEditableShaderLayer::Ptr& layer, const int& value)
    {
        auto currentSize = layer->getRenderMapSize();
        layer->setRenderMapSize(Vector2(currentSize.x(), value));
    });

    getControl<wxCheckBox>("MaterialStageSoundMapWaveform")->Bind(wxEVT_CHECKBOX, [this] (wxCommandEvent& ev)
    {
        if (this->_stageUpdateInProgress) return;
        auto stage = getEditableStageForSelection();
        if (!stage) return;

        stage->setSoundMapWaveForm(ev.IsChecked());
    });

    getControl<wxCheckBox>("MaterialStageVideoMapLoop")->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& ev)
    {
        if (this->_stageUpdateInProgress) return;
        auto stage = getEditableStageForSelection();
        if (!stage) return;

        auto filePath = getControl<MapExpressionEntry>("MaterialStageImageMap")->GetValue().ToStdString();
        stage->setVideoMapProperties(filePath, ev.IsChecked());
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

    _stageTransformView = wxutil::TreeView::CreateWithModel(transformationPanel, _stageTransformations.get());
    _stageTransformView->AppendTextColumn(_("#"), _stageTransformationColumns.index.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);
    _stageTransformView->AppendTextColumn(_("Type"), _stageTransformationColumns.type.getColumnIndex(),
        wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);
    _stageTransformView->AppendTextColumn(_("Expression 1"), _stageTransformationColumns.expression1.getColumnIndex(),
        wxDATAVIEW_CELL_EDITABLE, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);
    _stageTransformView->AppendTextColumn(_("Expression 2"), _stageTransformationColumns.expression2.getColumnIndex(),
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
        [](const IShaderLayer::Ptr& layer) { return stageQualifiesAsColoured(layer); },
        [this](const IEditableShaderLayer::Ptr& layer, const bool& value) { _onStageColoredChecked(layer, value); },
        std::bind(&MaterialEditor::onMaterialChanged, this),
        std::bind(&MaterialEditor::getEditableStageForSelection, this)));

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

bool MaterialEditor::saveCurrentMaterial()
{
    if (!_material->isModified())
    {
        return true;
    }

    if (_material->getShaderFileInfo().fullPath().empty())
    {
        auto mtrExtension = shaders::getMaterialFileExtension();
        auto materialsFolderName = shaders::getMaterialsFolderName();
        
        while (true)
        {
            // Ask the user where to save it
            wxutil::FileChooser chooser(this, _("Select Material file"), false, "material", "." + mtrExtension);

            fs::path modMaterialsPath = GlobalGameManager().getModPath();
            modMaterialsPath /= materialsFolderName;

            if (!os::fileOrDirExists(modMaterialsPath.string()))
            {
                rMessage() << "Ensuring mod materials path: " << modMaterialsPath << std::endl;
                fs::create_directories(modMaterialsPath);
            }

            // Point the file chooser to that new file
            chooser.setCurrentPath(GlobalGameManager().getModPath() + materialsFolderName);
            chooser.askForOverwrite(false);

            std::string result = chooser.display();

            if (result.empty())
            {
                return false; // save aborted
            }

            try
            {
                // Setting the path might fail if it's invalid, try to set it and break the loop
                _material->setShaderFileName(os::standardPath(result));
                break;
            }
            catch (const std::invalid_argument& ex)
            {
                // Invalid path, notify user and get ready for the next round
                wxutil::Messagebox::ShowError(ex.what(), this);
            }
        }
    }

    try
    {
        // Write to the specified .mtr file
        GlobalMaterialManager().saveMaterial(_material->getName());

        _material = GlobalMaterialManager().getMaterial(_material->getName());
        selectMaterial(_material);
    }
    catch (const std::runtime_error& ex)
    {
        rError() << "Could not save file: " << ex.what() << std::endl;
        wxutil::Messagebox::ShowError(ex.what(), this);

        return false; // failure means to abort the process
    }

    updateMaterialTreeItem();
    updateMaterialControlSensitivity();

    return true;
}

void MaterialEditor::revertCurrentMaterial()
{
    if (!_material) return;

    if (_material->isModified() && _material->getShaderFileInfo().name.empty())
    {
        // This material has been created and not been saved yet, 
        // discarding it means removing it
        GlobalMaterialManager().removeMaterial(_material->getName());
        selectMaterial(MaterialPtr());
        return;
    }

    _material->revertModifications();

    onMaterialChanged();
}

bool MaterialEditor::askUserAboutModifiedMaterial()
{
    // Get the original name
    std::string origName = _material->getName();

    // Does not make sense to save a null material
    assert(!origName.empty());

    // The material we're editing has been changed from the saved one
    wxutil::Messagebox box(_("Save Changes"),
        fmt::format(_("Do you want to save the changes to the material\n{0}?"), origName),
        IDialog::MESSAGE_SAVECONFIRMATION, this);

    auto result = box.run();

    if (result == IDialog::RESULT_YES)
    {
        // User wants to save, return true if save was successful
        return saveCurrentMaterial();
    }
    else if (result == IDialog::RESULT_NO)
    {
        // Discard changes
        revertCurrentMaterial();
        return true;
    }
    else if (result == IDialog::RESULT_CANCELLED)
    {
        return false; // user cancelled
    }

    // User doesn't want to save
    return true;
}

bool MaterialEditor::isAllowedToChangeMaterial()
{
    return true;
}

void MaterialEditor::handleMaterialSelectionChange()
{
    _selectedMaterialItem = _treeView->GetSelection();

    _materialChanged.disconnect();

    // Update the preview if a texture is selected
    if (_selectedMaterialItem.IsOk() && !_treeView->IsDirectorySelected())
    {
        _material = GlobalMaterialManager().getMaterial(_treeView->GetSelectedFullname());

        _materialChanged = _material->sig_materialChanged().connect([this]()
        {
            updateSourceView();
            updateMaterialTreeItem();
        });
    }
    else
    {
        _material.reset();
    }

    updateControlsFromMaterial();
}

void MaterialEditor::_onMaterialNameChanged(wxCommandEvent& ev)
{
    if (_materialUpdateInProgress || !_material) return;

    auto nameEntry = static_cast<wxTextCtrl*>(ev.GetEventObject());

    GlobalMaterialManager().renameMaterial(_material->getName(), nameEntry->GetValue().ToStdString());
    auto item = _treeView->GetTreeModel()->FindString(_material->getName(), _treeView->Columns().fullName);
    _treeView->EnsureVisible(item);
 
    updateMaterialPropertiesFromMaterial();
    onMaterialChanged();
}

void MaterialEditor::_onMaterialSelectionChanged(wxDataViewEvent& ev)
{
    // Check if the material has been modified and ask for save
    if (!isAllowedToChangeMaterial())
    {
        // Revert the selection and cancel the operation
        _treeView->Select(_selectedMaterialItem);
        return;
    }

    handleMaterialSelectionChange();
}

void MaterialEditor::_onSaveMaterial(wxCommandEvent& ev)
{
    if (!_material) return;

    saveCurrentMaterial();
}

void MaterialEditor::selectMaterial(const MaterialPtr& material)
{
    if (!material)
    {
        _treeView->UnselectAll();
        handleMaterialSelectionChange();
        return;
    }

    auto newItem = _treeView->GetTreeModel()->FindString(material->getName(), _treeView->Columns().fullName);

    if (newItem.IsOk())
    {
        _treeView->Select(newItem);
        _treeView->EnsureVisible(newItem);

        handleMaterialSelectionChange();
        updateMaterialTreeItem();
    }
}

void MaterialEditor::_onNewMaterial(wxCommandEvent& ev)
{
    auto materialName = "textures/darkmod/map_specific/unnamed";
    auto newMaterial = GlobalMaterialManager().createEmptyMaterial(materialName);

    selectMaterial(newMaterial);
}

void MaterialEditor::copySelectedMaterial()
{
    if (!_material) return;

    auto newMaterialName = _material->getName() + _("_copy");
    auto newMaterial = GlobalMaterialManager().copyMaterial(_material->getName(), newMaterialName);

    selectMaterial(newMaterial);
}

void MaterialEditor::_onCopyMaterial(wxCommandEvent& ev)
{
    copySelectedMaterial();
}

void MaterialEditor::_onRevertMaterial(wxCommandEvent& ev)
{
    if (!_material) return;

    // The material we're editing has been changed from the saved one
    wxutil::Messagebox box(_("Discard Changes"),
        fmt::format(_("Do you want to discard all the changes to the material\n{0}?"), _material->getName()),
        IDialog::MESSAGE_ASK, this);

    if (box.run() == IDialog::RESULT_YES)
    {
        revertCurrentMaterial();
        updateControlsFromMaterial();
    }
}

void MaterialEditor::_onUnlockMaterial(wxCommandEvent& ev)
{
    copySelectedMaterial();
}

void MaterialEditor::_onStageListSelectionChanged(wxDataViewEvent& ev)
{
    auto item = _stageView->GetSelection();

    if (!_material || !item.IsOk()) return;
    auto row = wxutil::TreeModel::Row(item, *_stageList);

    updateSettingsNotebook();
    updateStageControls();
    updateStageButtonSensitivity();
}

void MaterialEditor::updateSettingsNotebook()
{
    auto item = _stageView->GetSelection();

    bool isGlobal = true;

    if (_material && item.IsOk())
    {
        auto row = wxutil::TreeModel::Row(item, *_stageList);
        isGlobal = row[STAGE_COLS().global].getBool();
    }

    auto notebook = getControl<wxNotebook>("MaterialStageSettingsNotebook");
    auto desiredPrefix = isGlobal ? "Material" : "Stage";

    // Remove unwanted pages
    for (int i = notebook->GetPageCount() - 1; i >= 0; --i)
    {
        if (!notebook->GetPage(i)->GetName().StartsWith(desiredPrefix))
        {
            notebook->RemovePage(i);
        }
    }

    // Add missing pages
    for (const auto& pair : _notebookPages)
    {
        auto page = pair.second.first;
        if (page->GetName().StartsWith(desiredPrefix) && notebook->FindPage(page) == -1)
        {
            notebook->AddPage(page, pair.second.second);
        }
    }

    // Update settings title
    getControl<wxStaticText>("MaterialEditorStagePropertiesLabel")->SetLabel(isGlobal ?
        _("Global Material Settings") : _("Stage Settings"));
}

void MaterialEditor::updateStageButtonSensitivity()
{
    auto item = _stageView->GetSelection();

    bool materialCanBeModified = _material && GlobalMaterialManager().materialCanBeModified(_material->getName());

    if (materialCanBeModified && item.IsOk())
    {
        auto row = wxutil::TreeModel::Row(item, *_stageList);
        auto index = row[STAGE_COLS().index].getInteger();
        auto isGlobalStage = row[STAGE_COLS().global].getBool();
        auto layersCount = _material->getAllLayers().size();

        getControl<wxButton>("MaterialEditorRemoveStageButton")->Enable(!isGlobalStage);
        getControl<wxButton>("MaterialEditorToggleStageButton")->Enable(!isGlobalStage);
        getControl<wxButton>("MaterialEditorMoveUpStageButton")->Enable(!isGlobalStage && index > 0);
        getControl<wxButton>("MaterialEditorMoveDownStageButton")->Enable(!isGlobalStage && index + 1 < layersCount);
        getControl<wxButton>("MaterialEditorDuplicateStageButton")->Enable(!isGlobalStage);
        getControl<wxButton>("MaterialEditorAddStageButton")->Enable();
    }
    else
    {
        getControl<wxButton>("MaterialEditorRemoveStageButton")->Disable();
        getControl<wxButton>("MaterialEditorToggleStageButton")->Disable();
        getControl<wxButton>("MaterialEditorMoveUpStageButton")->Disable();
        getControl<wxButton>("MaterialEditorMoveDownStageButton")->Disable();
        getControl<wxButton>("MaterialEditorDuplicateStageButton")->Disable();
        getControl<wxButton>("MaterialEditorAddStageButton")->Enable(materialCanBeModified);
    }
}

void MaterialEditor::_onStageListItemActivated(wxDataViewEvent& ev)
{
    toggleSelectedStage();
}

void MaterialEditor::updateMaterialControlSensitivity()
{
    getControl<wxButton>("MaterialEditorNewDefButton")->Enable(true);

    auto canBeModified = _material && GlobalMaterialManager().materialCanBeModified(_material->getName());

    getControl<wxButton>("MaterialEditorSaveDefButton")->Enable(_material && 
        _material->isModified() && canBeModified);

    getControl<wxButton>("MaterialEditorCopyDefButton")->Enable(_material != nullptr);
    getControl<wxButton>("MaterialEditorRevertButton")->Enable(_material && _material->isModified());

    auto unlockButton = getControl<wxButton>("MaterialEditorUnlockButton");
    unlockButton->Show(_material != nullptr);
    unlockButton->Enable(_material && !canBeModified);
    unlockButton->SetBackgroundColour(unlockButton->IsEnabled() ? wxColour(255, 70, 70) : wxNullColour);
    unlockButton->SetLabelText(unlockButton->IsEnabled() ? _("Unlock Editing") : _("Is editable"));
    unlockButton->SetForegroundColour(unlockButton->IsEnabled() ? *wxWHITE : wxNullColour);

    getControl<wxButton>("MaterialEditorReloadImagesButton")->Enable(_material != nullptr);
}

std::pair<IShaderLayer::Ptr, std::size_t> MaterialEditor::findMaterialStageByType(IShaderLayer::Type type)
{
    if (!_material) return std::pair<IShaderLayer::Ptr, std::size_t>();

    const auto& layers = _material->getAllLayers();
    for (auto i = 0; i < layers.size(); ++i)
    {
        if (layers[i]->getType() == type)
        {
            return std::make_pair(layers[i], i);
        }
    }

    return std::pair<IShaderLayer::Ptr, std::size_t>();
}

void MaterialEditor::updateBasicPageFromMaterial()
{
    auto nameEntry = getControl<wxTextCtrl>("BasicName");
    nameEntry->SetValue(_material ? _material->getName() : "");
    
    getControl<wxTextCtrl>("BasicDescription")->SetValue(_material ? _material->getDescription() : "");

    // Type dropdown
    auto* materialTypeDropdown = getControl<wxChoice>("BasicMaterialType");
    
    if (!_material || _material->getSurfaceType() == Material::SURFTYPE_DEFAULT)
    {
        materialTypeDropdown->Select(0);
    }
    else
    {
        auto surfType = shaders::getStringForSurfaceType(_material->getSurfaceType());
        materialTypeDropdown->Select(materialTypeDropdown->FindString(surfType));
    }

    updateBasicImagePreview();

    auto editorImageMap = getControl<MapExpressionEntry>("BasicEditorImageEntry");
    auto diffuseImageMap = getControl<MapExpressionEntry>("BasicDiffuseImageEntry");
    auto bumpImageMap = getControl<MapExpressionEntry>("BasicBumpImageEntry");
    auto specularImageMap = getControl<MapExpressionEntry>("BasicSpecularImageEntry");

    auto editorImg = _material ? _material->getEditorImageExpression() : shaders::IMapExpression::Ptr();
    editorImageMap->SetValue(editorImg ? editorImg->getExpressionString() : "");

    auto diffuse = findMaterialStageByType(IShaderLayer::DIFFUSE).first;
    auto expression = diffuse ? diffuse->getMapExpression() : shaders::IMapExpression::Ptr();
    diffuseImageMap->SetValue(expression ? expression->getExpressionString() : "");

    auto bump = findMaterialStageByType(IShaderLayer::BUMP).first;
    expression = bump ? bump->getMapExpression() : shaders::IMapExpression::Ptr();
    bumpImageMap->SetValue(expression ? expression->getExpressionString() : "");

    auto specular = findMaterialStageByType(IShaderLayer::SPECULAR).first;
    expression = specular ? specular->getMapExpression() : shaders::IMapExpression::Ptr();
    specularImageMap->SetValue(expression ? expression->getExpressionString() : "");

    updateBasicFrobStageControls();
}

void MaterialEditor::updateBasicFrobStageControls()
{
    auto addFrob = getControl<wxButton>("BasicAddFrobStages");
    auto removeFrob = getControl<wxButton>("BasicRemoveFrobStages");

    bool hasFrobStages = shaders::FrobStageSetup::IsPresent(_material);
    bool materialCanBeModified = _material && GlobalMaterialManager().materialCanBeModified(_material->getName());

    // We can add the frob stage if a diffuse map is present
    addFrob->Enable(materialCanBeModified && !hasFrobStages && !shaders::FrobStageSetup::GetDiffuseMap(_material).empty());
    removeFrob->Enable(materialCanBeModified && hasFrobStages);

    auto testFrob = getControl<wxButton>("BasicTestFrobStages");
    testFrob->Enable(materialCanBeModified && hasFrobStages);
}

void MaterialEditor::updateBasicImagePreview()
{
    auto editorImgTabImage = getControl<TexturePreview>("BasicEditorImageTabImage");
    auto diffuseTabImage = getControl<TexturePreview>("BasicDiffuseTabImage");
    auto bumpTabImage = getControl<TexturePreview>("BasicBumpTabImage");
    auto specularTabImage = getControl<TexturePreview>("BasicSpecularTabImage");

    editorImgTabImage->SetMaterial(_material);
    diffuseTabImage->SetMaterial(_material);
    bumpTabImage->SetMaterial(_material);
    specularTabImage->SetMaterial(_material);
}

void MaterialEditor::updateControlsFromMaterial()
{
    util::ScopedBoolLock lock(_materialUpdateInProgress);

    _preview->setMaterial(_material);

    updateMaterialControlSensitivity();
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
    }
    else // no material
    {
        getControl<wxChoice>("MaterialEditorDeformChoice")->Select(0);

        for (const auto& pair : _deformPanels)
        {
            pair.second->Hide();
        }
    }

    getControl<wxPanel>("MaterialPageDeform")->Layout();
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
    // Remove all non-global items
    _stageList->RemoveItems([&](const wxutil::TreeModel::Row& row)
    {
        return !row[STAGE_COLS().global].getBool();
    });

    getControl<wxPanel>("MaterialEditorStageListPanel")->Enable(_material != nullptr);

    if (!_material) return;

    const auto& layers = _material->getAllLayers();

    int index = 0;
    for (const auto& layer : layers)
    {
        auto row = _stageList->AddItem();

        row[STAGE_COLS().enabled] = layer->isEnabled();
        row[STAGE_COLS().icon] = wxVariant(layer->isEnabled() ? _iconVisible : _iconInvisible);
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
    util::ScopedBoolLock lock(_materialUpdateInProgress);

    bool materialCanBeModified = _material && GlobalMaterialManager().materialCanBeModified(_material->getName());

    getControl<wxPanel>("BasicEditorPanel")->Enable(materialCanBeModified);
    getControl<wxPanel>("MaterialNameAndDescription")->Enable(materialCanBeModified);
    getControl<wxPanel>("MaterialEditorImagePanel")->Enable(materialCanBeModified);
    
    // Enable/disable the settings pages
    for (const auto& pair : _notebookPages)
    {
        pair.second.first->Enable(materialCanBeModified);
    }

    auto nameEntry = getControl<wxTextCtrl>("MaterialName");
    nameEntry->Enable(materialCanBeModified);
    nameEntry->SetValue(_material ? _material->getName() : "");
    updateMaterialNameControl();
    
    // Update all registered bindings
    for (const auto& binding : _materialBindings)
    {
        binding->setSource(_material);
        binding->updateFromSource();
    }

    updateBasicPageFromMaterial();
    updateDeformControlsFromMaterial();
    updateSourceView();

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

        // Sort dropdown
        auto* materialSortDropdown = getControl<wxComboBox>("MaterialSortValue");
        if (_material->getMaterialFlags() & Material::FLAG_HAS_SORT_DEFINED)
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

        // Clamping
        auto clampDropdown = getControl<wxChoice>("MaterialClampType");
        auto clampTypeString = shaders::getStringForClampType(_material->getClampType());
        clampDropdown->SetStringSelection(clampTypeString);

        // Culling
        auto cullTypes = getControl<wxChoice>("MaterialCullType");
        auto cullTypeString = shaders::getStringForCullType(_material->getCullType());
        cullTypes->SetStringSelection(cullTypeString);

        // Light Falloff type
        auto lightFalloffCubeMapType = _material->getLightFalloffCubeMapType();
        getControl<wxChoice>("MaterialLightFalloffCubeMapType")->SetStringSelection(shaders::getStringForMapType(lightFalloffCubeMapType));
        
        // Spectrum
        bool hasSpectrum = (_material->getParseFlags() & Material::PF_HasSpectrum) != 0 || _material->getSpectrum() != 0;
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
        //getControl<wxTextCtrl>("MaterialRenderBumpArguments")->Enable(!_material->getRenderBumpArguments().empty());
        getControl<wxTextCtrl>("MaterialRenderBumpArguments")->SetValue(_material->getRenderBumpArguments());

        getControl<wxCheckBox>("MaterialHasRenderBumpFlat")->SetValue(!_material->getRenderBumpFlatArguments().empty());
        //getControl<wxTextCtrl>("MaterialRenderBumpFlatArguments")->Enable(!_material->getRenderBumpFlatArguments().empty());
        getControl<wxTextCtrl>("MaterialRenderBumpFlatArguments")->SetValue(_material->getRenderBumpFlatArguments());

        // guisurf
        auto guisurfArgument = _material->getGuiSurfArgument();
        getControl<wxTextCtrl>("MaterialGuiSurfPath")->SetValue(guisurfArgument);

        bool isEntityGui = (_material->getSurfaceFlags() & (Material::SURF_ENTITYGUI | Material::SURF_ENTITYGUI2 | Material::SURF_ENTITYGUI3)) != 0;

        //getControl<wxTextCtrl>("MaterialGuiSurfPath")->Enable(!isEntityGui);
        //getControl<wxPanel>("MaterialGuiSurfPanel")->Enable(_material->getSurfaceFlags() & Material::SURF_GUISURF);

        getControl<wxRadioButton>("MaterialGuiSurfRegular")->SetValue(!isEntityGui);
        getControl<wxRadioButton>("MaterialGuiSurfEntity")->SetValue(_material->getSurfaceFlags() & Material::SURF_ENTITYGUI);
        getControl<wxRadioButton>("MaterialGuiSurfEntity2")->SetValue(_material->getSurfaceFlags() & Material::SURF_ENTITYGUI2);
        getControl<wxRadioButton>("MaterialGuiSurfEntity3")->SetValue(_material->getSurfaceFlags() & Material::SURF_ENTITYGUI3);
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
        getControl<wxTextCtrl>("MaterialDescription")->SetValue("");
    }
}

void MaterialEditor::updateSourceView()
{
    if (_material)
    {
        // Surround the definition with curly braces, these are not included
        auto definition = fmt::format("{0}\n{{{1}}}", _material->getName(), _material->getDefinition());
        _sourceView->SetValue(definition);
    }
    else
    {
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

    updateSettingsNotebook();
    updateStageControls();
    updateStageButtonSensitivity();
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

        blendTypeSrc->Enable(!blendTypeStrings.second.empty());
        blendTypeDest->Enable(!blendTypeStrings.second.empty());

        if (blendTypeStrings.second.empty())
        {
            blendType->SetStringSelection(blendTypeStrings.first);

            blendTypeSrc->SetStringSelection("gl_one");
            blendTypeDest->SetStringSelection("gl_zero");

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
            blendType->SetStringSelection(_(CUSTOM_BLEND_TYPE));
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

    updateStageBlendControls();
    updateStageTextureControls();
    updateStageProgramControls(); 
    updateStageTransformControls();
    updateStageColoredStatus();

    if (selectedStage)
    {
        selectedStage->evaluateExpressions(0); // initialise the values of this stage
        
        auto imageMap = getControl<MapExpressionEntry>("MaterialStageImageMap");
        auto videoMapLoop = getControl<wxCheckBox>("MaterialStageVideoMapLoop");
        auto soundMapWave = getControl<wxCheckBox>("MaterialStageSoundMapWaveform");

        auto renderMapWidth = getControl<wxSpinCtrl>("MaterialStageRenderMapWidth");
        auto renderMapHeight = getControl<wxSpinCtrl>("MaterialStageRenderMapHeight");

        auto mapType = getControl<wxChoice>("MaterialStageMapType");
        mapType->SetStringSelection(shaders::getStringForMapType(selectedStage->getMapType()));

        if (selectedStage->getMapType() != IShaderLayer::MapType::SoundMap)
        {
            soundMapWave->SetValue(false);
        }

        if (selectedStage->getMapType() == IShaderLayer::MapType::RemoteRenderMap ||
            selectedStage->getMapType() == IShaderLayer::MapType::MirrorRenderMap ||
            selectedStage->getMapType() == IShaderLayer::MapType::SoundMap)
        {
            imageMap->Disable();
        }
        else
        {
            imageMap->Enable();
            videoMapLoop->SetValue(false);
            soundMapWave->SetValue(false);
            renderMapWidth->SetValue(0);
            renderMapHeight->SetValue(0);
        }

        switch (selectedStage->getMapType())
        {
        case IShaderLayer::MapType::VideoMap:
            {
                auto videoMapExpression = std::dynamic_pointer_cast<shaders::IVideoMapExpression>(selectedStage->getMapExpression());
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
        case IShaderLayer::MapType::MirrorRenderMap:
            renderMapWidth->SetValue(static_cast<int>(selectedStage->getRenderMapSize().x()));
            renderMapHeight->SetValue(static_cast<int>(selectedStage->getRenderMapSize().y()));
            break;
        }

        // Show the specialised map type controls
        videoMapLoop->Show(selectedStage->getMapType() == IShaderLayer::MapType::VideoMap);
        soundMapWave->Show(selectedStage->getMapType() == IShaderLayer::MapType::SoundMap);
        getControl<wxPanel>("MaterialStageRenderMapPanel")->Show(selectedStage->getMapType() == IShaderLayer::MapType::MirrorRenderMap ||
            selectedStage->getMapType() == IShaderLayer::MapType::RemoteRenderMap);

        getControl<wxPanel>("StagePageBlending")->Layout();

        // Vertex Colours
        getControl<wxRadioButton>("MaterialStageNoVertexColourFlag")->SetValue(selectedStage->getVertexColourMode() == IShaderLayer::VERTEX_COLOUR_NONE);
        getControl<wxRadioButton>("MaterialStageVertexColourFlag")->SetValue(selectedStage->getVertexColourMode() == IShaderLayer::VERTEX_COLOUR_MULTIPLY);
        getControl<wxRadioButton>("MaterialStageInverseVertexColourFlag")->SetValue(selectedStage->getVertexColourMode() == IShaderLayer::VERTEX_COLOUR_INVERSE_MULTIPLY);
    }
    else
    {
        getControl<wxRadioButton>("MaterialStageNoVertexColourFlag")->SetValue(true);
    }
}

void MaterialEditor::_onMaterialTypeChoice(wxCommandEvent& ev)
{
    if (!_material || _materialUpdateInProgress) return;
    
    auto selectedString = static_cast<wxChoice*>(ev.GetEventObject())->GetStringSelection();
    _material->setSurfaceType(shaders::getSurfaceTypeForString(selectedString.ToStdString()));

    updateMaterialPropertiesFromMaterial();
}

void MaterialEditor::_onAddStageTransform(wxCommandEvent& ev)
{
    auto selectedStage = getEditableStageForSelection();

    if (selectedStage)
    {
        auto typeString = getControl<wxChoice>("MaterialStageAddTransformChoice")->GetStringSelection();
        auto type = shaders::getTransformTypeForString(typeString.ToStdString());

        selectedStage->appendTransformation(IShaderLayer::Transformation{ type });

        updateStageControls();
        onMaterialChanged();
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
        onMaterialChanged();
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
    onMaterialChanged();
}

void MaterialEditor::_onStageColoredChecked(const IEditableShaderLayer::Ptr& stage, bool newValue)
{
    bool stageIsCurrentlyColoured = stageQualifiesAsColoured(stage);

    if (newValue && !stageIsCurrentlyColoured)
    {
        stage->setColourExpressionFromString(IShaderLayer::COMP_RED, "parm0");
        stage->setColourExpressionFromString(IShaderLayer::COMP_GREEN, "parm1");
        stage->setColourExpressionFromString(IShaderLayer::COMP_BLUE, "parm2");
        stage->setColourExpressionFromString(IShaderLayer::COMP_ALPHA, "parm3");
    }
    else if (!newValue && stageIsCurrentlyColoured)
    {
        stage->setColourExpressionFromString(IShaderLayer::COMP_RED, "");
        stage->setColourExpressionFromString(IShaderLayer::COMP_GREEN, "");
        stage->setColourExpressionFromString(IShaderLayer::COMP_BLUE, "");
        stage->setColourExpressionFromString(IShaderLayer::COMP_ALPHA, "");
    }

    updateStageControls();
}

void MaterialEditor::_onStageMapTypeChanged(wxCommandEvent& ev)
{
    auto stage = getEditableStageForSelection();
    if (!stage) return;

    auto mapTypeString = getControl<wxChoice>("MaterialStageMapType")->GetStringSelection();
    stage->setMapType(shaders::getMapTypeForString(mapTypeString.ToStdString()));

    updateStageControls();
}

void MaterialEditor::_onStageBlendTypeChanged(wxCommandEvent& ev)
{
    auto stage = getEditableStageForSelection();
    if (!stage) return;

    std::pair<std::string, std::string> blendFuncStrings;

    auto blendTypeString = getControl<wxChoice>("MaterialStageBlendType")->GetStringSelection();

    if (blendTypeString == _(CUSTOM_BLEND_TYPE))
    {
        blendFuncStrings.first = getControl<wxChoice>("MaterialStageBlendTypeSrc")->GetStringSelection();
        blendFuncStrings.second = getControl<wxChoice>("MaterialStageBlendTypeDest")->GetStringSelection();
    }
    else
    {
        // It's one of diffuse/bump/specular or a blend shortcut like add/modulate
        blendFuncStrings.first = blendTypeString;
    }

    stage->setBlendFuncStrings(blendFuncStrings);
    
    updateNameOfSelectedStage();
    updateStageControls();
    onMaterialChanged();
}

void MaterialEditor::_onAddStage(wxCommandEvent& ev)
{
    std::size_t index = _material->addLayer(determineStageTypeToCreate(_material));

    updateControlsFromMaterial();
    selectStageByIndex(index);
}

void MaterialEditor::_onRemoveStage(wxCommandEvent& ev)
{
    auto item = _stageView->GetSelection();
    if (!_material || !item.IsOk()) return;

    auto row = wxutil::TreeModel::Row(item, *_stageList);
    auto index = row[STAGE_COLS().index].getInteger();

    _material->removeLayer(index);
    updateStageListFromMaterial();

    auto layersCount = _material->getAllLayers().size();

    while (index > 0)
    {
        if (index < layersCount)
        {
            selectStageByIndex(index);
            return;
        }

        --index;
    }
}

void MaterialEditor::toggleSelectedStage()
{
    if (!_stageView->GetSelection().IsOk()) return;

    wxutil::TreeModel::Row row(_stageView->GetSelection(), *_stageList);

    if (!_material || !GlobalMaterialManager().materialCanBeModified(_material->getName()) ||
        row[STAGE_COLS().global].getBool())
    {
        return;
    }

    auto stage = getEditableStageForSelection();

    if (!stage) return;

    bool newState = !row[STAGE_COLS().enabled].getBool();
    stage->setEnabled(newState);

    row[STAGE_COLS().enabled] = newState;
    row[STAGE_COLS().icon] = wxVariant(newState ? _iconVisible : _iconInvisible);
    row.SendItemChanged();

    onMaterialChanged();
}

void MaterialEditor::_onToggleStage(wxCommandEvent& ev)
{
    toggleSelectedStage();
}

void MaterialEditor::_onDuplicateStage(wxCommandEvent& ev)
{
    auto item = _stageView->GetSelection();
    if (!_material || !item.IsOk()) return;

    auto row = wxutil::TreeModel::Row(item, *_stageList);
    auto index = row[STAGE_COLS().index].getInteger();

    auto newIndex = _material->duplicateLayer(index);
    updateStageListFromMaterial();
    selectStageByIndex(newIndex);
}

void MaterialEditor::updateNameOfSelectedStage()
{
    auto item = _stageView->GetSelection();
    if (!item.IsOk()) return;

    auto row = wxutil::TreeModel::Row(item, *_stageList);

    row[STAGE_COLS().name] = getNameForLayer(*getSelectedStage());
    row.SendItemChanged();
}

void MaterialEditor::moveStagePosition(int direction)
{
    auto item = _stageView->GetSelection();
    if (!_material || !item.IsOk()) return;

    auto row = wxutil::TreeModel::Row(item, *_stageList);
    auto index = row[STAGE_COLS().index].getInteger();

    int newPosition = index + direction;

    if (newPosition >= 0 && newPosition < _material->getAllLayers().size())
    {
        _material->swapLayerPosition(static_cast<std::size_t>(index), static_cast<std::size_t>(newPosition));
        onMaterialChanged();
        updateStageListFromMaterial();
        selectStageByIndex(static_cast<std::size_t>(newPosition));
    }
}

void MaterialEditor::_onSortRequestChanged(wxCommandEvent& ev)
{
    if (!_material || _materialUpdateInProgress) return;

    auto sortDropdown = getControl<wxComboBox>("MaterialSortValue");
    auto value = sortDropdown->GetValue().ToStdString();
    _material->setSortRequest(shaders::getSortRequestValueForString(value));
    
    onMaterialChanged();
}

void MaterialEditor::updateMaterialNameControl()
{
    if (!_material) return;

    auto nameControl = getControl<wxTextCtrl>("MaterialName");
    if (nameControl->GetValue() != _material->getName())
    {
        nameControl->SetForegroundColour(wxColor(220, 0, 0));
    }
    else
    {
        nameControl->SetForegroundColour(wxNullColour);
    }
}

void MaterialEditor::updateMaterialTreeItem()
{
    if (!_material) return;

    const auto& columns = _treeView->Columns();
    auto item = _treeView->GetTreeModel()->FindString(_material->getName(), columns.fullName);

    if (!item.IsOk())
    {
        return;
    }

    bool isModified = _material->isModified();

    wxutil::TreeModel::Row row(item, *_treeView->GetModel());
    
    if (!row[columns.isFolder].getBool())
    {
        row[columns.iconAndName] = wxutil::TreeViewItemStyle::Modified(isModified);
    }
    else
    {
        row[columns.iconAndName] = wxDataViewItemAttr();
    }

    wxDataViewIconText value = row[columns.iconAndName];
        
    if (!isModified && value.GetText().EndsWith("*"))
    {
        value.SetText(value.GetText().RemoveLast(1));
        row[columns.iconAndName] = wxVariant(value);
    }
    else if (isModified && !value.GetText().EndsWith("*"))
    {
        value.SetText(value.GetText() + "*");
        row[columns.iconAndName] = wxVariant(value);
    }

    row.SendItemChanged();
}

void MaterialEditor::onMaterialChanged()
{
    updateMaterialPropertiesFromMaterial();
    updateMaterialTreeItem();
    updateMaterialControlSensitivity();
    updateSourceView();
}

void MaterialEditor::convertTextCtrlToMapExpressionEntry(const std::string& ctrlName)
{
    auto windowToPlaceDialogsOn = getControl<wxWindow>("MaterialEditorLeftArea");

    auto oldCtrl = findNamedObject<wxTextCtrl>(this, ctrlName);
    replaceControl(oldCtrl, new MapExpressionEntry(oldCtrl->GetParent(), windowToPlaceDialogsOn));
}

void MaterialEditor::_onBasicAddFrobStages(wxCommandEvent& ev)
{
    if (!_material || !GlobalMaterialManager().materialCanBeModified(_material->getName()))
    {
        return;
    }

    try
    {
        shaders::FrobStageSetup::AddToMaterial(_material);
    }
    catch (const std::runtime_error& ex)
    {
        wxutil::Messagebox::ShowError(ex.what(), this);
        rError() << ex.what() << std::endl;
    }

    updateControlsFromMaterial();
    onMaterialChanged();
}

void MaterialEditor::_onBasicRemoveFrobStages(wxCommandEvent& ev)
{
    if (!_material || !GlobalMaterialManager().materialCanBeModified(_material->getName()))
    {
        return;
    }

    try
    {
        shaders::FrobStageSetup::RemoveFromMaterial(_material);
    }
    catch (const std::runtime_error& ex)
    {
        wxutil::Messagebox::ShowError(ex.what(), this);
        rError() << ex.what() << std::endl;
    }
    
    updateControlsFromMaterial();
    onMaterialChanged();
}

void MaterialEditor::_onBasicTestFrobStages(wxMouseEvent& ev)
{
    _preview->enableFrobHighlight(ev.ButtonDown());
    ev.Skip();
}

}
