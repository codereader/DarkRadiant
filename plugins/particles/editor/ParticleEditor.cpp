#include "ParticleEditor.h"

#include "i18n.h"
#include "imainframe.h"
#include "iuimanager.h"
#include "iparticles.h"
#include "igame.h"

#include "gtkutil/MultiMonitor.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/FileChooser.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/dialog/MessageBox.h"
#include "gtkutil/EntryAbortedException.h"

#include <gtkmm/button.h>
#include <gtkmm/paned.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/treeview.h>
#include <gtkmm/stock.h>

#include "../ParticlesManager.h"

#include "os/path.h"
#include "registry/bind.h"
#include "util/ScopedBoolLock.h"

#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>

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
        public Gtk::TreeModel::ColumnRecord
    {
        DefColumns() { add(name); }

        Gtk::TreeModelColumn<std::string> name;
    };
    DefColumns& DEF_COLS() { static DefColumns _i; return _i; }

    // Columns for the stages list
    struct StageColumns :
        public Gtk::TreeModel::ColumnRecord
    {
        StageColumns() { add(name); add(index); add(visible); add(colour); }

        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<int> index;
        Gtk::TreeModelColumn<bool> visible;
        Gtk::TreeModelColumn<Glib::ustring> colour;
    };
    StageColumns& STAGE_COLS() { static StageColumns _i; return _i; }
}

ParticleEditor::ParticleEditor() :
    gtkutil::BlockingTransientWindow(DIALOG_TITLE, GlobalMainFrame().getTopLevelWindow()),
    gtkutil::GladeWidgetHolder("ParticleEditor.glade"),
    _defList(Gtk::ListStore::create(DEF_COLS())),
    _stageList(Gtk::ListStore::create(STAGE_COLS())),
    _preview(new gtkutil::ParticlePreview),
    _callbacksDisabled(false),
    _saveInProgress(false)
{
    // Window properties
    set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
    set_position(Gtk::WIN_POS_CENTER_ON_PARENT);

    // Add vbox to dialog
    add(*gladeWidget<Gtk::Widget>("mainVbox"));
    g_assert(get_child() != NULL);

    // Wire up the close button
    gladeWidget<Gtk::Button>("closeButton")->signal_clicked().connect(
        sigc::mem_fun(*this, &ParticleEditor::_onClose)
    );

    gladeWidget<Gtk::Button>("newParticleButon")->signal_clicked().connect(
        sigc::mem_fun(*this, &ParticleEditor::_onNewParticle)
    );
    gladeWidget<Gtk::Button>("saveParticleButton")->signal_clicked().connect(
        sigc::hide_return(
            sigc::mem_fun(*this, &ParticleEditor::saveCurrentParticle)
        )
    );
    gladeWidget<Gtk::Button>("cloneParticleButton")->signal_clicked().connect(
        sigc::mem_fun(*this, &ParticleEditor::cloneCurrentParticle)
    );

    // Set the default size of the window
    const Glib::RefPtr<Gtk::Window>& mainWindow = GlobalMainFrame().getTopLevelWindow();

    Gdk::Rectangle rect = gtkutil::MultiMonitor::getMonitorForWindow(mainWindow);
    int height = static_cast<int>(rect.get_height() * 0.6f);

    set_default_size(static_cast<int>(rect.get_width() * 0.6f), height);

    // Setup the splitter and preview
    _preview->setSize(static_cast<int>(rect.get_width() * 0.3f), -1);
    Gtk::Paned* splitter = gladeWidget<Gtk::Paned>("mainPane");
    splitter->add2(*_preview);
    registry::bindPropertyToKey(splitter->property_position(), RKEY_SPLIT_POS);

    // Connect the window position tracker
    _windowPosition.loadFromPath(RKEY_WINDOW_STATE);
    _windowPosition.connect(this);
    _windowPosition.applyPosition();

    setupParticleDefList();
    setupParticleStageList();
    setupSettingsPages();

    // Fire the selection changed signal to initialise the sensitivity
    _onDefSelChanged();
    _onStageSelChanged();
}

void ParticleEditor::_onDeleteEvent()
{
    if (!promptUserToSaveChanges(false))    return; // action not allowed or cancelled

    // Window destruction allowed, pass to base class => triggers destroy
    BlockingTransientWindow::_onDeleteEvent();
}

void ParticleEditor::setupParticleDefList()
{
    Gtk::TreeView* view = gladeWidget<Gtk::TreeView>("definitionView");

    view->set_model(_defList);
    view->set_headers_visible(false);

    // Single text column
    view->append_column(_("Particle"), DEF_COLS().name);

    // Apply full-text search to the column
    view->set_search_equal_func(sigc::ptr_fun(gtkutil::TreeModel::equalFuncStringContains));

    populateParticleDefList();

    // Connect up the selection changed callback
    _defSelection = view->get_selection();
    _defSelection->signal_changed().connect(
        sigc::mem_fun(*this, &ParticleEditor::_onDefSelChanged)
    );
}

namespace
{

/**
 * Visitor class to retrieve particle system names and add them to a
 * GtkListStore.
 */
class ParticlesVisitor
{
    // List store to populate
    Glib::RefPtr<Gtk::ListStore> _store;

public:

    /**
     * Constructor.
     */
    ParticlesVisitor(const Glib::RefPtr<Gtk::ListStore>& store)
    : _store(store)
    {}

    /**
     * Functor operator.
     */
    void operator() (const IParticleDef& def)
    {
        // Add the Def name to the list store
        Gtk::TreeModel::iterator iter = _store->append();

        Gtk::TreeModel::Row row = *iter;
        row[DEF_COLS().name] = def.getName();
    }
};

} // namespace

void ParticleEditor::populateParticleDefList()
{
    _selectedDefIter = Gtk::TreeModel::iterator();
    _defList->clear();

    // Create and use a ParticlesVisitor to populate the list
    ParticlesVisitor visitor(_defList);
    GlobalParticlesManager().forEachParticleDef(visitor);
}

void ParticleEditor::selectParticleDef(const std::string& particleDefName)
{
    gtkutil::TreeModel::findAndSelectString(
        gladeWidget<Gtk::TreeView>("definitionView"), particleDefName, DEF_COLS().name
    );
}

