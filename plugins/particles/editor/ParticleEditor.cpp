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
	_preview(GlobalUIManager().createParticlePreview())
{
	// Window properties
    set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);
    set_position(Gtk::WIN_POS_CENTER_ON_PARENT);
    
    // Add vbox to dialog
    add(*getGladeWidget<Gtk::Widget>("mainVbox"));
    g_assert(get_child() != NULL);

	// Wire up the buttons
	getGladeWidget<Gtk::Button>("cancelButton")->signal_clicked().connect(
        sigc::mem_fun(*this, &ParticleEditor::_onCancel)
    );
	getGladeWidget<Gtk::Button>("okButton")->signal_clicked().connect(
        sigc::mem_fun(*this, &ParticleEditor::_onOK)
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

	// Fire the selection changed signal to initialise the sensitivity
	_onDefSelChanged();
	_onStageSelChanged();
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

		// Reload the current stage data
		// TODO

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

bool ParticleEditor::selectionChangeAllowed()
{
	// Get the selection and store it
	Gtk::TreeModel::iterator iter = _defSelection->get_selected();

	if (_selectedDefIter && _particle != NULL && iter && _selectedDefIter != iter)
	{
		// Particle selection changed, check if we have any unsaved changes
		std::string originalParticleName = (*_selectedDefIter)[_defColumns.name];

		particles::IParticleDefPtr originalParticle = GlobalParticlesManager().getParticle(originalParticleName);

		if (!originalParticle || *_particle != *originalParticle)
		{
			// The particle we're editing has been changed from the saved one
			gtkutil::MessageBox box(_("Save Changes"), 
				(boost::format(_("Do you want to save the changes\nyou made to the particle %s?")) % originalParticleName).str(),
				IDialog::MESSAGE_SAVECONFIRMATION, GlobalMainFrame().getTopLevelWindow());

			IDialog::Result result = box.run();

			if (result == IDialog::RESULT_CANCELLED)
			{
				return false;
			}
			
			if (result = IDialog::RESULT_YES)
			{
				// Save the changes
				// TODO
			}

			return true;
		}
	}

	return true;
}

void ParticleEditor::_onCancel()
{
	// Close the window
	destroy();
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

void ParticleEditor::_onOK()
{
	// Close the window
	destroy();
}

void ParticleEditor::displayDialog(const cmd::ArgumentList& args)
{
	ParticleEditor editor;
	editor.show();
}

}
