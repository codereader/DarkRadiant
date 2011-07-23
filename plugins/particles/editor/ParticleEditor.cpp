#include "ParticleEditor.h"

#include "i18n.h"
#include "imainframe.h"
#include "iuimanager.h"
#include "iparticles.h"

#include "gtkutil/MultiMonitor.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/dialog/MessageBox.h"

#include <gtkmm/button.h>
#include <gtkmm/paned.h>
#include <gtkmm/spinbutton.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/treeview.h>
#include <gtkmm/stock.h>

#include "ParticleDefPopulator.h"
#include "../ParticlesManager.h"

#include <boost/algorithm/string/predicate.hpp>

namespace ui
{

// CONSTANTS
namespace
{
	const char* const DIALOG_TITLE = N_("Particle Editor");

	const std::string RKEY_ROOT = "user/ui/particleEditor/";
	const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";

	const std::string EDIT_SUFFIX = "___editor";
}

ParticleEditor::ParticleEditor() :
	gtkutil::BlockingTransientWindow(DIALOG_TITLE, GlobalMainFrame().getTopLevelWindow()),
	gtkutil::GladeWidgetHolder(GlobalUIManager().getGtkBuilderFromFile("ParticleEditor.glade")),
	_defList(Gtk::ListStore::create(_defColumns)),
	_stageList(Gtk::ListStore::create(_stageColumns)),
	_preview(GlobalUIManager().createParticlePreview()),
	_callbacksDisabled(false)
{
	// Window properties
    set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
    set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
    
    // Add vbox to dialog
    add(*getGladeWidget<Gtk::Widget>("mainVbox"));
    g_assert(get_child() != NULL);

	// Wire up the close button
	getGladeWidget<Gtk::Button>("closeButton")->signal_clicked().connect(
        sigc::mem_fun(*this, &ParticleEditor::_onClose)
    );

	// Set the default size of the window
	const Glib::RefPtr<Gtk::Window>& mainWindow = GlobalMainFrame().getTopLevelWindow();

	Gdk::Rectangle rect = gtkutil::MultiMonitor::getMonitorForWindow(mainWindow);
	int height = static_cast<int>(rect.get_height() * 0.6f);

	set_default_size(static_cast<int>(rect.get_width() * 0.6f), height);

	// Setup and pack the preview
	_preview->setSize(height);
	getGladeWidget<Gtk::HPaned>("mainPane")->add2(*_preview->getWidget());

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
	if (particleHasUnsavedChanges() && askForSave() && !saveCurrentParticle())
	{
		// Particle has unsaved changes, user wants to save the particle, 
		// but the save attempt failed, inhibit window destruction
		return;
	}

	// Window destruction allowed, pass to base class => triggers destroy
	BlockingTransientWindow::_onDeleteEvent();
}

void ParticleEditor::setupParticleDefList()
{
	Gtk::TreeView* view = getGladeWidget<Gtk::TreeView>("definitionView");

	view->set_model(_defList);
	view->set_headers_visible(false);

	// Single text column
	view->append_column(*Gtk::manage(new gtkutil::TextColumn(_("Particle"), _defColumns.name, false)));

	// Apply full-text search to the column
	view->set_search_equal_func(sigc::ptr_fun(gtkutil::TreeModel::equalFuncStringContains));

	populateParticleDefList();

	// Connect up the selection changed callback
	_defSelection = view->get_selection();
	_defSelection->signal_changed().connect(sigc::mem_fun(*this, &ParticleEditor::_onDefSelChanged));
}

void ParticleEditor::populateParticleDefList()
{
	_defList->clear();

	// Create and use a ParticlesVisitor to populate the list
	ParticlesVisitor visitor(_defList, _defColumns);
	GlobalParticlesManager().forEachParticleDef(visitor);
}

void ParticleEditor::setupParticleStageList()
{
	Gtk::TreeView* view = getGladeWidget<Gtk::TreeView>("stageView");

	view->set_model(_stageList);
	view->set_headers_visible(false);

	// Single text column
	view->append_column(*Gtk::manage(new gtkutil::ColouredTextColumn(_("Stage"), _stageColumns.name, _stageColumns.colour, false)));

	// Connect up the selection changed callback
	_stageSelection = view->get_selection();
	_stageSelection->signal_changed().connect(sigc::mem_fun(*this, &ParticleEditor::_onStageSelChanged));

	// Connect the stage control buttons
	getGladeWidget<Gtk::Button>("addStageButton")->signal_clicked().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onAddStage));
	getGladeWidget<Gtk::Button>("removeStageButton")->signal_clicked().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onRemoveStage));
	getGladeWidget<Gtk::Button>("toggleStageButton")->signal_clicked().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onToggleStage));
	getGladeWidget<Gtk::Button>("moveStageUpButton")->signal_clicked().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onMoveUpStage));
	getGladeWidget<Gtk::Button>("moveStageDownButton")->signal_clicked().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onMoveDownStage));
	getGladeWidget<Gtk::Button>("duplicateStageButton")->signal_clicked().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onDuplicateStage));

	getGladeWidget<Gtk::Button>("duplicateStageButton")->set_image(*Gtk::manage(new Gtk::Image(Gtk::Stock::COPY, Gtk::ICON_SIZE_BUTTON)));
}