void ParticleEditor::setupParticleStageList()
{
    Gtk::TreeView* view = gladeWidget<Gtk::TreeView>("stageView");

    view->set_model(_stageList);
    view->set_headers_visible(false);

    // Single text column
    view->append_column(*Gtk::manage(new gtkutil::ColouredTextColumn(_("Stage"), STAGE_COLS().name, STAGE_COLS().colour, false)));

    // Connect up the selection changed callback
    _stageSelection = view->get_selection();
    _stageSelection->signal_changed().connect(sigc::mem_fun(*this, &ParticleEditor::_onStageSelChanged));

    // Connect the stage control buttons
    gladeWidget<Gtk::Button>("addStageButton")->signal_clicked().connect(
        sigc::mem_fun(*this, &ParticleEditor::_onAddStage));
    gladeWidget<Gtk::Button>("removeStageButton")->signal_clicked().connect(
        sigc::mem_fun(*this, &ParticleEditor::_onRemoveStage));
    gladeWidget<Gtk::Button>("toggleStageButton")->signal_clicked().connect(
        sigc::mem_fun(*this, &ParticleEditor::_onToggleStage));
    gladeWidget<Gtk::Button>("moveStageUpButton")->signal_clicked().connect(
        sigc::mem_fun(*this, &ParticleEditor::_onMoveUpStage));
    gladeWidget<Gtk::Button>("moveStageDownButton")->signal_clicked().connect(
        sigc::mem_fun(*this, &ParticleEditor::_onMoveDownStage));
    gladeWidget<Gtk::Button>("duplicateStageButton")->signal_clicked().connect(
        sigc::mem_fun(*this, &ParticleEditor::_onDuplicateStage));

    gladeWidget<Gtk::Button>("duplicateStageButton")->set_image(*Gtk::manage(new Gtk::Image(Gtk::Stock::COPY, Gtk::ICON_SIZE_BUTTON)));
}

void ParticleEditor::setupSettingsPages()
{
    // Depth Hack

    connectSpinner("depthHackSpinner", &ParticleEditor::_onDepthHackChanged);

    // SHADER

    gladeWidget<Gtk::Entry>("shaderEntry")->signal_changed().connect(
        sigc::mem_fun(*this, &ParticleEditor::_onShaderControlsChanged));
    gladeWidget<Gtk::Entry>("colourEntry")->signal_changed().connect(
        sigc::mem_fun(*this, &ParticleEditor::_onShaderControlsChanged));
    gladeWidget<Gtk::Entry>("fadeColourEntry")->signal_changed().connect(
        sigc::mem_fun(*this, &ParticleEditor::_onShaderControlsChanged));
    gladeWidget<Gtk::CheckButton>("useEntityColour")->signal_toggled().connect(
        sigc::mem_fun(*this, &ParticleEditor::_onShaderControlsChanged));

    connectSpinner("fadeInFractionSpinner", &ParticleEditor::_onShaderControlsChanged);
    connectSpinner("fadeOutFractionSpinner", &ParticleEditor::_onShaderControlsChanged);
    connectSpinner("fadeIndexFractionSpinner", &ParticleEditor::_onShaderControlsChanged);
    connectSpinner("animFramesSpinner", &ParticleEditor::_onShaderControlsChanged);
    connectSpinner("animRateSpinner", &ParticleEditor::_onShaderControlsChanged);

    // COUNT

    connectSpinner("countSpinner", &ParticleEditor::_onCountTimeControlsChanged);
    connectSpinner("timeSpinner", &ParticleEditor::_onCountTimeControlsChanged);
    connectSpinner("bunchingSpinner", &ParticleEditor::_onCountTimeControlsChanged);
    connectSpinner("cyclesSpinner", &ParticleEditor::_onCountTimeControlsChanged);
    connectSpinner("timeOffsetSpinner", &ParticleEditor::_onCountTimeControlsChanged);
    connectSpinner("deadTimeSpinner", &ParticleEditor::_onCountTimeControlsChanged);

    // DISTRIBUTION

    gladeWidget<Gtk::RadioButton>("distRectangle")->signal_toggled().connect(
        sigc::mem_fun(*this, &ParticleEditor::_onDistributionControlsChanged));
    gladeWidget<Gtk::RadioButton>("distCylinder")->signal_toggled().connect(
        sigc::mem_fun(*this, &ParticleEditor::_onDistributionControlsChanged));
    gladeWidget<Gtk::RadioButton>("distSphere")->signal_toggled().connect(
        sigc::mem_fun(*this, &ParticleEditor::_onDistributionControlsChanged));

    connectSpinner("distSizeXSpinner", &ParticleEditor::_onDistributionControlsChanged);
    connectSpinner("distSizeYSpinner", &ParticleEditor::_onDistributionControlsChanged);
    connectSpinner("distSizeZSpinner", &ParticleEditor::_onDistributionControlsChanged);
    connectSpinner("distSizeRingSpinner", &ParticleEditor::_onDistributionControlsChanged);

    gladeWidget<Gtk::Entry>("distOffsetEntry")->signal_changed().connect(
        sigc::mem_fun(*this, &ParticleEditor::_onDistributionControlsChanged));

    gladeWidget<Gtk::CheckButton>("distRandom")->signal_toggled().connect(
        sigc::mem_fun(*this, &ParticleEditor::_onDistributionControlsChanged));

    // DIRECTION / ORIENTATION

    gladeWidget<Gtk::RadioButton>("directionCone")->signal_toggled().connect(
        sigc::mem_fun(*this, &ParticleEditor::_onDirectionControlsChanged));
    gladeWidget<Gtk::RadioButton>("directionOutward")->signal_toggled().connect(
        sigc::mem_fun(*this, &ParticleEditor::_onDirectionControlsChanged));

    connectSpinner("coneAngleSpinner", &ParticleEditor::_onDirectionControlsChanged);
    connectSpinner("upwardBiasSpinner", &ParticleEditor::_onDirectionControlsChanged);

    gladeWidget<Gtk::RadioButton>("orientationView")->signal_toggled().connect(
        sigc::mem_fun(*this, &ParticleEditor::_onDirectionControlsChanged));
    gladeWidget<Gtk::RadioButton>("orientationAimed")->signal_toggled().connect(
        sigc::mem_fun(*this, &ParticleEditor::_onDirectionControlsChanged));
    gladeWidget<Gtk::RadioButton>("orientationX")->signal_toggled().connect(
        sigc::mem_fun(*this, &ParticleEditor::_onDirectionControlsChanged));
    gladeWidget<Gtk::RadioButton>("orientationY")->signal_toggled().connect(
        sigc::mem_fun(*this, &ParticleEditor::_onDirectionControlsChanged));
    gladeWidget<Gtk::RadioButton>("orientationZ")->signal_toggled().connect(
        sigc::mem_fun(*this, &ParticleEditor::_onDirectionControlsChanged));

    connectSpinner("aimedTrailsSpinner", &ParticleEditor::_onDirectionControlsChanged);
    connectSpinner("aimedTimeSpinner", &ParticleEditor::_onDirectionControlsChanged);
    connectSpinner("initialAngleSpinner", &ParticleEditor::_onDirectionControlsChanged);

    // SIZE / SPEED / ASPECT

    connectSpinner("sizeFromSpinner", &ParticleEditor::_onSizeControlsChanged);
    connectSpinner("sizeToSpinner", &ParticleEditor::_onSizeControlsChanged);
    connectSpinner("speedFromSpinner", &ParticleEditor::_onSizeControlsChanged);
    connectSpinner("speedToSpinner", &ParticleEditor::_onSizeControlsChanged);
    connectSpinner("rotationSpeedFromSpinner", &ParticleEditor::_onSizeControlsChanged);
    connectSpinner("rotationSpeedToSpinner", &ParticleEditor::_onSizeControlsChanged);
    connectSpinner("aspectFromSpinner", &ParticleEditor::_onSizeControlsChanged);
    connectSpinner("aspectToSpinner", &ParticleEditor::_onSizeControlsChanged);
    connectSpinner("gravitySpinner", &ParticleEditor::_onSizeControlsChanged);
    connectSpinner("boundsExpansionSpinner", &ParticleEditor::_onSizeControlsChanged);

    gladeWidget<Gtk::CheckButton>("useWorldGravity")->signal_toggled().connect(
        sigc::mem_fun(*this, &ParticleEditor::_onSizeControlsChanged));

    // PATH

    gladeWidget<Gtk::RadioButton>("pathStandard")->signal_toggled().connect(
        sigc::mem_fun(*this, &ParticleEditor::_onPathControlsChanged));
    gladeWidget<Gtk::RadioButton>("pathFlies")->signal_toggled().connect(
        sigc::mem_fun(*this, &ParticleEditor::_onPathControlsChanged));
    gladeWidget<Gtk::RadioButton>("pathHelix")->signal_toggled().connect(
        sigc::mem_fun(*this, &ParticleEditor::_onPathControlsChanged));

    connectSpinner("pathRadialSpeedSpinner", &ParticleEditor::_onPathControlsChanged);
    connectSpinner("pathAxialSpeedSpinner", &ParticleEditor::_onPathControlsChanged);
    connectSpinner("pathRadiusSpinner", &ParticleEditor::_onPathControlsChanged);
    connectSpinner("pathSizeXSpinner", &ParticleEditor::_onPathControlsChanged);
    connectSpinner("pathSizeYSpinner", &ParticleEditor::_onPathControlsChanged);
    connectSpinner("pathSizeZSpinner", &ParticleEditor::_onPathControlsChanged);
}

