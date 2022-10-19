#include "ParticleEditor.h"

#include "i18n.h"
#include "ui/imainframe.h"
#include "iparticles.h"
#include "iparticlestage.h"
#include "igame.h"
#include "itextstream.h"

#include "wxutil/FileChooser.h"
#include "wxutil/dialog/MessageBox.h"
#include "wxutil/dialog/Dialog.h"
#include "wxutil/EntryAbortedException.h"

#include <wx/button.h>
#include <wx/spinctrl.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/textctrl.h>
#include <wx/panel.h>
#include <wx/checkbox.h>
#include <wx/slider.h>
#include <wx/radiobut.h>
#include <wx/stattext.h>
#include <wx/notebook.h>

#include "os/fs.h"
#include "os/file.h"
#include "os/path.h"
#include "util/ScopedBoolLock.h"

#include "string/predicate.h"

namespace ui
{

using namespace particles;

// CONSTANTS
namespace
{
    const char* const DIALOG_TITLE = N_("Particle Editor");

    const std::string RKEY_ROOT = "user/ui/particleEditor/";
    const std::string RKEY_SPLIT_POS = RKEY_ROOT + "splitPos";
    const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
    const std::string RKEY_RECENT_PATH = RKEY_ROOT + "recentSavePath";

    const std::string EDIT_SUFFIX = "___editor";
}

namespace
{
    // Columns for the def list
    struct DefColumns :
        public wxutil::TreeModel::ColumnRecord
    {
        DefColumns() : 
			name(add(wxutil::TreeModel::Column::String))
		{}

        wxutil::TreeModel::Column name;
    };
    DefColumns& DEF_COLS() { static DefColumns _i; return _i; }

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
    StageColumns& STAGE_COLS() { static StageColumns _i; return _i; }
}

ParticleEditor::ParticleEditor() :
    DialogBase(DIALOG_TITLE),
    _defList(new wxutil::TreeModel(DEF_COLS(), true)),
	_defView(NULL),
    _stageList(new wxutil::TreeModel(STAGE_COLS(), true)),
	_stageView(NULL),
    _callbacksDisabled(false),
    _saveInProgress(false)
{
	loadNamedPanel(this, "ParticleEditorMainPanel");

    // Wire up the close button
	findNamedObject<wxButton>(this, "ParticleEditorCloseButton")->Connect(
		wxEVT_BUTTON, wxCommandEventHandler(ParticleEditor::_onClose), NULL, this);

	findNamedObject<wxButton>(this, "ParticleEditorNewDefButton")->Connect(
		wxEVT_BUTTON, wxCommandEventHandler(ParticleEditor::_onNewParticle), NULL, this);
	findNamedObject<wxButton>(this, "ParticleEditorSaveDefButton")->Connect(
		wxEVT_BUTTON, wxCommandEventHandler(ParticleEditor::_onSaveParticle), NULL, this);
	findNamedObject<wxButton>(this, "ParticleEditorCopyDefButton")->Connect(
		wxEVT_BUTTON, wxCommandEventHandler(ParticleEditor::_onCloneCurrentParticle), NULL, this);

	// Set up the preview
	wxPanel* previewPanel = findNamedObject<wxPanel>(this, "ParticleEditorPreviewPanel");
	_preview.reset(new wxutil::ParticlePreview(previewPanel));

	previewPanel->GetSizer()->Add(_preview->getWidget(), 1, wxEXPAND);
	
    // Set the default size of the window
	FitToScreen(0.6f, 0.6f);

    // Setup the splitter and preview
	wxSplitterWindow* splitter = findNamedObject<wxSplitterWindow>(this, "ParticleEditorSplitter");
	splitter->SetSashPosition(GetSize().GetWidth() * 0.6f);
    splitter->SetMinimumPaneSize(10); // disallow unsplitting

	setupParticleDefList();
    setupParticleStageList();
    setupSettingsPages();

	Layout();
	Fit();

	// Connect the window position tracker
    _windowPosition.loadFromPath(RKEY_WINDOW_STATE);
    _windowPosition.connect(this);
    _windowPosition.applyPosition();

    _panedPosition.connect(splitter);
    _panedPosition.loadFromPath(RKEY_SPLIT_POS);

    CenterOnParent();

    // Fire the selection changed signal to initialise the sensitivity
    handleDefSelChanged();
    handleStageSelChanged();
}

bool ParticleEditor::_onDeleteEvent()
{
    if (!promptUserToSaveChanges(false)) return true; // action not allowed or cancelled

	// Pass to base class, which defaults to "ok, let's close"
	return DialogBase::_onDeleteEvent();
}

void ParticleEditor::setupParticleDefList()
{
	wxPanel* panel = findNamedObject<wxPanel>(this, "ParticleEditorDefinitionView");

	_defView = wxutil::TreeView::CreateWithModel(panel, _defList.get(), wxDV_NO_HEADER);
	panel->GetSizer()->Add(_defView, 1, wxEXPAND);

	// Single text column
	_defView->AppendTextColumn(_("Particle"), DEF_COLS().name.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

    // Apply full-text search to the column
	_defView->AddSearchColumn(DEF_COLS().name);
    
    populateParticleDefList();

    // Connect up the selection changed callback
	_defView->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED,
		wxDataViewEventHandler(ParticleEditor::_onDefSelChanged), NULL, this);
}

void ParticleEditor::populateParticleDefList()
{
    _selectedDefIter = wxDataViewItem();
    _defList->Clear();

    // Create and use a ParticlesVisitor to populate the list
    GlobalParticlesManager().forEachParticleDef([&] (const IParticleDef& particle)
	{
        auto name = particle.getDeclName();

        if (string::ends_with(name, EDIT_SUFFIX))
        {
            return; // skip the edit particles, they should not be showing up
        }

		// Add the Def name to the list store
        wxutil::TreeModel::Row row = _defList->AddItem();

        row[DEF_COLS().name] = name;

		row.SendItemAdded();
	});
}

void ParticleEditor::selectParticleDef(const std::string& particleDefName)
{
	wxDataViewItem found = _defList->FindString(particleDefName, DEF_COLS().name);

	if (found.IsOk())
	{
		_defView->Select(found);
		handleDefSelChanged();
	}
}

void ParticleEditor::setupParticleStageList()
{
	wxPanel* panel = findNamedObject<wxPanel>(this, "ParticleEditorStageView");

	_stageView = wxutil::TreeView::CreateWithModel(panel, _stageList.get(), wxDV_NO_HEADER);
	panel->GetSizer()->Add(_stageView, 1, wxEXPAND);

	// Single text column
	_stageView->AppendTextColumn(_("Stage"), DEF_COLS().name.getColumnIndex(),
		wxDATAVIEW_CELL_INERT, wxCOL_WIDTH_AUTOSIZE, wxALIGN_NOT, wxDATAVIEW_COL_SORTABLE);

    // Connect up the selection changed callback
	_stageView->Connect(wxEVT_DATAVIEW_SELECTION_CHANGED,
		wxDataViewEventHandler(ParticleEditor::_onStageSelChanged), NULL, this);

    // Connect the stage control buttons
	findNamedObject<wxButton>(this, "ParticleEditorAddStageButton")->Connect(
		wxEVT_BUTTON, wxCommandEventHandler(ParticleEditor::_onAddStage), NULL, this);
	findNamedObject<wxButton>(this, "ParticleEditorRemoveStageButton")->Connect(
		wxEVT_BUTTON, wxCommandEventHandler(ParticleEditor::_onRemoveStage), NULL, this);
	findNamedObject<wxButton>(this, "ParticleEditorToggleStageButton")->Connect(
		wxEVT_BUTTON, wxCommandEventHandler(ParticleEditor::_onToggleStage), NULL, this);
	findNamedObject<wxButton>(this, "ParticleEditorMoveUpStageButton")->Connect(
		wxEVT_BUTTON, wxCommandEventHandler(ParticleEditor::_onMoveUpStage), NULL, this);
	findNamedObject<wxButton>(this, "ParticleEditorMoveDownStageButton")->Connect(
		wxEVT_BUTTON, wxCommandEventHandler(ParticleEditor::_onMoveDownStage), NULL, this);
	findNamedObject<wxButton>(this, "ParticleEditorDuplicateStageButton")->Connect(
		wxEVT_BUTTON, wxCommandEventHandler(ParticleEditor::_onDuplicateStage), NULL, this);
}

wxSpinCtrlDouble* ParticleEditor::convertToSpinCtrlDouble(const std::string& name, double min, double max, double increment, int digits)
{
	auto spinCtrlDouble = XmlResourceBasedWidget::convertToSpinCtrlDouble(this, name, min, max, increment, digits);
    spinCtrlDouble->SetMaxSize(wxSize(70, -1));
    return spinCtrlDouble;
}

void ParticleEditor::setupSettingsPages()
{
    // Depth Hack
	wxSpinCtrlDouble* depthHack = convertToSpinCtrlDouble("ParticleEditorDepthHack", 0, 999, 0.1, 2);
	
	depthHack->Connect(wxEVT_SPINCTRLDOUBLE, 
		wxSpinDoubleEventHandler(ParticleEditor::_onDepthHackChanged), NULL, this);

    // SHADER
	findNamedObject<wxTextCtrl>(this, "ParticleEditorStageShader")->Connect(
		wxEVT_TEXT, wxCommandEventHandler(ParticleEditor::_onShaderControlsChanged), NULL, this);
	findNamedObject<wxTextCtrl>(this, "ParticleEditorStageColour")->Connect(
		wxEVT_TEXT, wxCommandEventHandler(ParticleEditor::_onShaderControlsChanged), NULL, this);
	findNamedObject<wxTextCtrl>(this, "ParticleEditorStageFadeColour")->Connect(
		wxEVT_TEXT, wxCommandEventHandler(ParticleEditor::_onShaderControlsChanged), NULL, this);

	findNamedObject<wxCheckBox>(this, "ParticleEditorStageUseEntityColour")->Connect(
		wxEVT_CHECKBOX, wxCommandEventHandler(ParticleEditor::_onShaderControlsChanged), NULL, this);

	convertToSpinCtrlDouble("ParticleEditorStageFadeInFrac", 0, 1, 0.01, 2);
	convertToSpinCtrlDouble("ParticleEditorStageFadeOutFrac", 0, 1, 0.01, 2);
	convertToSpinCtrlDouble("ParticleEditorStageFadeIdxFrac", 0, 1, 0.01, 2);
	convertToSpinCtrlDouble("ParticleEditorStageAnimRate", 0, 600, 0.1, 2);

    connectSpinner("ParticleEditorStageFadeInFrac", &ParticleEditor::_onShaderControlsChanged);
	connectSpinner("ParticleEditorStageFadeOutFrac", &ParticleEditor::_onShaderControlsChanged);
    connectSpinner("ParticleEditorStageFadeIdxFrac", &ParticleEditor::_onShaderControlsChanged);
    connectSpinner("ParticleEditorStageAnimFrames", &ParticleEditor::_onShaderControlsChanged);
    connectSpinner("ParticleEditorStageAnimRate", &ParticleEditor::_onShaderControlsChanged);

    // COUNT

	convertToSpinCtrlDouble("ParticleEditorStageDuration", 0, 999, 0.1, 2);
	convertToSpinCtrlDouble("ParticleEditorStageBunching", 0, 1, 0.1, 2);
	convertToSpinCtrlDouble("ParticleEditorStageTimeOffset", 0, 999, 0.1, 2);
	convertToSpinCtrlDouble("ParticleEditorStageDeadTime", 0, 999, 0.1, 2);

    connectSpinner("ParticleEditorStageCount", &ParticleEditor::_onCountTimeControlsChanged);
    connectSpinner("ParticleEditorStageDuration", &ParticleEditor::_onCountTimeControlsChanged);
    connectSpinner("ParticleEditorStageBunching", &ParticleEditor::_onCountTimeControlsChanged);
    connectSpinner("ParticleEditorStageCycles", &ParticleEditor::_onCountTimeControlsChanged);
    connectSpinner("ParticleEditorStageTimeOffset", &ParticleEditor::_onCountTimeControlsChanged);
    connectSpinner("ParticleEditorStageDeadTime", &ParticleEditor::_onCountTimeControlsChanged);

    // DISTRIBUTION

	findNamedObject<wxRadioButton>(this, "ParticleEditorStageShapeRect")->Connect(
		wxEVT_RADIOBUTTON, wxCommandEventHandler(ParticleEditor::_onDistributionControlsChanged), NULL, this);
	findNamedObject<wxRadioButton>(this, "ParticleEditorStageShapeCyl")->Connect(
		wxEVT_RADIOBUTTON, wxCommandEventHandler(ParticleEditor::_onDistributionControlsChanged), NULL, this);
	findNamedObject<wxRadioButton>(this, "ParticleEditorStageSpherical")->Connect(
		wxEVT_RADIOBUTTON, wxCommandEventHandler(ParticleEditor::_onDistributionControlsChanged), NULL, this);

	convertToSpinCtrlDouble("ParticleEditorStageXSize", 0, 999, 0.1, 2);
	convertToSpinCtrlDouble("ParticleEditorStageYSize", 0, 999, 0.1, 2);
	convertToSpinCtrlDouble("ParticleEditorStageZSize", 0, 999, 0.1, 2);
	convertToSpinCtrlDouble("ParticleEditorStageRingSize", 0, 999, 0.1, 2);

    connectSpinner("ParticleEditorStageXSize", &ParticleEditor::_onDistributionControlsChanged);
    connectSpinner("ParticleEditorStageYSize", &ParticleEditor::_onDistributionControlsChanged);
    connectSpinner("ParticleEditorStageZSize", &ParticleEditor::_onDistributionControlsChanged);
    connectSpinner("ParticleEditorStageRingSize", &ParticleEditor::_onDistributionControlsChanged);

	findNamedObject<wxTextCtrl>(this, "ParticleEditorStageOffset")->Connect(
		wxEVT_TEXT, wxCommandEventHandler(ParticleEditor::_onDistributionControlsChanged), NULL, this);
	findNamedObject<wxCheckBox>(this, "ParticleEditorStageRandomDist")->Connect(
		wxEVT_CHECKBOX, wxCommandEventHandler(ParticleEditor::_onDistributionControlsChanged), NULL, this);

    // DIRECTION / ORIENTATION

	findNamedObject<wxRadioButton>(this, "ParticleEditorStageCone")->Connect(
		wxEVT_RADIOBUTTON, wxCommandEventHandler(ParticleEditor::_onDirectionControlsChanged), NULL, this);
	findNamedObject<wxRadioButton>(this, "ParticleEditorStageOutward")->Connect(
		wxEVT_RADIOBUTTON, wxCommandEventHandler(ParticleEditor::_onDirectionControlsChanged), NULL, this);

	convertToSpinCtrlDouble("ParticleEditorStageConeAngle", 0, 180, 0.1, 2);
	convertToSpinCtrlDouble("ParticleEditorStageUpwardBias", 0, 999, 0.1, 2);
	convertToSpinCtrlDouble("ParticleEditorStageAimedTime", 0, 60, 0.1, 2);
	convertToSpinCtrlDouble("ParticleEditorStageInitialAngle", 0, 359, 0.1, 2);
	
    connectSpinner("ParticleEditorStageConeAngle", &ParticleEditor::_onDirectionControlsChanged);
    connectSpinner("ParticleEditorStageUpwardBias", &ParticleEditor::_onDirectionControlsChanged);

	findNamedObject<wxRadioButton>(this, "ParticleEditorStageOrientView")->Connect(
		wxEVT_RADIOBUTTON, wxCommandEventHandler(ParticleEditor::_onDirectionControlsChanged), NULL, this);
	findNamedObject<wxRadioButton>(this, "ParticleEditorStageOrientAimed")->Connect(
		wxEVT_RADIOBUTTON, wxCommandEventHandler(ParticleEditor::_onDirectionControlsChanged), NULL, this);
	findNamedObject<wxRadioButton>(this, "ParticleEditorStageOrientX")->Connect(
		wxEVT_RADIOBUTTON, wxCommandEventHandler(ParticleEditor::_onDirectionControlsChanged), NULL, this);
	findNamedObject<wxRadioButton>(this, "ParticleEditorStageOrientY")->Connect(
		wxEVT_RADIOBUTTON, wxCommandEventHandler(ParticleEditor::_onDirectionControlsChanged), NULL, this);
	findNamedObject<wxRadioButton>(this, "ParticleEditorStageOrientZ")->Connect(
		wxEVT_RADIOBUTTON, wxCommandEventHandler(ParticleEditor::_onDirectionControlsChanged), NULL, this);

    connectSpinner("ParticleEditorStageTrails", &ParticleEditor::_onDirectionControlsChanged);
    connectSpinner("ParticleEditorStageAimedTime", &ParticleEditor::_onDirectionControlsChanged);
    connectSpinner("ParticleEditorStageInitialAngle", &ParticleEditor::_onDirectionControlsChanged);

    // SIZE / SPEED / ASPECT

	convertToSpinCtrlDouble("ParticleEditorStageSpeedFrom", -500, 500, 0.5, 1);
	convertToSpinCtrlDouble("ParticleEditorStageSpeedTo", -500, 500, 0.5, 1);
	convertToSpinCtrlDouble("ParticleEditorStageSizeFrom", 0, 500, 0.5, 1);
	convertToSpinCtrlDouble("ParticleEditorStageSizeTo", 0, 500, 0.5, 1);
	convertToSpinCtrlDouble("ParticleEditorStageRotationSpeedFrom", 0, 500, 0.5, 1);
	convertToSpinCtrlDouble("ParticleEditorStageRotationSpeedTo", 0, 500, 0.5, 1);
	convertToSpinCtrlDouble("ParticleEditorStageAspectFrom", 0, 500, 0.5, 1);
	convertToSpinCtrlDouble("ParticleEditorStageAspectTo", 0, 500, 0.5, 1);
	convertToSpinCtrlDouble("ParticleEditorStageGravity", -999, 999, 0.1, 1);
	convertToSpinCtrlDouble("ParticleEditorStageBoundsExpansion", 0, 2500, 0.1, 2);

    connectSpinner("ParticleEditorStageSpeedFrom", &ParticleEditor::_onSizeControlsChanged);
    connectSpinner("ParticleEditorStageSpeedTo", &ParticleEditor::_onSizeControlsChanged);
    connectSpinner("ParticleEditorStageSizeFrom", &ParticleEditor::_onSizeControlsChanged);
    connectSpinner("ParticleEditorStageSizeTo", &ParticleEditor::_onSizeControlsChanged);
    connectSpinner("ParticleEditorStageRotationSpeedFrom", &ParticleEditor::_onSizeControlsChanged);
    connectSpinner("ParticleEditorStageRotationSpeedTo", &ParticleEditor::_onSizeControlsChanged);
    connectSpinner("ParticleEditorStageAspectFrom", &ParticleEditor::_onSizeControlsChanged);
    connectSpinner("ParticleEditorStageAspectTo", &ParticleEditor::_onSizeControlsChanged);
    connectSpinner("ParticleEditorStageGravity", &ParticleEditor::_onSizeControlsChanged);
    connectSpinner("ParticleEditorStageBoundsExpansion", &ParticleEditor::_onSizeControlsChanged);

	findNamedObject<wxCheckBox>(this, "ParticleEditorStageUseWorldGravity")->Connect(
		wxEVT_CHECKBOX, wxCommandEventHandler(ParticleEditor::_onSizeControlsChanged), NULL, this);

    // PATH

	findNamedObject<wxRadioButton>(this, "ParticleEditorStagePathStandard")->Connect(
		wxEVT_RADIOBUTTON, wxCommandEventHandler(ParticleEditor::_onPathControlsChanged), NULL, this);
	findNamedObject<wxRadioButton>(this, "ParticleEditorStagePathFlies")->Connect(
		wxEVT_RADIOBUTTON, wxCommandEventHandler(ParticleEditor::_onPathControlsChanged), NULL, this);
	findNamedObject<wxRadioButton>(this, "ParticleEditorStagePathHelix")->Connect(
		wxEVT_RADIOBUTTON, wxCommandEventHandler(ParticleEditor::_onPathControlsChanged), NULL, this);

	convertToSpinCtrlDouble("ParticleEditorStageRadialSpeed", 0, 999, 0.1, 2);
	convertToSpinCtrlDouble("ParticleEditorStageAxialSpeed", 0, 200, 0.1, 2);
	convertToSpinCtrlDouble("ParticleEditorStageSphereRadius", 0, 999, 0.1, 2);
	convertToSpinCtrlDouble("ParticleEditorStageCylSizeX", 0, 999, 0.1, 2);
	convertToSpinCtrlDouble("ParticleEditorStageCylSizeY", 0, 999, 0.1, 2);
	convertToSpinCtrlDouble("ParticleEditorStageCylSizeZ", 0, 999, 0.1, 2);

    connectSpinner("ParticleEditorStageRadialSpeed", &ParticleEditor::_onPathControlsChanged);
    connectSpinner("ParticleEditorStageAxialSpeed", &ParticleEditor::_onPathControlsChanged);
    connectSpinner("ParticleEditorStageSphereRadius", &ParticleEditor::_onPathControlsChanged);
    connectSpinner("ParticleEditorStageCylSizeX", &ParticleEditor::_onPathControlsChanged);
    connectSpinner("ParticleEditorStageCylSizeY", &ParticleEditor::_onPathControlsChanged);
    connectSpinner("ParticleEditorStageCylSizeZ", &ParticleEditor::_onPathControlsChanged);
}

void ParticleEditor::_onShaderControlsChanged(wxCommandEvent& ev)
{
    if (_callbacksDisabled || !_currentDef || !_selectedStageIter.IsOk()) return;

    const auto& stage = _currentDef->getStage(getSelectedStageIndex());

    std::string material = findNamedObject<wxTextCtrl>(this, "ParticleEditorStageShader")->GetValue().ToStdString();

    // Only assign a new material if it has actually changed, otherwise the whole particle gets re-shuffled
    if (material != stage->getMaterialName())
    {
        stage->setMaterialName(material);
    }

    stage->setColour(string::convert<Vector4>(
		findNamedObject<wxTextCtrl>(this, "ParticleEditorStageColour")->GetValue().ToStdString()));

    stage->setUseEntityColour(findNamedObject<wxCheckBox>(this, "ParticleEditorStageUseEntityColour")->GetValue());
    stage->setFadeColour(string::convert<Vector4>(
		findNamedObject<wxTextCtrl>(this, "ParticleEditorStageFadeColour")->GetValue().ToStdString()));

    stage->setFadeInFraction(getSpinButtonValueAsFloat("ParticleEditorStageFadeInFrac"));
    stage->setFadeOutFraction(getSpinButtonValueAsFloat("ParticleEditorStageFadeOutFrac"));
    stage->setFadeIndexFraction(getSpinButtonValueAsFloat("ParticleEditorStageFadeIdxFrac"));
    stage->setAnimationFrames(getSpinButtonValueAsInt("ParticleEditorStageAnimFrames"));
    stage->setAnimationRate(getSpinButtonValueAsFloat("ParticleEditorStageAnimRate"));
}

void ParticleEditor::_onCountTimeControlsChanged(wxCommandEvent& ev)
{
    if (_callbacksDisabled || !_currentDef || !_selectedStageIter.IsOk()) return;

    const auto& stage = _currentDef->getStage(getSelectedStageIndex());

    stage->setCount(getSpinButtonValueAsInt("ParticleEditorStageCount"));
    stage->setDuration(getSpinButtonValueAsFloat("ParticleEditorStageDuration"));
    stage->setBunching(getSpinButtonValueAsFloat("ParticleEditorStageBunching"));
    stage->setCycles(getSpinButtonValueAsInt("ParticleEditorStageCycles"));
    stage->setTimeOffset(getSpinButtonValueAsFloat("ParticleEditorStageTimeOffset"));
    stage->setDeadTime(getSpinButtonValueAsFloat("ParticleEditorStageDeadTime"));
}

void ParticleEditor::_onDistributionControlsChanged(wxCommandEvent& ev)
{
    if (_callbacksDisabled || !_currentDef || !_selectedStageIter.IsOk()) return;

    const auto& stage = _currentDef->getStage(getSelectedStageIndex());

	if (findNamedObject<wxRadioButton>(this, "ParticleEditorStageShapeRect")->GetValue())
    {
        stage->setDistributionType(IStageDef::DISTRIBUTION_RECT);
    }
    else if (findNamedObject<wxRadioButton>(this, "ParticleEditorStageShapeCyl")->GetValue())
    {
        stage->setDistributionType(IStageDef::DISTRIBUTION_CYLINDER);
    }
    else if (findNamedObject<wxRadioButton>(this, "ParticleEditorStageSpherical")->GetValue())
    {
        stage->setDistributionType(IStageDef::DISTRIBUTION_SPHERE);
    }

    bool useRingSize = stage->getDistributionType() != IStageDef::DISTRIBUTION_RECT;

    findNamedObject<wxWindow>(this, "ParticleEditorStageRingSize")->Enable(useRingSize);
	findNamedObject<wxSlider>(this, "ParticleEditorStageRingSizeSlider")->Enable(useRingSize);
    findNamedObject<wxStaticText>(this, "ParticleEditorStageRingSizeLabel")->Enable(useRingSize);

    stage->setDistributionParm(0, getSpinButtonValueAsFloat("ParticleEditorStageXSize"));
    stage->setDistributionParm(1, getSpinButtonValueAsFloat("ParticleEditorStageYSize"));
    stage->setDistributionParm(2, getSpinButtonValueAsFloat("ParticleEditorStageZSize"));
    stage->setDistributionParm(3, getSpinButtonValueAsFloat("ParticleEditorStageRingSize"));

    stage->setOffset(string::convert<Vector3>(
		findNamedObject<wxTextCtrl>(this, "ParticleEditorStageOffset")->GetValue().ToStdString()));

    stage->setRandomDistribution(
		findNamedObject<wxCheckBox>(this, "ParticleEditorStageRandomDist")->GetValue());
}

void ParticleEditor::_onDirectionControlsChanged(wxCommandEvent& ev)
{
    if (_callbacksDisabled || !_currentDef || !_selectedStageIter.IsOk()) return;

    const auto& stage = _currentDef->getStage(getSelectedStageIndex());

    if (findNamedObject<wxRadioButton>(this, "ParticleEditorStageCone")->GetValue())
    {
        stage->setDirectionType(IStageDef::DIRECTION_CONE);
        stage->setDirectionParm(0, getSpinButtonValueAsFloat("ParticleEditorStageConeAngle"));
    }
    else if (findNamedObject<wxRadioButton>(this, "ParticleEditorStageOutward")->GetValue())
    {
        stage->setDirectionType(IStageDef::DIRECTION_OUTWARD);
        stage->setDirectionParm(0, getSpinButtonValueAsFloat("ParticleEditorStageUpwardBias"));
    }

	findNamedObject<wxStaticText>(this, "ParticleEditorStageConeAngleLabel")->Enable(
		stage->getDirectionType() == IStageDef::DIRECTION_CONE);
	findNamedObject<wxWindow>(this, "ParticleEditorStageConeAngle")->Enable(
		stage->getDirectionType() == IStageDef::DIRECTION_CONE);
	findNamedObject<wxSlider>(this, "ParticleEditorStageConeAngleSlider")->Enable(
		stage->getDirectionType() == IStageDef::DIRECTION_CONE);
	
	findNamedObject<wxStaticText>(this, "ParticleEditorStageUpwardBiasLabel")->Enable(
		stage->getDirectionType() == IStageDef::DIRECTION_OUTWARD);
	findNamedObject<wxWindow>(this, "ParticleEditorStageUpwardBias")->Enable(
		stage->getDirectionType() == IStageDef::DIRECTION_OUTWARD);
	findNamedObject<wxSlider>(this, "ParticleEditorStageUpwardBiasSlider")->Enable(
		stage->getDirectionType() == IStageDef::DIRECTION_OUTWARD);

    if (findNamedObject<wxRadioButton>(this, "ParticleEditorStageOrientView")->GetValue())
    {
        stage->setOrientationType(IStageDef::ORIENTATION_VIEW);
    }
    else if (findNamedObject<wxRadioButton>(this, "ParticleEditorStageOrientAimed")->GetValue())
    {
        stage->setOrientationType(IStageDef::ORIENTATION_AIMED);

        stage->setOrientationParm(0, getSpinButtonValueAsInt("ParticleEditorStageTrails"));
        stage->setOrientationParm(1, getSpinButtonValueAsFloat("ParticleEditorStageAimedTime"));
    }
    else if (findNamedObject<wxRadioButton>(this, "ParticleEditorStageOrientX")->GetValue())
    {
        stage->setOrientationType(IStageDef::ORIENTATION_X);
    }
    else if (findNamedObject<wxRadioButton>(this, "ParticleEditorStageOrientY")->GetValue())
    {
        stage->setOrientationType(IStageDef::ORIENTATION_Y);
    }
    else if (findNamedObject<wxRadioButton>(this, "ParticleEditorStageOrientZ")->GetValue())
    {
        stage->setOrientationType(IStageDef::ORIENTATION_Z);
    }

	findNamedObject<wxWindow>(this, "ParticleEditorStageTrails")->Enable(
		stage->getOrientationType() == IStageDef::ORIENTATION_AIMED);
	findNamedObject<wxWindow>(this, "ParticleEditorStageTrailsSlider")->Enable(
		stage->getOrientationType() == IStageDef::ORIENTATION_AIMED);
	findNamedObject<wxWindow>(this, "ParticleEditorStageTrailsLabel")->Enable(
		stage->getOrientationType() == IStageDef::ORIENTATION_AIMED);

	findNamedObject<wxWindow>(this, "ParticleEditorStageAimedTime")->Enable(
		stage->getOrientationType() == IStageDef::ORIENTATION_AIMED);
	findNamedObject<wxWindow>(this, "ParticleEditorStageAimedTimeSlider")->Enable(
		stage->getOrientationType() == IStageDef::ORIENTATION_AIMED);
	findNamedObject<wxWindow>(this, "ParticleEditorStageTimeLabel")->Enable(
		stage->getOrientationType() == IStageDef::ORIENTATION_AIMED);

    stage->setInitialAngle(getSpinButtonValueAsFloat("ParticleEditorStageInitialAngle"));
}

void ParticleEditor::_onSizeControlsChanged(wxCommandEvent& ev)
{
    if (_callbacksDisabled || !_currentDef || !_selectedStageIter.IsOk()) return;

    const auto& stage = _currentDef->getStage(getSelectedStageIndex());

    stage->getSize().setFrom(getSpinButtonValueAsFloat("ParticleEditorStageSizeFrom"));
    stage->getSize().setTo(getSpinButtonValueAsFloat("ParticleEditorStageSizeTo"));

    stage->getSpeed().setFrom(getSpinButtonValueAsFloat("ParticleEditorStageSpeedFrom"));
    stage->getSpeed().setTo(getSpinButtonValueAsFloat("ParticleEditorStageSpeedTo"));

    stage->getRotationSpeed().setFrom(getSpinButtonValueAsFloat("ParticleEditorStageRotationSpeedFrom"));
    stage->getRotationSpeed().setTo(getSpinButtonValueAsFloat("ParticleEditorStageRotationSpeedTo"));

    stage->getAspect().setFrom(getSpinButtonValueAsFloat("ParticleEditorStageAspectFrom"));
    stage->getAspect().setTo(getSpinButtonValueAsFloat("ParticleEditorStageAspectTo"));

    stage->setGravity(getSpinButtonValueAsFloat("ParticleEditorStageGravity"));
    stage->setWorldGravityFlag(
		findNamedObject<wxCheckBox>(this, "ParticleEditorStageUseWorldGravity")->GetValue());

    stage->setBoundsExpansion(getSpinButtonValueAsFloat("ParticleEditorStageBoundsExpansion"));
}

void ParticleEditor::_onPathControlsChanged(wxCommandEvent& ev)
{
    if (_callbacksDisabled || !_currentDef || !_selectedStageIter.IsOk()) return;

    const auto& stage = _currentDef->getStage(getSelectedStageIndex());

    if (findNamedObject<wxRadioButton>(this, "ParticleEditorStagePathStandard")->GetValue())
    {
        stage->setCustomPathType(IStageDef::PATH_STANDARD);
    }
    else if (findNamedObject<wxRadioButton>(this, "ParticleEditorStagePathFlies")->GetValue())
    {
        stage->setCustomPathType(IStageDef::PATH_FLIES);

        stage->setCustomPathParm(0, getSpinButtonValueAsFloat("ParticleEditorStageRadialSpeed"));
        stage->setCustomPathParm(1, getSpinButtonValueAsFloat("ParticleEditorStageAxialSpeed"));
        stage->setCustomPathParm(2, getSpinButtonValueAsFloat("ParticleEditorStageSphereRadius"));
    }
    else if (findNamedObject<wxRadioButton>(this, "ParticleEditorStagePathHelix")->GetValue())
    {
        stage->setCustomPathType(IStageDef::PATH_HELIX);

        stage->setCustomPathParm(0, getSpinButtonValueAsFloat("ParticleEditorStageCylSizeX"));
        stage->setCustomPathParm(1, getSpinButtonValueAsFloat("ParticleEditorStageCylSizeY"));
        stage->setCustomPathParm(2, getSpinButtonValueAsFloat("ParticleEditorStageCylSizeZ"));
        stage->setCustomPathParm(3, getSpinButtonValueAsFloat("ParticleEditorStageRadialSpeed"));
        stage->setCustomPathParm(4, getSpinButtonValueAsFloat("ParticleEditorStageAxialSpeed"));
    }

    updatePathWidgetSensitivity();
}

void ParticleEditor::_onDepthHackChanged(wxSpinDoubleEvent& ev)
{
    if (_callbacksDisabled || !_currentDef) return;

    _currentDef->setDepthHack(getSpinButtonValueAsFloat("ParticleEditorDepthHack"));
}

void ParticleEditor::updatePathWidgetSensitivity()
{
    const auto& stage = _currentDef->getStage(getSelectedStageIndex());

    // Sensitivity
    bool useAnySpinner = stage->getCustomPathType() != IStageDef::PATH_STANDARD;
    bool useFlies = stage->getCustomPathType() == IStageDef::PATH_FLIES;

    findNamedObject<wxWindow>(this, "ParticleEditorStageRadialSpeedLabel")->Enable(useAnySpinner);
    findNamedObject<wxWindow>(this, "ParticleEditorStageAxialSpeedLabel")->Enable(useAnySpinner);

    findNamedObject<wxWindow>(this, "ParticleEditorStageRadialSpeed")->Enable(useAnySpinner);
	findNamedObject<wxWindow>(this, "ParticleEditorStageRadialSpeedSlider")->Enable(useAnySpinner);

	findNamedObject<wxWindow>(this, "ParticleEditorStageAxialSpeed")->Enable(useAnySpinner);
	findNamedObject<wxWindow>(this, "ParticleEditorStageAxialSpeedSlider")->Enable(useAnySpinner);
	
    findNamedObject<wxWindow>(this, "ParticleEditorStageSphereRadiusLabel")->Enable(useAnySpinner && useFlies);
    findNamedObject<wxWindow>(this, "ParticleEditorStageSphereRadius")->Enable(useAnySpinner && useFlies);
	findNamedObject<wxWindow>(this, "ParticleEditorStageSphereRadiusSlider")->Enable(useAnySpinner && useFlies);
	
    findNamedObject<wxWindow>(this, "ParticleEditorStageCylSizeXLabel")->Enable(useAnySpinner && !useFlies);
    findNamedObject<wxWindow>(this, "ParticleEditorStageCylSizeYLabel")->Enable(useAnySpinner && !useFlies);
    findNamedObject<wxWindow>(this, "ParticleEditorStageCylSizeZLabel")->Enable(useAnySpinner && !useFlies);
    findNamedObject<wxWindow>(this, "ParticleEditorStageCylSizeX")->Enable(useAnySpinner && !useFlies);
    findNamedObject<wxWindow>(this, "ParticleEditorStageCylSizeY")->Enable(useAnySpinner && !useFlies);
    findNamedObject<wxWindow>(this, "ParticleEditorStageCylSizeZ")->Enable(useAnySpinner && !useFlies);
	findNamedObject<wxWindow>(this, "ParticleEditorStageCylSizeXSlider")->Enable(useAnySpinner && !useFlies);
    findNamedObject<wxWindow>(this, "ParticleEditorStageCylSizeYSlider")->Enable(useAnySpinner && !useFlies);
    findNamedObject<wxWindow>(this, "ParticleEditorStageCylSizeZSlider")->Enable(useAnySpinner && !useFlies);
}

void ParticleEditor::connectSpinner(const std::string& name, MemberMethod func)
{
	wxWindow* spinctrl = findNamedObject<wxWindow>(this, name);

	// By convention, the sliders carry the same name plus a "Slider" suffix
	wxSlider* slider = tryGetNamedObject<wxSlider>(this, name + "Slider");

	// Function object to update the spinctrl
	std::function<void(double value)> updateSpinCtrl;

	if (dynamic_cast<wxSpinCtrl*>(spinctrl) != NULL)
	{
		wxSpinCtrl* spin = static_cast<wxSpinCtrl*>(spinctrl);

		// Regular integer-valued spinctrl
		spinctrl->Bind(wxEVT_SPINCTRL, [=] (wxSpinEvent& ev)
		{
			(this->*func)(ev);

			// Update slider when spinctrl is changing
			if (slider != NULL)
			{
				slider->SetValue(spin->GetValue());
			}
		});

		if (slider != NULL)
		{
			// Slider needs to have the same range as the spinner
			slider->SetRange(spin->GetMin(), spin->GetMax());
		}

		updateSpinCtrl = [=] (double value)
		{
			spin->SetValue(value);
		}; 
	}
	else if (dynamic_cast<wxSpinCtrlDouble*>(spinctrl) != NULL)
	{
		wxSpinCtrlDouble* spin = static_cast<wxSpinCtrlDouble*>(spinctrl);
		int sliderFactor = static_cast<int>(1 / spin->GetIncrement());

		// Float-valued spinctrl
		spinctrl->Bind(wxEVT_SPINCTRLDOUBLE, [=] (wxSpinDoubleEvent& ev)
		{
			(this->*func)(ev);

			// Update slider when spinctrl is changing
			if (slider != NULL)
			{
				slider->SetValue(static_cast<int>(spin->GetValue() * sliderFactor));
			}
		});

		if (slider != NULL)
		{
			// Slider needs to have the same range as the spinner, proportionally
			slider->SetRange(spin->GetMin() * sliderFactor, spin->GetMax() * sliderFactor);
		}

		updateSpinCtrl = [=] (double value)
		{
			spin->SetValue(value / sliderFactor);
		}; 
	}

	if (slider != NULL)
	{
		slider->Bind(wxEVT_SCROLL_CHANGED, [=] (wxScrollEvent& ev)
		{ 
			(this->*func)(ev);

			// Update spinctrl when slider changes
			updateSpinCtrl(slider->GetValue());
			ev.Skip();
		});

		slider->Bind(wxEVT_SCROLL_THUMBTRACK, [=] (wxScrollEvent& ev)
		{ 
			(this->*func)(ev);

			// Update spinctrl when slider changes
			updateSpinCtrl(slider->GetValue());
			ev.Skip();
		});
	}
}

float ParticleEditor::getSpinButtonValueAsFloat(const std::string& widgetName)
{
	wxSpinCtrlDouble* sb = findNamedObject<wxSpinCtrlDouble>(this, widgetName);
	return static_cast<float>(sb->GetValue());
}

int ParticleEditor::getSpinButtonValueAsInt(const std::string& widgetName)
{
	wxSpinCtrl* sb = findNamedObject<wxSpinCtrl>(this, widgetName);
    return sb->GetValue();
}

void ParticleEditor::activateEditPanels()
{
	findNamedObject<wxStaticText>(this, "ParticleEditorStageLabel")->Enable(true);
    findNamedObject<wxStaticText>(this, "ParticleEditorStageSettingsLabel")->Enable(true);
    
    activateSettingsEditPanels();
}

void ParticleEditor::deactivateEditPanels()
{
	findNamedObject<wxStaticText>(this, "ParticleEditorStageLabel")->Enable(false);
    findNamedObject<wxPanel>(this, "ParticleEditorStagePanel")->Enable(false);

    deactivateSettingsEditPanels();
}

void ParticleEditor::activateSettingsEditPanels()
{
	findNamedObject<wxPanel>(this, "ParticleEditorStagePanel")->Enable(true);
    findNamedObject<wxNotebook>(this, "ParticleEditorSettingsNotebook")->Enable(true);
}

void ParticleEditor::deactivateSettingsEditPanels()
{
    findNamedObject<wxStaticText>(this, "ParticleEditorStageSettingsLabel")->Enable(false);
    findNamedObject<wxNotebook>(this, "ParticleEditorSettingsNotebook")->Enable(false);
}

std::size_t ParticleEditor::getSelectedStageIndex()
{
    // Get the selection and store it
	wxDataViewItem item = _stageView->GetSelection();

	if (item.IsOk())
	{
		wxutil::TreeModel::Row row(item, *_stageList);
		int value = row[STAGE_COLS().index].getInteger();

		if (value < 0)
		{
			throw std::logic_error("Invalid stage index stored in model.");
		}

		return value;
	}

    throw std::logic_error("Nothing selected, cannot get selected stage index.");
}

void ParticleEditor::selectStage(std::size_t index)
{
	wxDataViewItem item = _stageList->FindInteger(index, STAGE_COLS().index);
	_stageView->Select(item);
	handleStageSelChanged();
}

void ParticleEditor::setSaveButtonsSensitivity(bool sensitive)
{
    findNamedObject<wxButton>(this, "ParticleEditorSaveDefButton")->Enable(sensitive);
    findNamedObject<wxButton>(this, "ParticleEditorCopyDefButton")->Enable(sensitive);
}

std::string ParticleEditor::getParticleNameFromIter(const wxDataViewItem& item)
{
    if (!item.IsOk())
    {
        return "";
    }
    else
    {
		wxutil::TreeModel::Row row(item, *_defList);
        return row[DEF_COLS().name];
    }
}

void ParticleEditor::_onDefSelChanged(wxDataViewEvent& ev)
{
	handleDefSelChanged();
}

void ParticleEditor::handleDefSelChanged()
{
    // Get the selection and store it
    wxDataViewItem item = _defView->GetSelection();

    if (!promptUserToSaveChanges())
    {
        // Revert the selection (re-enter this function) and cancel the operation
        _defView->Select(_selectedDefIter);
        return;
    }

    if (_selectedDefIter.IsOk() && item.IsOk() && _selectedDefIter == item)
    {
        return; // nothing to do so far
    }

    // Selected particle changed, free the existing edit particle
    releaseEditParticle();

    // Store new selection
    _selectedDefIter = item;

    if (_selectedDefIter.IsOk())
    {
        // Copy the particle def and set it up for editing
        setupEditParticle();

        activateEditPanels();
        setSaveButtonsSensitivity(true);

        // Load particle data
        updateWidgetsFromParticle();
    }
    else
    {
        _preview->ClearPreview();
		_stageView->UnselectAll();
        _selectedStageIter = wxDataViewItem();
        _stageList->Clear();
        deactivateEditPanels();
        setSaveButtonsSensitivity(false);
    }
}

void ParticleEditor::_onStageSelChanged(wxDataViewEvent& ev)
{
	handleStageSelChanged();
}

void ParticleEditor::handleStageSelChanged()
{
    // Get the selection and store it
    wxDataViewItem item = _stageView->GetSelection();

    if (_selectedStageIter && item.IsOk() && _selectedStageIter == item)
    {
        return; // nothing to do so far
    }

    _selectedStageIter = item;

    bool isStageSelected = false;

    if (_selectedStageIter)
    {
        activateSettingsEditPanels();

        // Activate delete, move and toggle buttons
        isStageSelected = true;

		wxutil::TreeModel::Row row(_selectedStageIter, *_stageList);
        std::size_t index = row[STAGE_COLS().index].getInteger();

		findNamedObject<wxButton>(this, "ParticleEditorMoveUpStageButton")->Enable(index > 0);
		findNamedObject<wxButton>(this, "ParticleEditorMoveDownStageButton")->Enable(index < _currentDef->getNumStages() - 1);
    }
    else
    {
        // No valid selection
        deactivateSettingsEditPanels();

        // Deactivate delete, move and toggle buttons
        isStageSelected = false;

		findNamedObject<wxButton>(this, "ParticleEditorMoveUpStageButton")->Enable(false);
		findNamedObject<wxButton>(this, "ParticleEditorMoveDownStageButton")->Enable(false);
    }

	findNamedObject<wxButton>(this, "ParticleEditorRemoveStageButton")->Enable(isStageSelected);
	findNamedObject<wxButton>(this, "ParticleEditorToggleStageButton")->Enable(isStageSelected);
	findNamedObject<wxButton>(this, "ParticleEditorDuplicateStageButton")->Enable(isStageSelected);

    // Reload the current stage data
    updateWidgetsFromStage();
}

void ParticleEditor::_onAddStage(wxCommandEvent& ev)
{
    if (!_currentDef) return;

    // Add a new stage at the end of the list
    std::size_t newStage = _currentDef->addParticleStage();

    reloadStageList();

    selectStage(newStage);
}

void ParticleEditor::_onRemoveStage(wxCommandEvent& ev)
{
    if (!_currentDef || !_selectedStageIter.IsOk()) return;

    _currentDef->removeParticleStage(getSelectedStageIndex());

    reloadStageList();
}

void ParticleEditor::_onToggleStage(wxCommandEvent& ev)
{
    if (!_currentDef || !_selectedStageIter.IsOk()) return;

    std::size_t index = getSelectedStageIndex();

    const auto& stage = _currentDef->getStage(index);

    stage->setVisible(!stage->isVisible());

    reloadStageList();
    selectStage(index);
}

void ParticleEditor::_onMoveUpStage(wxCommandEvent& ev)
{
    if (!_currentDef) return;

    std::size_t selIndex = getSelectedStageIndex();
    assert(selIndex > 0);

    _currentDef->swapParticleStages(selIndex, selIndex - 1);

    reloadStageList();
    selectStage(selIndex - 1);
}

void ParticleEditor::_onMoveDownStage(wxCommandEvent& ev)
{
    if (!_currentDef) return;

    std::size_t selIndex = getSelectedStageIndex();
    assert(_currentDef->getNumStages() > 0 && selIndex < _currentDef->getNumStages() - 1);

    _currentDef->swapParticleStages(selIndex, selIndex + 1);

    reloadStageList();
    selectStage(selIndex + 1);
}

void ParticleEditor::_onDuplicateStage(wxCommandEvent& ev)
{
    if (!_currentDef) return;

    std::size_t srcStageIndex = getSelectedStageIndex();
    std::size_t newStageIndex = _currentDef->addParticleStage();

    const auto& srcStage = _currentDef->getStage(srcStageIndex);
    const auto& newStage = _currentDef->getStage(newStageIndex);

    newStage->copyFrom(srcStage);

    reloadStageList();

    selectStage(newStageIndex);
}

void ParticleEditor::updateWidgetsFromParticle()
{
    if (!_currentDef)
    {
		findNamedObject<wxStaticText>(this, "ParticleEditorSaveNote")->SetLabel("");
        return;
    }

    // Load stages
    reloadStageList();

    _callbacksDisabled = true;

    // Update depth hack
	findNamedObject<wxSpinCtrlDouble>(this, "ParticleEditorDepthHack")->SetValue(_currentDef->getDepthHack());

    _callbacksDisabled = false;

    // Load stage data into controls
    updateWidgetsFromStage();

    // Update outfile label
    std::string origName = getParticleNameFromIter(_selectedDefIter);
    auto origDef = GlobalParticlesManager().getDefByName(origName);

    fs::path outFile = GlobalGameManager().getModPath();
    outFile /= origDef->getBlockSyntax().fileInfo.fullPath();

	findNamedObject<wxStaticText>(this, "ParticleEditorSaveNote")->SetLabelMarkup(
		fmt::format(_("Note: changes will be written to the file <i>{0}</i>"), outFile.string()));
}

void ParticleEditor::reloadStageList()
{
    if (!_currentDef) return;

    // Load stages
	_stageView->UnselectAll();
    _stageList->Clear();
    _selectedStageIter = wxDataViewItem();

    for (std::size_t i = 0; i < _currentDef->getNumStages(); ++i)
    {
        const auto& stage = _currentDef->getStage(i);

        wxutil::TreeModel::Row row = _stageList->AddItem();

		wxDataViewItemAttr colour;
		colour.SetColour(stage->isVisible() ? wxColour(0, 0, 0) : wxColour(127, 127, 127));

        row[STAGE_COLS().name] = fmt::format("Stage {0}", static_cast<int>(i));
		row[STAGE_COLS().name].setAttr(colour);

        row[STAGE_COLS().index] = static_cast<int>(i);
        row[STAGE_COLS().visible] = true;

		row.SendItemAdded();

		// Select the first stage if possible
		if (i == 0)
		{
			_stageView->Select(row.getItem());
			handleStageSelChanged();
		}
    }
}

void ParticleEditor::setSpinCtrlValue(const std::string& name, double value)
{
	wxWindow* spin = findNamedObject<wxWindow>(this, name);
	wxSlider* slider = tryGetNamedObject<wxSlider>(this, name + "Slider");
	
	if (dynamic_cast<wxSpinCtrl*>(spin) != NULL)
	{
		static_cast<wxSpinCtrl*>(spin)->SetValue(static_cast<int>(value));

		if (slider != NULL)
		{
			slider->SetValue(static_cast<int>(value));
		}
	}
	else if (dynamic_cast<wxSpinCtrlDouble*>(spin) != NULL)
	{
		wxSpinCtrlDouble* spinCtrl = static_cast<wxSpinCtrlDouble*>(spin);

		spinCtrl->SetValue(value);

		int sliderFactor = static_cast<int>(1 / spinCtrl->GetIncrement());

		if (slider != NULL)
		{
			slider->SetValue(value * sliderFactor);
		}
	}
}

void ParticleEditor::updateWidgetsFromStage()
{
    if (!_currentDef || !_selectedStageIter.IsOk()) return;

    _callbacksDisabled = true;

    const auto& stage = _currentDef->getStage(getSelectedStageIndex());

	findNamedObject<wxTextCtrl>(this, "ParticleEditorStageShader")->SetValue(stage->getMaterialName());

    const Vector4& colour = stage->getColour();
	findNamedObject<wxTextCtrl>(this, "ParticleEditorStageColour")->SetValue(
        fmt::format("{:.3f} {:.3f} {:.3f} {:.3f}", colour.x(), colour.y(), colour.z(), colour.w()));

	findNamedObject<wxCheckBox>(this, "ParticleEditorStageUseEntityColour")->SetValue(stage->getUseEntityColour());

    const Vector4& fadeColour = stage->getFadeColour();

	findNamedObject<wxTextCtrl>(this, "ParticleEditorStageFadeColour")->SetValue(
		fmt::format("{:.3f} {:.3f} {:.3f} {:.3f}", fadeColour.x(), fadeColour.y(), fadeColour.z(), fadeColour.w()));

	setSpinCtrlValue("ParticleEditorStageFadeInFrac", stage->getFadeInFraction());
	setSpinCtrlValue("ParticleEditorStageFadeOutFrac", stage->getFadeOutFraction());
	setSpinCtrlValue("ParticleEditorStageFadeIdxFrac", stage->getFadeIndexFraction());
	setSpinCtrlValue("ParticleEditorStageAnimFrames", stage->getAnimationFrames());
	setSpinCtrlValue("ParticleEditorStageAnimRate", stage->getAnimationRate());

	setSpinCtrlValue("ParticleEditorStageCount", stage->getCount());
	setSpinCtrlValue("ParticleEditorStageDuration", stage->getDuration());
	setSpinCtrlValue("ParticleEditorStageBunching", stage->getBunching());
	setSpinCtrlValue("ParticleEditorStageCycles", stage->getCycles());
	setSpinCtrlValue("ParticleEditorStageTimeOffset", stage->getTimeOffset());
	setSpinCtrlValue("ParticleEditorStageDeadTime", stage->getDeadTime());

    // DISTRIBUTION

    bool useRingSize = false;

    switch (stage->getDistributionType())
    {
    case IStageDef::DISTRIBUTION_RECT:
        findNamedObject<wxRadioButton>(this, "ParticleEditorStageShapeRect")->SetValue(true);
        break;
    case IStageDef::DISTRIBUTION_CYLINDER:
		findNamedObject<wxRadioButton>(this, "ParticleEditorStageShapeCyl")->SetValue(true);
        useRingSize = true;
        break;
    case IStageDef::DISTRIBUTION_SPHERE:
		findNamedObject<wxRadioButton>(this, "ParticleEditorStageSpherical")->SetValue(true);
        useRingSize = true;
        break;
    };

    findNamedObject<wxWindow>(this, "ParticleEditorStageRingSize")->Enable(useRingSize);
	findNamedObject<wxWindow>(this, "ParticleEditorStageRingSizeSlider")->Enable(useRingSize);
    findNamedObject<wxWindow>(this, "ParticleEditorStageRingSizeLabel")->Enable(useRingSize);

	setSpinCtrlValue("ParticleEditorStageXSize", stage->getDistributionParm(0));
	setSpinCtrlValue("ParticleEditorStageYSize", stage->getDistributionParm(1));
	setSpinCtrlValue("ParticleEditorStageZSize", stage->getDistributionParm(2));
	setSpinCtrlValue("ParticleEditorStageRingSize", stage->getDistributionParm(3));

	findNamedObject<wxTextCtrl>(this, "ParticleEditorStageOffset")->SetValue(string::to_string(stage->getOffset()));
	findNamedObject<wxCheckBox>(this, "ParticleEditorStageRandomDist")->SetValue(stage->getRandomDistribution());

    // DIRECTION / ORIENTATION

    switch (stage->getDirectionType())
    {
    case IStageDef::DIRECTION_CONE:
		findNamedObject<wxRadioButton>(this, "ParticleEditorStageCone")->SetValue(true);
		setSpinCtrlValue("ParticleEditorStageConeAngle", stage->getDirectionParm(0));
        break;
    case IStageDef::DIRECTION_OUTWARD:
        findNamedObject<wxRadioButton>(this, "ParticleEditorStageOutward")->SetValue(true);
		setSpinCtrlValue("ParticleEditorStageUpwardBias", stage->getDirectionParm(0));
        break;
    };

	findNamedObject<wxWindow>(this, "ParticleEditorStageConeAngle")->Enable(stage->getDirectionType() == IStageDef::DIRECTION_CONE);
	findNamedObject<wxWindow>(this, "ParticleEditorStageConeAngleSlider")->Enable(stage->getDirectionType() == IStageDef::DIRECTION_CONE);
	findNamedObject<wxWindow>(this, "ParticleEditorStageConeAngleLabel")->Enable(stage->getDirectionType() == IStageDef::DIRECTION_CONE);

	findNamedObject<wxWindow>(this, "ParticleEditorStageUpwardBias")->Enable(stage->getDirectionType() == IStageDef::DIRECTION_OUTWARD);
    findNamedObject<wxWindow>(this, "ParticleEditorStageUpwardBiasSlider")->Enable(stage->getDirectionType() == IStageDef::DIRECTION_OUTWARD);
	findNamedObject<wxWindow>(this, "ParticleEditorStageUpwardBiasLabel")->Enable(stage->getDirectionType() == IStageDef::DIRECTION_OUTWARD);
	
    // Orientation Type
    switch (stage->getOrientationType())
    {
    case IStageDef::ORIENTATION_VIEW:
		findNamedObject<wxRadioButton>(this, "ParticleEditorStageOrientView")->SetValue(true);
        break;
    case IStageDef::ORIENTATION_AIMED:
		findNamedObject<wxRadioButton>(this, "ParticleEditorStageOrientAimed")->SetValue(true);
		setSpinCtrlValue("ParticleEditorStageTrails", stage->getOrientationParm(0));
		setSpinCtrlValue("ParticleEditorStageAimedTime", stage->getOrientationParm(1));
        break;
    case IStageDef::ORIENTATION_X:
		findNamedObject<wxRadioButton>(this, "ParticleEditorStageOrientX")->SetValue(true);
        break;
    case IStageDef::ORIENTATION_Y:
		findNamedObject<wxRadioButton>(this, "ParticleEditorStageOrientY")->SetValue(true);
        break;
    case IStageDef::ORIENTATION_Z:
		findNamedObject<wxRadioButton>(this, "ParticleEditorStageOrientZ")->SetValue(true);
        break;
    };

	findNamedObject<wxWindow>(this, "ParticleEditorStageTrails")->Enable(
		stage->getOrientationType() == IStageDef::ORIENTATION_AIMED);
	findNamedObject<wxWindow>(this, "ParticleEditorStageTrailsSlider")->Enable(
		stage->getOrientationType() == IStageDef::ORIENTATION_AIMED);
	findNamedObject<wxWindow>(this, "ParticleEditorStageTrailsLabel")->Enable(
		stage->getOrientationType() == IStageDef::ORIENTATION_AIMED);

	findNamedObject<wxWindow>(this, "ParticleEditorStageAimedTime")->Enable(
		stage->getOrientationType() == IStageDef::ORIENTATION_AIMED);
	findNamedObject<wxWindow>(this, "ParticleEditorStageAimedTimeSlider")->Enable(
		stage->getOrientationType() == IStageDef::ORIENTATION_AIMED);
	findNamedObject<wxWindow>(this, "ParticleEditorStageTimeLabel")->Enable(
		stage->getOrientationType() == IStageDef::ORIENTATION_AIMED);

	setSpinCtrlValue("ParticleEditorStageInitialAngle", stage->getInitialAngle());

    // SIZE / SPEED / ASPECT

	setSpinCtrlValue("ParticleEditorStageSizeFrom", stage->getSize().getFrom());
	setSpinCtrlValue("ParticleEditorStageSizeTo", stage->getSize().getTo());

	setSpinCtrlValue("ParticleEditorStageSpeedFrom", stage->getSpeed().getFrom());
	setSpinCtrlValue("ParticleEditorStageSpeedTo", stage->getSpeed().getTo());

	setSpinCtrlValue("ParticleEditorStageRotationSpeedFrom", stage->getRotationSpeed().getFrom());
	setSpinCtrlValue("ParticleEditorStageRotationSpeedTo", stage->getRotationSpeed().getTo());

	setSpinCtrlValue("ParticleEditorStageAspectFrom", stage->getAspect().getFrom());
	setSpinCtrlValue("ParticleEditorStageAspectTo", stage->getAspect().getTo());

	setSpinCtrlValue("ParticleEditorStageGravity", stage->getGravity());
	findNamedObject<wxCheckBox>(this, "ParticleEditorStageUseWorldGravity")->SetValue(stage->getWorldGravityFlag());

	setSpinCtrlValue("ParticleEditorStageBoundsExpansion", stage->getBoundsExpansion());

    // PATH

    switch (stage->getCustomPathType())
    {
    case IStageDef::PATH_STANDARD:
		findNamedObject<wxRadioButton>(this, "ParticleEditorStagePathStandard")->SetValue(true);
        break;
    case IStageDef::PATH_FLIES:
		findNamedObject<wxRadioButton>(this, "ParticleEditorStagePathFlies")->SetValue(true);

		setSpinCtrlValue("ParticleEditorStageRadialSpeed", stage->getCustomPathParm(0));
		setSpinCtrlValue("ParticleEditorStageAxialSpeed", stage->getCustomPathParm(1));
		setSpinCtrlValue("ParticleEditorStageSphereRadius", stage->getCustomPathParm(2));
        break;
    case IStageDef::PATH_HELIX:
		findNamedObject<wxRadioButton>(this, "ParticleEditorStagePathHelix")->SetValue(true);

		setSpinCtrlValue("ParticleEditorStageRadialSpeed", stage->getCustomPathParm(3));
		setSpinCtrlValue("ParticleEditorStageAxialSpeed", stage->getCustomPathParm(4));
		
		setSpinCtrlValue("ParticleEditorStageCylSizeX", stage->getCustomPathParm(0));
		setSpinCtrlValue("ParticleEditorStageCylSizeY", stage->getCustomPathParm(1));
		setSpinCtrlValue("ParticleEditorStageCylSizeZ", stage->getCustomPathParm(2));
        break;
    default:
        rWarning() << "This custom particle path type is not supported." << std::endl;
        break;
    };

    updatePathWidgetSensitivity();

    _callbacksDisabled = false;
}

void ParticleEditor::setupEditParticle()
{
    wxDataViewItem item = _defView->GetSelection();
    if (!item.IsOk()) return;

    // Get the def for the selected particle system if it exists
    std::string selectedName = getParticleNameFromIter(item);
    IParticleDef::Ptr def = GlobalParticlesManager().getDefByName(selectedName);
    if (!def)
    {
        _preview->ClearPreview();
        return;
    }

    // Generate a temporary name for this particle, and instantiate a copy
    std::string temporaryParticleName = selectedName + EDIT_SUFFIX;

    _currentDef = GlobalParticlesManager().findOrInsertParticleDef(temporaryParticleName);
    // Set the edit particle to an empty file info, it will be filled on saving
    _currentDef->setFileInfo(vfs::FileInfo());

    _currentDef->copyFrom(def);

    // Point the preview to this temporary particle def
    _preview->SetPreviewDeclName(_currentDef->getDeclName());
}

void ParticleEditor::releaseEditParticle()
{
    if (_currentDef && string::ends_with(_currentDef->getDeclName(), EDIT_SUFFIX))
    {
        GlobalParticlesManager().removeParticleDef(_currentDef->getDeclName());
    }

    _currentDef.reset();
}

bool ParticleEditor::particleHasUnsavedChanges()
{
    if (_selectedDefIter && _currentDef)
    {
        // Particle selection changed, check if we have any unsaved changes
        std::string origName = getParticleNameFromIter(_selectedDefIter);

        IParticleDef::Ptr origDef = GlobalParticlesManager().getDefByName(origName);

        if (!origDef || !_currentDef->isEqualTo(origDef))
        {
            return true;
        }
    }
    return false;
}

IDialog::Result ParticleEditor::askForSave()
{
    // Get the original particle name
    std::string origName = getParticleNameFromIter(_selectedDefIter);

    // Does not make sense to save a null particle
    assert(!origName.empty());

    // The particle we're editing has been changed from the saved one
    wxutil::Messagebox box(_("Save Changes"),
        fmt::format(_("Do you want to save the changes\nyou made to the particle {0}?"), origName),
        IDialog::MESSAGE_SAVECONFIRMATION);

    return box.run();
}

void ParticleEditor::_onSaveParticle(wxCommandEvent& ev)
{
	saveCurrentParticle();
}

bool ParticleEditor::saveCurrentParticle()
{
    // Get the original particle name
    std::string origName = getParticleNameFromIter(_selectedDefIter);

    IParticleDef::Ptr origDef = GlobalParticlesManager().getDefByName(origName);

    // This should really succeed, we can't have non-existing particles selected
    // in the treeview
    assert(origDef);

    // Write the changes from the working copy into the actual instance
    origDef->copyFrom(_currentDef);

    // Write changes to disk, and return the result
    try
    {
        GlobalParticlesManager().saveParticleDef(origDef->getDeclName());
        return true;
    }
    catch (std::runtime_error& err)
    {
        std::string errMsg = fmt::format(_("Error saving particle definition:\n{0}"), err.what());

        rError() << errMsg << std::endl;

        wxutil::Messagebox::ShowError(errMsg, this);

        return false;
    }
}

int ParticleEditor::ShowModal()
{
	// Restore the position
    _windowPosition.applyPosition();

	int returnCode = DialogBase::ShowModal();

	// Tell the position tracker to save the information
    _windowPosition.saveToPath(RKEY_WINDOW_STATE);

    // Free the edit particle before hiding this dialog
    releaseEditParticle();

	return returnCode;
}

void ParticleEditor::_onClose(wxCommandEvent& ev)
{
    if (!promptUserToSaveChanges(false)) return; // action not allowed or cancelled

    // Close the window
    EndModal(wxID_CLOSE);
}

bool ParticleEditor::defSelectionHasChanged()
{
    // Check if the selection has changed
    wxDataViewItem item = _defView->GetSelection();

    bool changed;
    if (!_selectedDefIter.IsOk())
    {
        changed = item.IsOk();
    }
    else if (!item.IsOk()) // _selectedDefIter is valid
    {
        changed = true;
    }
    else // both iter and _selectedDefIter are valid
    {
        changed = (_selectedDefIter != item);
    }

    return changed;
}

bool ParticleEditor::promptUserToSaveChanges(bool requireSelectionChange)
{
    // Do not prompt if we are already in the middle of a save operation (which
    // currently results in several selection changes that will trigger this
    // method).
    if (_saveInProgress) return true;

    // On close requests we don't require the selection to have changed
    if ((!requireSelectionChange || defSelectionHasChanged()) && particleHasUnsavedChanges())
    {
        IDialog::Result result = askForSave();

        if (result == IDialog::RESULT_YES)
        {
            // User wants to save
            if (!saveCurrentParticle())
            {
                return false; // save attempt failed
            }

            // Save successful, go ahead
        }
        else if (result == IDialog::RESULT_CANCELLED)
        {
            return false; // user cancelled
        }

        // User doesn't want to save
    }

    return true;
}

void ParticleEditor::_onNewParticle(wxCommandEvent& ev)
{
    // Check for unsaved changes, don't require a selection change
    if (!promptUserToSaveChanges(false)) return; // action not allowed or cancelled

    createAndSelectNewParticle();
}

IParticleDef::Ptr ParticleEditor::createAndSelectNewParticle()
{
    std::string particleName = queryNewParticleName();

    if (particleName.empty())
    {
        return IParticleDef::Ptr(); // no valid name, abort
    }

    std::string destFile = queryParticleFile();

    if (destFile.empty())
    {
        return IParticleDef::Ptr(); // no valid destination file
    }

    // Good filename, good destination file, we're set to go
    auto particle = GlobalParticlesManager().findOrInsertParticleDef(particleName);

    particle->setFilename("particles/" + destFile);

    // Re-load the particles list
    populateParticleDefList();

    // Highlight our new particle
    selectParticleDef(particle->getDeclName());

    return particle;
}

std::string ParticleEditor::queryParticleFile()
{
    // Get the filename we should save this particle into
    wxutil::FileChooser chooser(this, _("Select .prt file"), false, "particle", ".prt");

    fs::path modParticlesPath = GlobalGameManager().getModPath();
    modParticlesPath /= "particles";

    if (!os::fileOrDirExists(modParticlesPath.string()))
    {
        rMessage() << "Ensuring mod particles path: " << modParticlesPath << std::endl;
        fs::create_directories(modParticlesPath);
    }

    // Point the file chooser to that new file
    chooser.setCurrentPath(GlobalGameManager().getModPath() + "/particles");
    chooser.askForOverwrite(false);

    std::string result = chooser.display();

    return !result.empty() ? os::getFilename(os::standardPath(result)) : "";
}

std::string ParticleEditor::queryNewParticleName()
{
    // It's ok after this point to create a new particle
    while (true)
    {
        // Query the name of the new particle from the user
        std::string particleName;

        if (particleName.empty())
        {
            try
            {
                particleName = wxutil::Dialog::TextEntryDialog(
                    _("Enter Name"),
                    _("Enter Particle Name"),
                    "",
                    this
                );
            }
            catch (wxutil::EntryAbortedException&)
            {
                break;
            }
        }

        if (particleName.empty())
        {
            // Wrong name, let the user try again
            wxutil::Messagebox::ShowError(_("Cannot create particle with an empty name."));
            continue;
        }

        // Check if this particle already exists
        IParticleDef::Ptr existing = GlobalParticlesManager().getDefByName(particleName);

        if (existing == NULL)
        {
            // Success, return that name
            return particleName;
        }
        else
        {
            // Wrong name, let the user try again
            wxutil::Messagebox::ShowError(_("This name is already in use."));
            continue;
        }
    }

    return ""; // no successful entry
}

void ParticleEditor::_onCloneCurrentParticle(wxCommandEvent& ev)
{
    util::ScopedBoolLock lock(_saveInProgress);

    // Get the original particle name
    std::string origName = getParticleNameFromIter(_selectedDefIter);

    if (origName.empty())
    {
        return;
    }

    // Look up the original particle def
    IParticleDef::Ptr original = GlobalParticlesManager().getDefByName(origName);

    // Create a new particle (this will already set up an edit particle, which is empty)
    auto newParticle = createAndSelectNewParticle();

    if (!newParticle)
    {
        return;
    }

    // Copy stuff from original particle
    newParticle->copyFrom(original);

    // Clear selection and re-select the particle to set up the working copy
    _defView->UnselectAll();
    _selectedDefIter = wxDataViewItem(); // to force re-setup of the selected edit particle
    _preview->ClearPreview(); // Preview might hold old data as well

    selectParticleDef(newParticle->getDeclName());

    // Save the new particle declaration to the file immediately
    saveCurrentParticle();

    // Reload controls
    updateWidgetsFromParticle();
}

void ParticleEditor::DisplayDialog(const cmd::ArgumentList& args)
{
    ParticleEditor* editor = new ParticleEditor;

    editor->ShowModal();
	editor->Destroy();
}

}