void ParticleEditor::setupSettingsPages()
{
	getGladeWidget<Gtk::Entry>("shaderEntry")->signal_changed().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onShaderControlsChanged));
	getGladeWidget<Gtk::Entry>("colourEntry")->signal_changed().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onShaderControlsChanged));
	getGladeWidget<Gtk::Entry>("fadeColourEntry")->signal_changed().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onShaderControlsChanged));

	getGladeWidget<Gtk::SpinButton>("fadeInFractionSpinner")->signal_changed().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onShaderControlsChanged));
	getGladeWidget<Gtk::SpinButton>("fadeOutFractionSpinner")->signal_changed().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onShaderControlsChanged));
	getGladeWidget<Gtk::SpinButton>("fadeIndexFractionSpinner")->signal_changed().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onShaderControlsChanged));
	getGladeWidget<Gtk::SpinButton>("animFramesSpinner")->signal_changed().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onShaderControlsChanged));
	getGladeWidget<Gtk::SpinButton>("animRateSpinner")->signal_changed().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onShaderControlsChanged));

	// COUNT

	getGladeWidget<Gtk::SpinButton>("countSpinner")->signal_changed().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onCountTimeControlsChanged));
	getGladeWidget<Gtk::SpinButton>("timeSpinner")->signal_changed().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onCountTimeControlsChanged));
	getGladeWidget<Gtk::SpinButton>("bunchingSpinner")->signal_changed().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onCountTimeControlsChanged));
	getGladeWidget<Gtk::SpinButton>("cyclesSpinner")->signal_changed().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onCountTimeControlsChanged));
	getGladeWidget<Gtk::SpinButton>("timeOffsetSpinner")->signal_changed().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onCountTimeControlsChanged));
	getGladeWidget<Gtk::SpinButton>("deadTimeSpinner")->signal_changed().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onCountTimeControlsChanged));

	// DISTRIBUTION

	getGladeWidget<Gtk::RadioButton>("distRectangle")->signal_toggled().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onDistributionControlsChanged));
	getGladeWidget<Gtk::RadioButton>("distCylinder")->signal_toggled().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onDistributionControlsChanged));
	getGladeWidget<Gtk::RadioButton>("distSphere")->signal_toggled().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onDistributionControlsChanged));

	getGladeWidget<Gtk::SpinButton>("distSizeXSpinner")->signal_changed().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onDistributionControlsChanged));
	getGladeWidget<Gtk::SpinButton>("distSizeYSpinner")->signal_changed().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onDistributionControlsChanged));
	getGladeWidget<Gtk::SpinButton>("distSizeZSpinner")->signal_changed().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onDistributionControlsChanged));
	getGladeWidget<Gtk::SpinButton>("distSizeRingSpinner")->signal_changed().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onDistributionControlsChanged));

	getGladeWidget<Gtk::Entry>("distOffsetEntry")->signal_changed().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onDistributionControlsChanged));

	getGladeWidget<Gtk::CheckButton>("distRandom")->signal_toggled().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onDistributionControlsChanged));

	// DIRECTION / ORIENTATION

	getGladeWidget<Gtk::RadioButton>("directionCone")->signal_toggled().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onDirectionControlsChanged));
	getGladeWidget<Gtk::RadioButton>("directionOutward")->signal_toggled().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onDirectionControlsChanged));

	getGladeWidget<Gtk::SpinButton>("coneAngleSpinner")->signal_changed().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onDistributionControlsChanged));
	getGladeWidget<Gtk::SpinButton>("upwardBiasSpinner")->signal_changed().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onDistributionControlsChanged));

	getGladeWidget<Gtk::RadioButton>("orientationView")->signal_toggled().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onDirectionControlsChanged));
	getGladeWidget<Gtk::RadioButton>("orientationAimed")->signal_toggled().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onDirectionControlsChanged));
	getGladeWidget<Gtk::RadioButton>("orientationX")->signal_toggled().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onDirectionControlsChanged));
	getGladeWidget<Gtk::RadioButton>("orientationY")->signal_toggled().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onDirectionControlsChanged));
	getGladeWidget<Gtk::RadioButton>("orientationZ")->signal_toggled().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onDirectionControlsChanged));

	getGladeWidget<Gtk::SpinButton>("aimedTrailsSpinner")->signal_changed().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onDistributionControlsChanged));
	getGladeWidget<Gtk::SpinButton>("aimedTimeSpinner")->signal_changed().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onDistributionControlsChanged));
	getGladeWidget<Gtk::SpinButton>("initialAngleSpinner")->signal_changed().connect(
		sigc::mem_fun(*this, &ParticleEditor::_onDistributionControlsChanged));
}