void ParticleEditor::_onShaderControlsChanged()
{
    if (_callbacksDisabled || !_currentDef || !_selectedStageIter) return;

    IStageDef& stage = _currentDef->getStage(getSelectedStageIndex());

    std::string material = gladeWidget<Gtk::Entry>("shaderEntry")->get_text();

    // Only assign a new material if it has actually changed, otherwise the whole particle gets re-shuffled
    if (material != stage.getMaterialName())
    {
        stage.setMaterialName(gladeWidget<Gtk::Entry>("shaderEntry")->get_text());
    }

    stage.setColour(
        string::convert<Vector4>(
            gladeWidget<Gtk::Entry>("colourEntry")->get_text()
        )
    );
    stage.setUseEntityColour(gladeWidget<Gtk::CheckButton>("useEntityColour")->get_active());
    stage.setFadeColour(
        string::convert<Vector4>(
            gladeWidget<Gtk::Entry>("fadeColourEntry")->get_text()
        )
    );
    stage.setFadeInFraction(getSpinButtonValueAsFloat("fadeInFractionSpinner"));
    stage.setFadeOutFraction(getSpinButtonValueAsFloat("fadeOutFractionSpinner"));
    stage.setFadeIndexFraction(getSpinButtonValueAsFloat("fadeIndexFractionSpinner"));
    stage.setAnimationFrames(getSpinButtonValueAsInt("animFramesSpinner"));
    stage.setAnimationRate(getSpinButtonValueAsFloat("animRateSpinner"));
}

void ParticleEditor::_onCountTimeControlsChanged()
{
    if (_callbacksDisabled || !_currentDef || !_selectedStageIter) return;

    IStageDef& stage = _currentDef->getStage(getSelectedStageIndex());

    stage.setCount(getSpinButtonValueAsInt("countSpinner"));
    stage.setDuration(getSpinButtonValueAsFloat("timeSpinner"));
    stage.setBunching(getSpinButtonValueAsFloat("bunchingSpinner"));
    stage.setCycles(getSpinButtonValueAsFloat("cyclesSpinner"));
    stage.setTimeOffset(getSpinButtonValueAsFloat("timeOffsetSpinner"));
    stage.setDeadTime(getSpinButtonValueAsFloat("deadTimeSpinner"));
}

void ParticleEditor::_onDistributionControlsChanged()
{
    if (_callbacksDisabled || !_currentDef || !_selectedStageIter) return;

    IStageDef& stage = _currentDef->getStage(getSelectedStageIndex());

    if (gladeWidget<Gtk::RadioButton>("distRectangle")->get_active())
    {
        stage.setDistributionType(IStageDef::DISTRIBUTION_RECT);
    }
    else if (gladeWidget<Gtk::RadioButton>("distCylinder")->get_active())
    {
        stage.setDistributionType(IStageDef::DISTRIBUTION_CYLINDER);
    }
    else if (gladeWidget<Gtk::RadioButton>("distSphere")->get_active())
    {
        stage.setDistributionType(IStageDef::DISTRIBUTION_SPHERE);
    }

    bool useRingSize = stage.getDistributionType() != IStageDef::DISTRIBUTION_RECT;

    gladeWidget<Gtk::Widget>("distSizeRingHBox")->set_sensitive(useRingSize);
    gladeWidget<Gtk::Widget>("distSizeRingLabel")->set_sensitive(useRingSize);

    stage.setDistributionParm(0, getSpinButtonValueAsFloat("distSizeXSpinner"));
    stage.setDistributionParm(1, getSpinButtonValueAsFloat("distSizeYSpinner"));
    stage.setDistributionParm(2, getSpinButtonValueAsFloat("distSizeZSpinner"));
    stage.setDistributionParm(3, getSpinButtonValueAsFloat("distSizeRingSpinner"));

    stage.setOffset(
        string::convert<Vector3>(
            gladeWidget<Gtk::Entry>("distOffsetEntry")->get_text()
        )
    );

    stage.setRandomDistribution(gladeWidget<Gtk::CheckButton>("distRandom")->get_active());
}

