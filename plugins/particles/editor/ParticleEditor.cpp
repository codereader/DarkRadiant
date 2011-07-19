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

#include "ParticleDefPopulator.h"

namespace ui
{

// CONSTANTS
namespace
{
	const char* const DIALOG_TITLE = N_("Particle Editor");

	const std::string RKEY_ROOT = "user/ui/particleEditor/";
	const std::string RKEY_WINDOW_STATE = RKEY_ROOT + "window";
}

ParticleEditor::ParticleEditor() :
	gtkutil::BlockingTransientWindow(DIALOG_TITLE, GlobalMainFrame().getTopLevelWindow()),
	gtkutil::GladeWidgetHolder(GlobalUIManager().getGtkBuilderFromFile("ParticleEditor.glade")),
	_defList(Gtk::ListStore::create(_defColumns)),
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

	// Fire the selection changed signal to initialise the sensitiveness
	_onSelChanged();
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
	_defSelection->signal_changed().connect(sigc::mem_fun(*this, &ParticleEditor::_onSelChanged));
}

void ParticleEditor::populateParticleDefList()
{
	_defList->clear();

	// Create and use a ParticlesVisitor to populate the list
	ParticlesVisitor visitor(_defList, _defColumns);
	GlobalParticlesManager().forEachParticleDef(visitor);
}

void ParticleEditor::activateEditPanels()
{
	getGladeWidget<Gtk::Widget>("stageLabel")->set_sensitive(true);
	getGladeWidget<Gtk::Widget>("settingsLabel")->set_sensitive(true);

	getGladeWidget<Gtk::Widget>("stageAlignment")->set_sensitive(true);
	getGladeWidget<Gtk::Widget>("settingsNotebook")->set_sensitive(true);
}

void ParticleEditor::deactivateEditPanels()
{
	getGladeWidget<Gtk::Widget>("stageLabel")->set_sensitive(false);
	getGladeWidget<Gtk::Widget>("settingsLabel")->set_sensitive(false);		

	getGladeWidget<Gtk::Widget>("stageAlignment")->set_sensitive(false);
	getGladeWidget<Gtk::Widget>("settingsNotebook")->set_sensitive(false);
}

void ParticleEditor::_onSelChanged()
{
	// Get the selection and store it
	Gtk::TreeModel::iterator iter = _defSelection->get_selected();

	if (!selectionChangeAllowed())
	{
		// Revert the selection (re-enter this function) and cancel the operation
		_defSelection->select(_selectedIter);
		return;
	}

	if (_selectedIter == iter)
	{
		return; // nothing to do so far
	}

	// Store new selection
	_selectedIter = iter;

	if (_selectedIter)
	{
		// Copy the particle def and set it up for editing
		setupEditParticle();

		activateEditPanels();

		// TODO Load particle data
	}
	else
	{
		// TODO Clear working particle
		
		_preview->setParticle("");

		deactivateEditPanels();
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

	_preview->setParticle(selectedParticle);
}

bool ParticleEditor::selectionChangeAllowed()
{
	// Get the selection and store it
	Gtk::TreeModel::iterator iter = _defSelection->get_selected();

	if (_selectedIter && _particle != NULL && _selectedIter != iter)
	{
		// Particle selection changed, check if we have any unsaved changes
		std::string originalParticleName = (*_selectedIter)[_defColumns.name];

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