void ParticleEditor::_onShaderControlsChanged()
{
	if (_callbacksDisabled || !_particle || !_selectedStageIter) return;

	particles::IParticleStage& stage = _particle->getParticleStage(getSelectedStageIndex());

	std::string material = getGladeWidget<Gtk::Entry>("shaderEntry")->get_text();
	
	// Only assign a new material if it has actually changed, otherwise the whole particle gets re-shuffled
	if (material != stage.getMaterialName())
	{
		stage.setMaterialName(getGladeWidget<Gtk::Entry>("shaderEntry")->get_text());
	}

	stage.setColour(Vector4(getGladeWidget<Gtk::Entry>("colourEntry")->get_text()));
	stage.setFadeColour(Vector4(getGladeWidget<Gtk::Entry>("fadeColourEntry")->get_text()));
	
	stage.setFadeInFraction(getSpinButtonValueAsFloat("fadeInFractionSpinner"));
	stage.setFadeOutFraction(getSpinButtonValueAsFloat("fadeOutFractionSpinner"));
	stage.setFadeIndexFraction(getSpinButtonValueAsFloat("fadeIndexFractionSpinner"));
	stage.setAnimationFrames(getSpinButtonValueAsFloat("animFramesSpinner"));
	stage.setAnimationRate(getSpinButtonValueAsFloat("animRateSpinner"));
}

void ParticleEditor::_onCountTimeControlsChanged()
{
	if (_callbacksDisabled || !_particle || !_selectedStageIter) return;

	particles::IParticleStage& stage = _particle->getParticleStage(getSelectedStageIndex());

	stage.setCount(getSpinButtonValueAsInt("countSpinner"));
	stage.setDuration(getSpinButtonValueAsFloat("timeSpinner"));
	stage.setBunching(getSpinButtonValueAsFloat("bunchingSpinner"));
	stage.setCycles(getSpinButtonValueAsInt("cyclesSpinner"));
	stage.setTimeOffset(getSpinButtonValueAsFloat("timeOffsetSpinner"));
	stage.setDeadTime(getSpinButtonValueAsFloat("deadTimeSpinner"));
}