void ParticleEditor::_onDirectionControlsChanged()
{
    if (_callbacksDisabled || !_currentDef || !_selectedStageIter) return;

    IStageDef& stage = _currentDef->getStage(getSelectedStageIndex());

    if (gladeWidget<Gtk::RadioButton>("directionCone")->get_active())
    {
        stage.setDirectionType(IStageDef::DIRECTION_CONE);
        stage.setDirectionParm(0, getSpinButtonValueAsFloat("coneAngleSpinner"));
    }
    else if (gladeWidget<Gtk::RadioButton>("directionOutward")->get_active())
    {
        stage.setDirectionType(IStageDef::DIRECTION_OUTWARD);
        stage.setDirectionParm(0, getSpinButtonValueAsFloat("upwardBiasSpinner"));
    }

    gladeWidget<Gtk::Widget>("coneAngleHBox")->set_sensitive(stage.getDirectionType() == IStageDef::DIRECTION_CONE);
    gladeWidget<Gtk::Widget>("upwardBiasHBox")->set_sensitive(stage.getDirectionType() == IStageDef::DIRECTION_OUTWARD);

    if (gladeWidget<Gtk::RadioButton>("orientationView")->get_active())
    {
        stage.setOrientationType(IStageDef::ORIENTATION_VIEW);
    }
    else if (gladeWidget<Gtk::RadioButton>("orientationAimed")->get_active())
    {
        stage.setOrientationType(IStageDef::ORIENTATION_AIMED);

        stage.setOrientationParm(0, getSpinButtonValueAsFloat("aimedTrailsSpinner"));
        stage.setOrientationParm(1, getSpinButtonValueAsFloat("aimedTimeSpinner"));
    }
    else if (gladeWidget<Gtk::RadioButton>("orientationX")->get_active())
    {
        stage.setOrientationType(IStageDef::ORIENTATION_X);
    }
    else if (gladeWidget<Gtk::RadioButton>("orientationY")->get_active())
    {
        stage.setOrientationType(IStageDef::ORIENTATION_Y);
    }
    else if (gladeWidget<Gtk::RadioButton>("orientationZ")->get_active())
    {
        stage.setOrientationType(IStageDef::ORIENTATION_Z);
    }

    gladeWidget<Gtk::Widget>("aimedTrailsHBox")->set_sensitive(stage.getOrientationType() == IStageDef::ORIENTATION_AIMED);
    gladeWidget<Gtk::Widget>("aimedTimeHBox")->set_sensitive(stage.getOrientationType() == IStageDef::ORIENTATION_AIMED);

    stage.setInitialAngle(getSpinButtonValueAsFloat("initialAngleSpinner"));
}

void ParticleEditor::_onSizeControlsChanged()
{
    if (_callbacksDisabled || !_currentDef || !_selectedStageIter) return;

    IStageDef& stage = _currentDef->getStage(getSelectedStageIndex());

    stage.getSize().setFrom(getSpinButtonValueAsFloat("sizeFromSpinner"));
    stage.getSize().setTo(getSpinButtonValueAsFloat("sizeToSpinner"));

    stage.getSpeed().setFrom(getSpinButtonValueAsFloat("speedFromSpinner"));
    stage.getSpeed().setTo(getSpinButtonValueAsFloat("speedToSpinner"));

    stage.getRotationSpeed().setFrom(getSpinButtonValueAsFloat("rotationSpeedFromSpinner"));
    stage.getRotationSpeed().setTo(getSpinButtonValueAsFloat("rotationSpeedToSpinner"));

    stage.getAspect().setFrom(getSpinButtonValueAsFloat("aspectFromSpinner"));
    stage.getAspect().setTo(getSpinButtonValueAsFloat("aspectToSpinner"));

    stage.setGravity(getSpinButtonValueAsFloat("gravitySpinner"));
    stage.setWorldGravityFlag(gladeWidget<Gtk::CheckButton>("useWorldGravity")->get_active());

    stage.setBoundsExpansion(getSpinButtonValueAsFloat("boundsExpansionSpinner"));
}

void ParticleEditor::_onPathControlsChanged()
{
    if (_callbacksDisabled || !_currentDef || !_selectedStageIter) return;

    IStageDef& stage = _currentDef->getStage(getSelectedStageIndex());

    if (gladeWidget<Gtk::RadioButton>("pathStandard")->get_active())
    {
        stage.setCustomPathType(IStageDef::PATH_STANDARD);
    }
    else if (gladeWidget<Gtk::RadioButton>("pathFlies")->get_active())
    {
        stage.setCustomPathType(IStageDef::PATH_FLIES);

        stage.setCustomPathParm(0, getSpinButtonValueAsFloat("pathRadialSpeedSpinner"));
        stage.setCustomPathParm(1, getSpinButtonValueAsFloat("pathAxialSpeedSpinner"));
        stage.setCustomPathParm(2, getSpinButtonValueAsFloat("pathRadiusSpinner"));
    }
    else if (gladeWidget<Gtk::RadioButton>("pathHelix")->get_active())
    {
        stage.setCustomPathType(IStageDef::PATH_HELIX);

        stage.setCustomPathParm(0, getSpinButtonValueAsFloat("pathSizeXSpinner"));
        stage.setCustomPathParm(1, getSpinButtonValueAsFloat("pathSizeYSpinner"));
        stage.setCustomPathParm(2, getSpinButtonValueAsFloat("pathSizeZSpinner"));
        stage.setCustomPathParm(3, getSpinButtonValueAsFloat("pathRadialSpeedSpinner"));
        stage.setCustomPathParm(4, getSpinButtonValueAsFloat("pathAxialSpeedSpinner"));
    }

    updatePathWidgetSensitivity();
}

void ParticleEditor::_onDepthHackChanged()
{
    if (_callbacksDisabled || !_currentDef) return;

    _currentDef->setDepthHack(getSpinButtonValueAsFloat("depthHackSpinner"));
}

void ParticleEditor::updatePathWidgetSensitivity()
{
    IStageDef& stage = _currentDef->getStage(getSelectedStageIndex());

    // Sensitivity
    bool useAnySpinner = stage.getCustomPathType() != IStageDef::PATH_STANDARD;
    bool useFlies = stage.getCustomPathType() == IStageDef::PATH_FLIES;

    gladeWidget<Gtk::Widget>("pathRadialSpeedLabel")->set_sensitive(useAnySpinner);
    gladeWidget<Gtk::Widget>("pathAxialSpeedLabel")->set_sensitive(useAnySpinner);
    gladeWidget<Gtk::Widget>("pathRadialSpeedHBox")->set_sensitive(useAnySpinner);
    gladeWidget<Gtk::Widget>("pathAxialSpeedsHBox")->set_sensitive(useAnySpinner);

    gladeWidget<Gtk::Widget>("pathRadiusLabel")->set_sensitive(useAnySpinner && useFlies);
    gladeWidget<Gtk::Widget>("pathSphereRadiusHBox")->set_sensitive(useAnySpinner && useFlies);

    gladeWidget<Gtk::Widget>("pathSizeXLabel")->set_sensitive(useAnySpinner && !useFlies);
    gladeWidget<Gtk::Widget>("pathSizeYLabel")->set_sensitive(useAnySpinner && !useFlies);
    gladeWidget<Gtk::Widget>("pathSizeZLabel")->set_sensitive(useAnySpinner && !useFlies);
    gladeWidget<Gtk::Widget>("pathSizeXHBox")->set_sensitive(useAnySpinner && !useFlies);
    gladeWidget<Gtk::Widget>("pathSizeYHBox")->set_sensitive(useAnySpinner && !useFlies);
    gladeWidget<Gtk::Widget>("pathSizeZHBox")->set_sensitive(useAnySpinner && !useFlies);
}

bool ParticleEditor::_onSpinButtonKeyRelease(GdkEventKey*, MemberMethod func)
{
    // Call the pointer-to-member method
    (this->*func)();
    return false;
}

void ParticleEditor::connectSpinner(const std::string& name, MemberMethod func)
{
    // Connect the regular "value-changed" signal to the given method
    gladeWidget<Gtk::SpinButton>(name)->signal_value_changed().connect(
        sigc::mem_fun(*this, func));

    // Additionally, since the value-changed signal is only called after the user leaves
    // the entry field after typing in a value, we hook a special key-release event to catch inputs
    gladeWidget<Gtk::SpinButton>(name)->signal_key_release_event().connect(
        sigc::bind(sigc::mem_fun(*this, &ParticleEditor::_onSpinButtonKeyRelease), func), false);
}

float ParticleEditor::getSpinButtonValueAsFloat(const std::string& widgetName)
{
    Gtk::SpinButton* sb = gladeWidget<Gtk::SpinButton>(widgetName);
    return string::convert<float>(sb->get_text());
}

int ParticleEditor::getSpinButtonValueAsInt(const std::string& widgetName)
{
    Gtk::SpinButton* sb = gladeWidget<Gtk::SpinButton>(widgetName);
    return string::convert<int>(sb->get_text());
}

void ParticleEditor::activateEditPanels()
{
    gladeWidget<Gtk::Widget>("stageLabel")->set_sensitive(true);
    gladeWidget<Gtk::Widget>("settingsLabel")->set_sensitive(true);

    activateSettingsEditPanels();
}

void ParticleEditor::deactivateEditPanels()
{
    gladeWidget<Gtk::Widget>("stageLabel")->set_sensitive(false);
    gladeWidget<Gtk::Widget>("stageAlignment")->set_sensitive(false);

    deactivateSettingsEditPanels();
}

void ParticleEditor::activateSettingsEditPanels()
{
    gladeWidget<Gtk::Widget>("stageAlignment")->set_sensitive(true);
    gladeWidget<Gtk::Widget>("settingsNotebook")->set_sensitive(true);
}

void ParticleEditor::deactivateSettingsEditPanels()
{
    gladeWidget<Gtk::Widget>("settingsLabel")->set_sensitive(false);
    gladeWidget<Gtk::Widget>("settingsNotebook")->set_sensitive(false);
}

std::size_t ParticleEditor::getSelectedStageIndex()
{
    // Get the selection and store it
    Gtk::TreeModel::iterator iter = _stageSelection->get_selected();

    int value = (*iter)[STAGE_COLS().index];

    if (value < 0)
    {
        throw std::logic_error("Invalid stage index stored in model.");
    }

    return value;
}

void ParticleEditor::selectStage(std::size_t index)
{
    gtkutil::TreeModel::findAndSelectInteger(
        gladeWidget<Gtk::TreeView>("stageView"), static_cast<int>(index), STAGE_COLS().index);
}

void ParticleEditor::setSaveButtonsSensitivity(bool sensitive)
{
    gladeWidget<Gtk::Widget>("saveParticleButton")->set_sensitive(sensitive);
    gladeWidget<Gtk::Widget>("cloneParticleButton")->set_sensitive(sensitive);
}

namespace
{
    std::string particleNameFromIter(const Gtk::TreeModel::iterator& i)
    {
        if (!i)
        {
            return "";
        }
        else
        {
            return (*i)[DEF_COLS().name];
        }
    }
}

void ParticleEditor::_onDefSelChanged()
{
    // Get the selection and store it
    Gtk::TreeModel::iterator iter = _defSelection->get_selected();

    if (!promptUserToSaveChanges())
    {
        // Revert the selection (re-enter this function) and cancel the operation
        _defSelection->select(_selectedDefIter);
        return;
    }

    if (_selectedDefIter && iter && _selectedDefIter == iter)
    {
        return; // nothing to do so far
    }

    // Selected particle changed, free the existing edit particle
    releaseEditParticle();

    // Store new selection
    _selectedDefIter = iter;

    if (_selectedDefIter)
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
        _preview->setParticle("");
        _stageList->clear();
        deactivateEditPanels();
        setSaveButtonsSensitivity(false);
    }
}

void ParticleEditor::_onStageSelChanged()
{
    // Get the selection and store it
    Gtk::TreeModel::iterator iter = _stageSelection->get_selected();

    if (_selectedStageIter && iter && _selectedStageIter == iter)
    {
        return; // nothing to do so far
    }

    _selectedStageIter = iter;

    bool isStageSelected = false;

    if (_selectedStageIter)
    {
        activateSettingsEditPanels();

        // Activate delete, move and toggle buttons
        isStageSelected = true;

        std::size_t index = (*_selectedStageIter)[STAGE_COLS().index];

        gladeWidget<Gtk::Button>("moveStageUpButton")->set_sensitive(index > 0);
        gladeWidget<Gtk::Button>("moveStageDownButton")->set_sensitive(index < _currentDef->getNumStages() - 1);
    }
    else
    {
        // No valid selection
        deactivateSettingsEditPanels();

        // Deactivate delete, move and toggle buttons
        isStageSelected = false;

        gladeWidget<Gtk::Button>("moveStageUpButton")->set_sensitive(false);
        gladeWidget<Gtk::Button>("moveStageDownButton")->set_sensitive(false);
    }

    gladeWidget<Gtk::Button>("removeStageButton")->set_sensitive(isStageSelected);
    gladeWidget<Gtk::Button>("toggleStageButton")->set_sensitive(isStageSelected);
    gladeWidget<Gtk::Button>("duplicateStageButton")->set_sensitive(isStageSelected);

    // Reload the current stage data
    updateWidgetsFromStage();
}

void ParticleEditor::_onAddStage()
{
    if (!_currentDef) return;

    // Add a new stage at the end of the list
    std::size_t newStage = _currentDef->addParticleStage();

    reloadStageList();

    selectStage(newStage);
}

void ParticleEditor::_onRemoveStage()
{
    if (!_currentDef || !_selectedStageIter) return;

    _currentDef->removeParticleStage(getSelectedStageIndex());

    reloadStageList();
}

void ParticleEditor::_onToggleStage()
{
    if (!_currentDef || !_selectedStageIter) return;

    std::size_t index = getSelectedStageIndex();

    IStageDef& stage = _currentDef->getStage(index);

    stage.setVisible(!stage.isVisible());

    reloadStageList();
    selectStage(index);
}