void ParticleEditor::_onDistributionControlsChanged()
{
	if (_callbacksDisabled || !_particle || !_selectedStageIter) return;

	particles::IParticleStage& stage = _particle->getParticleStage(getSelectedStageIndex());

	if (getGladeWidget<Gtk::RadioButton>("distRectangle")->get_active())
	{
		stage.setDistributionType(particles::IParticleStage::DISTRIBUTION_RECT);
	}
	else if (getGladeWidget<Gtk::RadioButton>("distCylinder")->get_active())
	{
		stage.setDistributionType(particles::IParticleStage::DISTRIBUTION_CYLINDER);
	}
	else if (getGladeWidget<Gtk::RadioButton>("distSphere")->get_active())
	{
		stage.setDistributionType(particles::IParticleStage::DISTRIBUTION_SPHERE);
	}

	bool useRingSize = stage.getDistributionType() != particles::IParticleStage::DISTRIBUTION_RECT;

	getGladeWidget<Gtk::Widget>("distSizeRingHBox")->set_sensitive(useRingSize);
	getGladeWidget<Gtk::Widget>("distSizeRingLabel")->set_sensitive(useRingSize);

	stage.setDistributionParm(0, getSpinButtonValueAsFloat("distSizeXSpinner"));
	stage.setDistributionParm(1, getSpinButtonValueAsFloat("distSizeYSpinner"));
	stage.setDistributionParm(2, getSpinButtonValueAsFloat("distSizeZSpinner"));
	stage.setDistributionParm(3, getSpinButtonValueAsFloat("distSizeRingSpinner"));

	stage.setOffset(Vector3(getGladeWidget<Gtk::Entry>("distOffsetEntry")->get_text()));

	stage.setRandomDistribution(getGladeWidget<Gtk::CheckButton>("distRandom")->get_active());
}

void ParticleEditor::_onDirectionControlsChanged()
{
	if (_callbacksDisabled || !_particle || !_selectedStageIter) return;

	particles::IParticleStage& stage = _particle->getParticleStage(getSelectedStageIndex());

	if (getGladeWidget<Gtk::RadioButton>("directionCone")->get_active())
	{
		stage.setDirectionType(particles::IParticleStage::DIRECTION_CONE);
		stage.setDirectionParm(0, getSpinButtonValueAsFloat("coneAngleSpinner"));
	}
	else if (getGladeWidget<Gtk::RadioButton>("directionOutward")->get_active())
	{
		stage.setDirectionType(particles::IParticleStage::DIRECTION_OUTWARD);
		stage.setDirectionParm(0, getSpinButtonValueAsFloat("upwardBiasSpinner"));
	}

	getGladeWidget<Gtk::Widget>("coneAngleHBox")->set_sensitive(stage.getDirectionType() == particles::IParticleStage::DIRECTION_CONE);
	getGladeWidget<Gtk::Widget>("upwardBiasHBox")->set_sensitive(stage.getDirectionType() == particles::IParticleStage::DIRECTION_OUTWARD);

	if (getGladeWidget<Gtk::RadioButton>("orientationView")->get_active())
	{
		stage.setOrientationType(particles::IParticleStage::ORIENTATION_VIEW);
	}
	else if (getGladeWidget<Gtk::RadioButton>("orientationAimed")->get_active())
	{
		stage.setOrientationType(particles::IParticleStage::ORIENTATION_AIMED);

		stage.setOrientationParm(0, getSpinButtonValueAsFloat("aimedTrailsSpinner"));
		stage.setOrientationParm(1, getSpinButtonValueAsFloat("aimedTimeSpinner"));
	}
	else if (getGladeWidget<Gtk::RadioButton>("orientationX")->get_active())
	{
		stage.setOrientationType(particles::IParticleStage::ORIENTATION_X);
	}
	else if (getGladeWidget<Gtk::RadioButton>("orientationY")->get_active())
	{
		stage.setOrientationType(particles::IParticleStage::ORIENTATION_Y);
	}
	else if (getGladeWidget<Gtk::RadioButton>("orientationZ")->get_active())
	{
		stage.setOrientationType(particles::IParticleStage::ORIENTATION_Z);
	}

	getGladeWidget<Gtk::Widget>("aimedTrailsHBox")->set_sensitive(stage.getOrientationType() == particles::IParticleStage::ORIENTATION_AIMED);
	getGladeWidget<Gtk::Widget>("aimedTimeHBox")->set_sensitive(stage.getOrientationType() == particles::IParticleStage::ORIENTATION_AIMED);

	stage.setInitialAngle(getSpinButtonValueAsFloat("initialAngleSpinner"));
}

float ParticleEditor::getSpinButtonValueAsFloat(const std::string& widgetName)
{
	return static_cast<float>(getGladeWidget<Gtk::SpinButton>(widgetName)->get_adjustment()->get_value());
}

int ParticleEditor::getSpinButtonValueAsInt(const std::string& widgetName)
{
	return static_cast<int>(getGladeWidget<Gtk::SpinButton>(widgetName)->get_adjustment()->get_value());
}