void ParticleEditor::_onMoveUpStage()
{
    if (!_currentDef) return;

    std::size_t selIndex = getSelectedStageIndex();
    assert(selIndex > 0);

    _currentDef->swapParticleStages(selIndex, selIndex - 1);

    reloadStageList();
    selectStage(selIndex - 1);
}

void ParticleEditor::_onMoveDownStage()
{
    if (!_currentDef) return;

    std::size_t selIndex = getSelectedStageIndex();
    assert(_currentDef->getNumStages() > 0 && selIndex < _currentDef->getNumStages() - 1);

    _currentDef->swapParticleStages(selIndex, selIndex + 1);

    reloadStageList();
    selectStage(selIndex + 1);
}

void ParticleEditor::_onDuplicateStage()
{
    if (!_currentDef) return;

    std::size_t srcStageIndex = getSelectedStageIndex();
    std::size_t newStageIndex = _currentDef->addParticleStage();

    IStageDef& srcStage = _currentDef->getStage(srcStageIndex);
    IStageDef& newStage = _currentDef->getStage(newStageIndex);

    newStage.copyFrom(srcStage);

    reloadStageList();

    selectStage(newStageIndex);
}

void ParticleEditor::updateWidgetsFromParticle()
{
    if (!_currentDef)
    {
        gladeWidget<Gtk::Label>("outFileLabel")->set_markup("");
        return;
    }

    // Load stages
    reloadStageList();

    _callbacksDisabled = true;

    // Update depth hack
    gladeWidget<Gtk::SpinButton>("depthHackSpinner")->get_adjustment()->set_value(_currentDef->getDepthHack());

    _callbacksDisabled = false;

    // Load stage data into controls
    updateWidgetsFromStage();

    // Update outfile label
    boost::filesystem::path outFile = GlobalGameManager().getModPath();
    outFile /= PARTICLES_DIR;
    outFile /= _currentDef->getFilename();
    gladeWidget<Gtk::Label>("outFileLabel")->set_markup(
        (boost::format(_("Note: changes will be written to the file <i>%s</i>")) % outFile.string()).str());
}

void ParticleEditor::reloadStageList()
{
    if (!_currentDef) return;

    // Load stages
    _stageList->clear();

    for (std::size_t i = 0; i < _currentDef->getNumStages(); ++i)
    {
        const IStageDef& stage = _currentDef->getStage(i);

        Gtk::TreeModel::iterator iter = _stageList->append();

        (*iter)[STAGE_COLS().name] = (boost::format("Stage %d") % static_cast<int>(i)).str();
        (*iter)[STAGE_COLS().index] = static_cast<int>(i);
        (*iter)[STAGE_COLS().visible] = true;
        (*iter)[STAGE_COLS().colour] = stage.isVisible() ? "#000000" : "#707070";
    }

    // Select the first stage if possible
    gtkutil::TreeModel::findAndSelectInteger(
        gladeWidget<Gtk::TreeView>("stageView"), 0, STAGE_COLS().index
    );
}