void ParticleEditor::activateEditPanels()
{
	getGladeWidget<Gtk::Widget>("stageLabel")->set_sensitive(true);
	getGladeWidget<Gtk::Widget>("settingsLabel")->set_sensitive(true);

	activateSettingsEditPanels();
}

void ParticleEditor::deactivateEditPanels()
{
	getGladeWidget<Gtk::Widget>("stageLabel")->set_sensitive(false);
	getGladeWidget<Gtk::Widget>("stageAlignment")->set_sensitive(false);

	deactivateSettingsEditPanels();
}

void ParticleEditor::activateSettingsEditPanels()
{
	getGladeWidget<Gtk::Widget>("stageAlignment")->set_sensitive(true);
	getGladeWidget<Gtk::Widget>("settingsNotebook")->set_sensitive(true);
}

void ParticleEditor::deactivateSettingsEditPanels()
{
	getGladeWidget<Gtk::Widget>("settingsLabel")->set_sensitive(false);		
	getGladeWidget<Gtk::Widget>("settingsNotebook")->set_sensitive(false);
}

std::size_t ParticleEditor::getSelectedStageIndex()
{
	// Get the selection and store it
	Gtk::TreeModel::iterator iter = _stageSelection->get_selected();

	int value = (*iter)[_stageColumns.index];

	if (value < 0)
	{
		throw std::logic_error("Invalid stage index stored in model.");
	}

	return value;
}

void ParticleEditor::selectStage(std::size_t index)
{
	gtkutil::TreeModel::findAndSelectInteger(
		getGladeWidget<Gtk::TreeView>("stageView"), static_cast<int>(index), _stageColumns.index);
}

void ParticleEditor::_onDefSelChanged()
{
	// Get the selection and store it
	Gtk::TreeModel::iterator iter = _defSelection->get_selected();

	if (!selectionChangeAllowed())
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

		// Load particle data
		updateWidgetsFromParticle();
	}
	else
	{
		_preview->setParticle("");
		_stageList->clear();
		deactivateEditPanels();
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

		std::size_t index = (*_selectedStageIter)[_stageColumns.index];

		getGladeWidget<Gtk::Button>("moveStageUpButton")->set_sensitive(index > 0);
		getGladeWidget<Gtk::Button>("moveStageDownButton")->set_sensitive(index < _particle->getNumStages() - 1);
	}
	else
	{
		// No valid selection
		deactivateSettingsEditPanels();

		// Deactivate delete, move and toggle buttons
		isStageSelected = false;

		getGladeWidget<Gtk::Button>("moveStageUpButton")->set_sensitive(false);
		getGladeWidget<Gtk::Button>("moveStageDownButton")->set_sensitive(false);
	}

	getGladeWidget<Gtk::Button>("removeStageButton")->set_sensitive(isStageSelected);
	getGladeWidget<Gtk::Button>("toggleStageButton")->set_sensitive(isStageSelected);
	getGladeWidget<Gtk::Button>("duplicateStageButton")->set_sensitive(isStageSelected);

	// Reload the current stage data
	updateWidgetsFromStage();
}

void ParticleEditor::_onAddStage()
{
	if (!_particle) return;

	// Add a new stage at the end of the list
	std::size_t newStage = _particle->addParticleStage();

	reloadStageList();

	selectStage(newStage);
}

void ParticleEditor::_onRemoveStage()
{
	if (!_particle || !_selectedStageIter) return;

	_particle->removeParticleStage(getSelectedStageIndex());

	reloadStageList();
}

void ParticleEditor::_onToggleStage()
{
	if (!_particle || !_selectedStageIter) return;

	std::size_t index = getSelectedStageIndex();

	particles::IParticleStage& stage = _particle->getParticleStage(index);

	stage.setVisible(!stage.isVisible());

	reloadStageList();
	selectStage(index);
}

void ParticleEditor::_onMoveUpStage()
{
	if (!_particle) return;

	std::size_t selIndex = getSelectedStageIndex();
	assert(selIndex > 0);

	_particle->swapParticleStages(selIndex, selIndex - 1);

	reloadStageList();
	selectStage(selIndex - 1);
}

void ParticleEditor::_onMoveDownStage()
{
	if (!_particle) return;

	std::size_t selIndex = getSelectedStageIndex();
	assert(_particle->getNumStages() > 0 && selIndex < _particle->getNumStages() - 1);

	_particle->swapParticleStages(selIndex, selIndex + 1);

	reloadStageList();
	selectStage(selIndex + 1);
}

void ParticleEditor::_onDuplicateStage()
{
	if (!_particle) return;

	std::size_t srcStageIndex = getSelectedStageIndex();
	std::size_t newStageIndex = _particle->addParticleStage();

	particles::IParticleStage& srcStage = _particle->getParticleStage(srcStageIndex);
	particles::IParticleStage& newStage = _particle->getParticleStage(newStageIndex);

	newStage.copyFrom(srcStage);

	reloadStageList();

	selectStage(newStageIndex);
}

void ParticleEditor::updateWidgetsFromParticle()
{
	if (!_particle) return;

	// Load stages
	reloadStageList();

	// Load stage data into controls
	updateWidgetsFromStage();
}

void ParticleEditor::reloadStageList()
{
	if (!_particle) return;

	// Load stages
	_stageList->clear();

	for (std::size_t i = 0; i < _particle->getNumStages(); ++i)
	{
		const particles::IParticleStage& stage = _particle->getParticleStage(i);

		Gtk::TreeModel::iterator iter = _stageList->append();

		(*iter)[_stageColumns.name] = (boost::format("Stage %d") % static_cast<int>(i)).str();
		(*iter)[_stageColumns.index] = static_cast<int>(i);
		(*iter)[_stageColumns.visible] = true;
		(*iter)[_stageColumns.colour] = stage.isVisible() ? "#000000" : "#707070";
	}

	// Select the first stage if possible
	gtkutil::TreeModel::findAndSelectInteger(getGladeWidget<Gtk::TreeView>("stageView"), 0, _stageColumns.index);
}