void ParticleEditor::updateWidgetsFromStage()
{
    if (!_currentDef || !_selectedStageIter) return;

    _callbacksDisabled = true;

    const IStageDef& stage = _currentDef->getStage(getSelectedStageIndex());

    gladeWidget<Gtk::Entry>("shaderEntry")->set_text(stage.getMaterialName());

    const Vector4& colour = stage.getColour();
    gladeWidget<Gtk::Entry>("colourEntry")->set_text(
        (boost::format("%.2f %.2f %.2f %.2f") % colour.x() % colour.y() % colour.z() % colour.w()).str()
    );

    gladeWidget<Gtk::CheckButton>("useEntityColour")->set_active(stage.getUseEntityColour());

    const Vector4& fadeColour = stage.getFadeColour();
    gladeWidget<Gtk::Entry>("fadeColourEntry")->set_text(
        (boost::format("%.2f %.2f %.2f %.2f") % fadeColour.x() % fadeColour.y() % fadeColour.z() % fadeColour.w()).str()
    );

    gladeWidget<Gtk::SpinButton>("fadeInFractionSpinner")->get_adjustment()->set_value(stage.getFadeInFraction());
    gladeWidget<Gtk::SpinButton>("fadeOutFractionSpinner")->get_adjustment()->set_value(stage.getFadeOutFraction());
    gladeWidget<Gtk::SpinButton>("fadeIndexFractionSpinner")->get_adjustment()->set_value(stage.getFadeIndexFraction());
    gladeWidget<Gtk::SpinButton>("animFramesSpinner")->get_adjustment()->set_value(stage.getAnimationFrames());
    gladeWidget<Gtk::SpinButton>("animRateSpinner")->get_adjustment()->set_value(stage.getAnimationRate());

    gladeWidget<Gtk::SpinButton>("countSpinner")->get_adjustment()->set_value(stage.getCount());
    gladeWidget<Gtk::SpinButton>("timeSpinner")->get_adjustment()->set_value(stage.getDuration());
    gladeWidget<Gtk::SpinButton>("bunchingSpinner")->get_adjustment()->set_value(stage.getBunching());
    gladeWidget<Gtk::SpinButton>("cyclesSpinner")->get_adjustment()->set_value(stage.getCycles());
    gladeWidget<Gtk::SpinButton>("timeOffsetSpinner")->get_adjustment()->set_value(stage.getTimeOffset());
    gladeWidget<Gtk::SpinButton>("deadTimeSpinner")->get_adjustment()->set_value(stage.getDeadTime());

    // DISTRIBUTION

    bool useRingSize = false;

    switch (stage.getDistributionType())
    {
    case IStageDef::DISTRIBUTION_RECT:
        gladeWidget<Gtk::RadioButton>("distRectangle")->set_active(true);
        break;
    case IStageDef::DISTRIBUTION_CYLINDER:
        gladeWidget<Gtk::RadioButton>("distCylinder")->set_active(true);
        useRingSize = true;
        break;
    case IStageDef::DISTRIBUTION_SPHERE:
        gladeWidget<Gtk::RadioButton>("distSphere")->set_active(true);
        useRingSize = true;
        break;
    };

    gladeWidget<Gtk::Widget>("distSizeRingHBox")->set_sensitive(useRingSize);
    gladeWidget<Gtk::Widget>("distSizeRingLabel")->set_sensitive(useRingSize);

    gladeWidget<Gtk::SpinButton>("distSizeXSpinner")->get_adjustment()->set_value(stage.getDistributionParm(0));
    gladeWidget<Gtk::SpinButton>("distSizeYSpinner")->get_adjustment()->set_value(stage.getDistributionParm(1));
    gladeWidget<Gtk::SpinButton>("distSizeZSpinner")->get_adjustment()->set_value(stage.getDistributionParm(2));
    gladeWidget<Gtk::SpinButton>("distSizeRingSpinner")->get_adjustment()->set_value(stage.getDistributionParm(3));
    gladeWidget<Gtk::Entry>("distOffsetEntry")->set_text(
        string::to_string(stage.getOffset())
    );
    gladeWidget<Gtk::CheckButton>("distRandom")->set_active(stage.getRandomDistribution());

    // DIRECTION / ORIENTATION

    switch (stage.getDirectionType())
    {
    case IStageDef::DIRECTION_CONE:
        gladeWidget<Gtk::RadioButton>("directionCone")->set_active(true);
        gladeWidget<Gtk::SpinButton>("coneAngleSpinner")->get_adjustment()->set_value(stage.getDirectionParm(0));
        break;
    case IStageDef::DIRECTION_OUTWARD:
        gladeWidget<Gtk::RadioButton>("directionOutward")->set_active(true);
        gladeWidget<Gtk::SpinButton>("upwardBiasSpinner")->get_adjustment()->set_value(stage.getDirectionParm(0));
        break;
    };

    gladeWidget<Gtk::Widget>("coneAngleHBox")->set_sensitive(stage.getDirectionType() == IStageDef::DIRECTION_CONE);
    gladeWidget<Gtk::Widget>("upwardBiasHBox")->set_sensitive(stage.getDirectionType() == IStageDef::DIRECTION_OUTWARD);

    // Orientation Type
    switch (stage.getOrientationType())
    {
    case IStageDef::ORIENTATION_VIEW:
        gladeWidget<Gtk::RadioButton>("orientationView")->set_active(true);
        break;
    case IStageDef::ORIENTATION_AIMED:
        gladeWidget<Gtk::RadioButton>("orientationAimed")->set_active(true);
        gladeWidget<Gtk::SpinButton>("aimedTrailsSpinner")->get_adjustment()->set_value(stage.getOrientationParm(0));
        gladeWidget<Gtk::SpinButton>("aimedTimeSpinner")->get_adjustment()->set_value(stage.getOrientationParm(1));
        break;
    case IStageDef::ORIENTATION_X:
        gladeWidget<Gtk::RadioButton>("orientationX")->set_active(true);

        break;
    case IStageDef::ORIENTATION_Y:
        gladeWidget<Gtk::RadioButton>("orientationY")->set_active(true);
        break;
    case IStageDef::ORIENTATION_Z:
        gladeWidget<Gtk::RadioButton>("orientationZ")->set_active(true);
        break;
    };

    gladeWidget<Gtk::Widget>("aimedTrailsHBox")->set_sensitive(stage.getOrientationType() == IStageDef::ORIENTATION_AIMED);
    gladeWidget<Gtk::Widget>("aimedTimeHBox")->set_sensitive(stage.getOrientationType() == IStageDef::ORIENTATION_AIMED);

    gladeWidget<Gtk::SpinButton>("initialAngleSpinner")->get_adjustment()->set_value(stage.getInitialAngle());

    // SIZE / SPEED / ASPECT

    gladeWidget<Gtk::SpinButton>("sizeFromSpinner")->get_adjustment()->set_value(stage.getSize().getFrom());
    gladeWidget<Gtk::SpinButton>("sizeToSpinner")->get_adjustment()->set_value(stage.getSize().getTo());

    gladeWidget<Gtk::SpinButton>("speedFromSpinner")->get_adjustment()->set_value(stage.getSpeed().getFrom());
    gladeWidget<Gtk::SpinButton>("speedToSpinner")->get_adjustment()->set_value(stage.getSpeed().getTo());

    gladeWidget<Gtk::SpinButton>("rotationSpeedFromSpinner")->get_adjustment()->set_value(stage.getRotationSpeed().getFrom());
    gladeWidget<Gtk::SpinButton>("rotationSpeedToSpinner")->get_adjustment()->set_value(stage.getRotationSpeed().getTo());

    gladeWidget<Gtk::SpinButton>("aspectFromSpinner")->get_adjustment()->set_value(stage.getAspect().getFrom());
    gladeWidget<Gtk::SpinButton>("aspectToSpinner")->get_adjustment()->set_value(stage.getAspect().getTo());

    gladeWidget<Gtk::SpinButton>("gravitySpinner")->get_adjustment()->set_value(stage.getGravity());

    gladeWidget<Gtk::CheckButton>("useWorldGravity")->set_active(stage.getWorldGravityFlag());

    gladeWidget<Gtk::SpinButton>("boundsExpansionSpinner")->get_adjustment()->set_value(stage.getBoundsExpansion());

    // PATH

    switch (stage.getCustomPathType())
    {
    case IStageDef::PATH_STANDARD:
        gladeWidget<Gtk::RadioButton>("pathStandard")->set_active(true);
        break;
    case IStageDef::PATH_FLIES:
        gladeWidget<Gtk::RadioButton>("pathFlies")->set_active(true);

        gladeWidget<Gtk::SpinButton>("pathRadialSpeedSpinner")->get_adjustment()->set_value(stage.getCustomPathParm(0));
        gladeWidget<Gtk::SpinButton>("pathAxialSpeedSpinner")->get_adjustment()->set_value(stage.getCustomPathParm(1));
        gladeWidget<Gtk::SpinButton>("pathRadiusSpinner")->get_adjustment()->set_value(stage.getCustomPathParm(2));
        break;
    case IStageDef::PATH_HELIX:
        gladeWidget<Gtk::RadioButton>("pathHelix")->set_active(true);

        gladeWidget<Gtk::SpinButton>("pathRadialSpeedSpinner")->get_adjustment()->set_value(stage.getCustomPathParm(3));
        gladeWidget<Gtk::SpinButton>("pathAxialSpeedSpinner")->get_adjustment()->set_value(stage.getCustomPathParm(4));
        gladeWidget<Gtk::SpinButton>("pathSizeXSpinner")->get_adjustment()->set_value(stage.getCustomPathParm(0));
        gladeWidget<Gtk::SpinButton>("pathSizeYSpinner")->get_adjustment()->set_value(stage.getCustomPathParm(1));
        gladeWidget<Gtk::SpinButton>("pathSizeZSpinner")->get_adjustment()->set_value(stage.getCustomPathParm(2));
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
    Gtk::TreeModel::iterator iter = _defSelection->get_selected();
    if (!iter) return;

    // Get the def for the selected particle system if it exists
    std::string selectedName = particleNameFromIter(iter);
    IParticleDefPtr def = GlobalParticlesManager().getDefByName(selectedName);
    if (!def)
    {
        _preview->setParticle("");
        return;
    }

    // Generate a temporary name for this particle, and instantiate a copy
    std::string temporaryParticleName = selectedName + EDIT_SUFFIX;

    _currentDef = ParticlesManager::Instance().findOrInsertParticleDef(temporaryParticleName);
    _currentDef->setFilename(def->getFilename());

    _currentDef->copyFrom(*def);

    // Point the preview to this temporary particle def
    _preview->setParticle(_currentDef->getName());
}

void ParticleEditor::releaseEditParticle()
{
    if (_currentDef && boost::algorithm::ends_with(_currentDef->getName(), EDIT_SUFFIX))
    {
        ParticlesManager::Instance().removeParticleDef(_currentDef->getName());
    }

    _currentDef.reset();
}

bool ParticleEditor::particleHasUnsavedChanges()
{
    if (_selectedDefIter && _currentDef)
    {
        // Particle selection changed, check if we have any unsaved changes
        std::string origName = particleNameFromIter(_selectedDefIter);

        IParticleDefPtr origDef = GlobalParticlesManager().getDefByName(origName);

        if (!origDef || *_currentDef != *origDef)
        {
            return true;
        }
    }
    return false;
}

IDialog::Result ParticleEditor::askForSave()
{
    // Get the original particle name
    std::string origName = particleNameFromIter(_selectedDefIter);

    // Does not make sense to save a null particle
    g_assert(!origName.empty());

    // The particle we're editing has been changed from the saved one
    gtkutil::MessageBox box(_("Save Changes"),
        (boost::format(_("Do you want to save the changes\nyou made to the particle %s?")) % origName).str(),
        IDialog::MESSAGE_SAVECONFIRMATION, GlobalMainFrame().getTopLevelWindow());

    return box.run();
}

bool ParticleEditor::saveCurrentParticle()
{
    // Get the original particle name
    std::string origName = particleNameFromIter(_selectedDefIter);

    IParticleDefPtr origDef = GlobalParticlesManager().getDefByName(origName);

    // This should really succeed, we can't have non-existing particles selected
    // in the treeview
    assert(origDef);

    // Write the changes from the working copy into the actual instance
    origDef->copyFrom(*_currentDef);

    // Write changes to disk, and return the result
    try
    {
        ParticlesManager::Instance().saveParticleDef(origDef->getName());
        return true;
    }
    catch (std::runtime_error& err)
    {
        std::string errMsg = (boost::format(_("Error saving particle definition:\n%s")) % err.what()).str();

        rError() << errMsg << std::endl;

        gtkutil::MessageBox::ShowError(errMsg, getRefPtr());

        return false;
    }
}

void ParticleEditor::_preHide()
{
    // Tell the position tracker to save the information
    _windowPosition.saveToPath(RKEY_WINDOW_STATE);

    // Free the edit particle before hiding this dialog
    releaseEditParticle();
}

void ParticleEditor::_preShow()
{
    // Restore the position
    _windowPosition.applyPosition();
}

void ParticleEditor::_postShow()
{
    // Initialise the GL widget after the widgets have been shown
    _preview->initialisePreview();

    // Call the base class, will enter main loop
    BlockingTransientWindow::_postShow();
}

void ParticleEditor::_onClose()
{
    if (!promptUserToSaveChanges(false))    return; // action not allowed or cancelled

    // Close the window
    destroy();
}

bool ParticleEditor::defSelectionHasChanged()
{
    // Check if the selection has changed
    Gtk::TreeModel::iterator iter = _defSelection->get_selected();

    bool changed;
    if (!_selectedDefIter)
    {
        changed = static_cast<bool>(iter);
    }
    else if (!iter) // _selectedDefIter is valid
    {
        changed = true;
    }
    else // both iter and _selectedDefIter are valid
    {
        changed = (_selectedDefIter != iter);
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

void ParticleEditor::_onNewParticle()
{
    // Check for unsaved changes, don't require a selection change
    if (!promptUserToSaveChanges(false)) return; // action not allowed or cancelled

    createAndSelectNewParticle();
}

ParticleDefPtr ParticleEditor::createAndSelectNewParticle()
{
    std::string particleName = queryNewParticleName();

    if (particleName.empty())
    {
        return ParticleDefPtr(); // no valid name, abort
    }

    std::string destFile = queryParticleFile();

    if (destFile.empty())
    {
        return ParticleDefPtr(); // no valid destination file
    }

    // Good filename, good destination file, we're set to go
    ParticleDefPtr particle = ParticlesManager::Instance().findOrInsertParticleDef(particleName);

    particle->setFilename(destFile);

    // Re-load the particles list
    populateParticleDefList();

    // Highlight our new particle
    selectParticleDef(particle->getName());

    return particle;
}

std::string ParticleEditor::queryParticleFile()
{
    // Get the filename we should save this particle into
    gtkutil::FileChooser chooser(getRefPtr(), _("Select .prt file"), false, false, "particle", ".prt");

    boost::filesystem::path modParticlesPath = GlobalGameManager().getModPath();
    modParticlesPath /= "particles";

    if (!boost::filesystem::exists(modParticlesPath))
    {
        rMessage() << "Ensuring mod particles path: " << modParticlesPath << std::endl;
        boost::filesystem::create_directories(modParticlesPath);
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
                particleName = gtkutil::Dialog::TextEntryDialog(
                    _("Enter Name"),
                    _("Enter Particle Name"),
                    "",
                    GlobalMainFrame().getTopLevelWindow()
                );
            }
            catch (gtkutil::EntryAbortedException&)
            {
                break;
            }
        }

        if (particleName.empty())
        {
            // Wrong name, let the user try again
            gtkutil::MessageBox::ShowError(_("Cannot create particle with an empty name."), GlobalMainFrame().getTopLevelWindow());
            continue;
        }

        // Check if this particle already exists
        IParticleDefPtr existing = GlobalParticlesManager().getDefByName(particleName);

        if (existing == NULL)
        {
            // Success, return that name
            return particleName;
        }
        else
        {
            // Wrong name, let the user try again
            gtkutil::MessageBox::ShowError(_("This name is already in use."), GlobalMainFrame().getTopLevelWindow());
            continue;
        }
    }

    return ""; // no successful entry
}

void ParticleEditor::cloneCurrentParticle()
{
    util::ScopedBoolLock lock(_saveInProgress);

    // Get the original particle name
    std::string origName = particleNameFromIter(_selectedDefIter);

    if (origName.empty())
    {
        return;
    }

    // Look up the original particle def
    IParticleDefPtr original = GlobalParticlesManager().getDefByName(origName);

    // Create a new particle
    ParticleDefPtr newParticle = createAndSelectNewParticle();

    if (!newParticle)
    {
        return;
    }

    // Copy stuff from original particle
    newParticle->copyFrom(*original);

    // Clear selection and re-select the particle to set up the working copy
    _defSelection->unselect_all();
    selectParticleDef(newParticle->getName());

    // Save the new particle declaration to the file immediately
    saveCurrentParticle();

    // Reload controls
    updateWidgetsFromParticle();
}

void ParticleEditor::displayDialog(const cmd::ArgumentList& args)
{
    ParticleEditor editor;
    editor.show();
}

}