void ParticleEditor::updateWidgetsFromStage()
{
	if (!_particle || !_selectedStageIter) return;
	
	_callbacksDisabled = true;

	const particles::IParticleStage& stage = _particle->getParticleStage(getSelectedStageIndex());

	getGladeWidget<Gtk::Entry>("shaderEntry")->set_text(stage.getMaterialName());

	const Vector4& colour = stage.getColour();
	getGladeWidget<Gtk::Entry>("colourEntry")->set_text(
		(boost::format("%.2f %.2f %.2f %.2f") % colour.x() % colour.y() % colour.z() % colour.w()).str()
	);

	const Vector4& fadeColour = stage.getFadeColour();
	getGladeWidget<Gtk::Entry>("fadeColourEntry")->set_text(
		(boost::format("%.2f %.2f %.2f %.2f") % fadeColour.x() % fadeColour.y() % fadeColour.z() % fadeColour.w()).str()
	);

	getGladeWidget<Gtk::SpinButton>("fadeInFractionSpinner")->get_adjustment()->set_value(stage.getFadeInFraction());
	getGladeWidget<Gtk::SpinButton>("fadeOutFractionSpinner")->get_adjustment()->set_value(stage.getFadeOutFraction());
	getGladeWidget<Gtk::SpinButton>("fadeIndexFractionSpinner")->get_adjustment()->set_value(stage.getFadeIndexFraction());
	getGladeWidget<Gtk::SpinButton>("animFramesSpinner")->get_adjustment()->set_value(stage.getAnimationFrames());
	getGladeWidget<Gtk::SpinButton>("animRateSpinner")->get_adjustment()->set_value(stage.getAnimationRate());

	getGladeWidget<Gtk::SpinButton>("countSpinner")->get_adjustment()->set_value(stage.getCount());
	getGladeWidget<Gtk::SpinButton>("timeSpinner")->get_adjustment()->set_value(stage.getDuration());
	getGladeWidget<Gtk::SpinButton>("bunchingSpinner")->get_adjustment()->set_value(stage.getBunching());
	getGladeWidget<Gtk::SpinButton>("cyclesSpinner")->get_adjustment()->set_value(stage.getCycles());
	getGladeWidget<Gtk::SpinButton>("timeOffsetSpinner")->get_adjustment()->set_value(stage.getTimeOffset());
	getGladeWidget<Gtk::SpinButton>("deadTimeSpinner")->get_adjustment()->set_value(stage.getDeadTime());

	// DISTRIBUTION

	bool useRingSize = false;

	switch (stage.getDistributionType())
	{
	case particles::IParticleStage::DISTRIBUTION_RECT:
		getGladeWidget<Gtk::RadioButton>("distRectangle")->set_active(true);
		break;
	case particles::IParticleStage::DISTRIBUTION_CYLINDER:
		getGladeWidget<Gtk::RadioButton>("distCylinder")->set_active(true);
		useRingSize = true;
		break;
	case particles::IParticleStage::DISTRIBUTION_SPHERE:
		getGladeWidget<Gtk::RadioButton>("distSphere")->set_active(true);
		useRingSize = true;
		break;
	};

	getGladeWidget<Gtk::Widget>("distSizeRingHBox")->set_sensitive(useRingSize);
	getGladeWidget<Gtk::Widget>("distSizeRingLabel")->set_sensitive(useRingSize);
	
	getGladeWidget<Gtk::SpinButton>("distSizeXSpinner")->get_adjustment()->set_value(stage.getDistributionParm(0));
	getGladeWidget<Gtk::SpinButton>("distSizeYSpinner")->get_adjustment()->set_value(stage.getDistributionParm(1));
	getGladeWidget<Gtk::SpinButton>("distSizeZSpinner")->get_adjustment()->set_value(stage.getDistributionParm(2));
	getGladeWidget<Gtk::SpinButton>("distSizeRingSpinner")->get_adjustment()->set_value(stage.getDistributionParm(3));
	getGladeWidget<Gtk::Entry>("distOffsetEntry")->set_text(std::string(stage.getOffset()));
	getGladeWidget<Gtk::CheckButton>("distRandom")->set_active(stage.getRandomDistribution());

	// DIRECTION / ORIENTATION

	switch (stage.getDirectionType())
	{
	case particles::IParticleStage::DIRECTION_CONE:
		getGladeWidget<Gtk::RadioButton>("directionCone")->set_active(true);
		getGladeWidget<Gtk::SpinButton>("coneAngleSpinner")->get_adjustment()->set_value(stage.getDirectionParm(0));
		break;
	case particles::IParticleStage::DIRECTION_OUTWARD:
		getGladeWidget<Gtk::RadioButton>("directionOutward")->set_active(true);
		getGladeWidget<Gtk::SpinButton>("upwardBiasSpinner")->get_adjustment()->set_value(stage.getDirectionParm(0));
		break;
	};

	getGladeWidget<Gtk::Widget>("coneAngleHBox")->set_sensitive(stage.getDirectionType() == particles::IParticleStage::DIRECTION_CONE);
	getGladeWidget<Gtk::Widget>("upwardBiasHBox")->set_sensitive(stage.getDirectionType() == particles::IParticleStage::DIRECTION_OUTWARD);

	// Orientation Type
	switch (stage.getOrientationType())
	{
	case particles::IParticleStage::ORIENTATION_VIEW:
		getGladeWidget<Gtk::RadioButton>("orientationView")->set_active(true);
		break;
	case particles::IParticleStage::ORIENTATION_AIMED:
		getGladeWidget<Gtk::RadioButton>("orientationAimed")->set_active(true);
		getGladeWidget<Gtk::SpinButton>("aimedTrailsSpinner")->get_adjustment()->set_value(stage.getOrientationParm(0));
		getGladeWidget<Gtk::SpinButton>("aimedTimeSpinner")->get_adjustment()->set_value(stage.getOrientationParm(1));
		break;
	case particles::IParticleStage::ORIENTATION_X:
		getGladeWidget<Gtk::RadioButton>("orientationX")->set_active(true);

		break;
	case particles::IParticleStage::ORIENTATION_Y:
		getGladeWidget<Gtk::RadioButton>("orientationY")->set_active(true);
		break;
	case particles::IParticleStage::ORIENTATION_Z:
		getGladeWidget<Gtk::RadioButton>("orientationZ")->set_active(true);
		break;
	};

	getGladeWidget<Gtk::Widget>("aimedTrailsHBox")->set_sensitive(stage.getOrientationType() == particles::IParticleStage::ORIENTATION_AIMED);
	getGladeWidget<Gtk::Widget>("aimedTimeHBox")->set_sensitive(stage.getOrientationType() == particles::IParticleStage::ORIENTATION_AIMED);

	getGladeWidget<Gtk::SpinButton>("initialAngleSpinner")->get_adjustment()->set_value(stage.getInitialAngle());

	_callbacksDisabled = false;
}

void ParticleEditor::setupEditParticle()
{
	Gtk::TreeModel::iterator iter = _defSelection->get_selected();

	if (!iter) return;

	std::string selectedParticle = (*iter)[_defColumns.name];

	particles::IParticleDefPtr particleDef = GlobalParticlesManager().getParticle(selectedParticle);

	if (particleDef == NULL)
	{
		_preview->setParticle("");
		return;
	}

	// Generate a temporary name for this particle, and instantiate a copy
	std::string temporaryParticleName = selectedParticle + EDIT_SUFFIX;

	_particle = particles::ParticlesManager::Instance().findOrInsertParticleDef(temporaryParticleName);
	_particle->setFilename(particleDef->getFilename());

	_particle->copyFrom(*particleDef);

	// Point the preview to this temporary particle def
	_preview->setParticle(_particle->getName());
}

void ParticleEditor::releaseEditParticle()
{
	if (_particle && boost::algorithm::ends_with(_particle->getName(), EDIT_SUFFIX))
	{
		particles::ParticlesManager::Instance().removeParticleDef(_particle->getName());
	}
	
	_particle.reset();
}

bool ParticleEditor::particleHasUnsavedChanges()
{
	// Get the selection and store it
	Gtk::TreeModel::iterator iter = _defSelection->get_selected();

	if (_selectedDefIter && _particle && iter && _selectedDefIter != iter)
	{
		// Particle selection changed, check if we have any unsaved changes
		std::string originalParticleName = (*_selectedDefIter)[_defColumns.name];

		particles::IParticleDefPtr originalParticle = GlobalParticlesManager().getParticle(originalParticleName);

		if (!originalParticle || *_particle != *originalParticle)
		{
			return true;
		}
	}

	return false;
}

bool ParticleEditor::askForSave()
{
	// Get the original particle name
	std::string originalParticleName = (*_selectedDefIter)[_defColumns.name];

	particles::IParticleDefPtr originalParticle = GlobalParticlesManager().getParticle(originalParticleName);

	// The particle we're editing has been changed from the saved one
	gtkutil::MessageBox box(_("Save Changes"), 
		(boost::format(_("Do you want to save the changes\nyou made to the particle %s?")) % originalParticleName).str(),
		IDialog::MESSAGE_SAVECONFIRMATION, GlobalMainFrame().getTopLevelWindow());

	IDialog::Result result = box.run();
				
	return (result == IDialog::RESULT_YES);
}

bool ParticleEditor::selectionChangeAllowed()
{
	if (particleHasUnsavedChanges())
	{
		if (askForSave())
		{
			// Attempt to save the particle, return true on success to allow selection change
			return saveCurrentParticle();
		}
		
		// User does not want to save, selection change is allowed
		return true;
	}

	// No changes, selection change allowed
	return true;
}

bool ParticleEditor::saveCurrentParticle()
{
	// Get the original particle name
	std::string originalParticleName = (*_selectedDefIter)[_defColumns.name];

	particles::IParticleDefPtr originalParticle = GlobalParticlesManager().getParticle(originalParticleName);

	// If the particle is not existing at all yet, create a new one
	// TODO: This is not needed when the "New Particle" algorithm is working ok
	if (!originalParticle)
	{
		particles::ParticleDefPtr newParticle = 
			particles::ParticlesManager::Instance().findOrInsertParticleDef(originalParticleName);

		newParticle->setFilename(""); // TODO
		
		originalParticle = newParticle;
	}

	// Write the changes from the working copy into the actual instance
	originalParticle->copyFrom(*_particle);

	// Write changes to disk, and return the result
	return particles::ParticlesManager::Instance().saveParticleDef(originalParticle->getName());
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
	if (particleHasUnsavedChanges() && askForSave() && !saveCurrentParticle())
	{
		// Particle has unsaved changes, user wants to save the particle, 
		// but the save attempt failed, inhibit window destruction
		return;
	}

	// Close the window
	destroy();
}

void ParticleEditor::displayDialog(const cmd::ArgumentList& args)
{
	ParticleEditor editor;
	editor.show();
}

}
