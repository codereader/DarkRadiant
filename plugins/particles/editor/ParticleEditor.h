#pragma once

#include "icommandsystem.h"
#include "gtkutil/window/BlockingTransientWindow.h"
#include "gtkutil/GladeWidgetHolder.h"
#include "gtkutil/WindowPosition.h"

#include <gtkmm/liststore.h>
#include <gtkmm/treeselection.h>

#include "iparticlepreview.h"

#include "../ParticleDef.h"

namespace ui
{

class ParticleEditor :
	public gtkutil::BlockingTransientWindow,
    private gtkutil::GladeWidgetHolder
{
public:
	// Treemodel definition
	struct DefColumns :
		public Gtk::TreeModel::ColumnRecord
	{
		DefColumns() { add(name); }

		Gtk::TreeModelColumn<std::string> name;
	};

	// Treemodel definition
	struct StageColumns :
		public Gtk::TreeModel::ColumnRecord
	{
		StageColumns() { add(name); add(index); add(visible); add(colour); }

		Gtk::TreeModelColumn<Glib::ustring> name;
		Gtk::TreeModelColumn<int> index;
		Gtk::TreeModelColumn<bool> visible;
		Gtk::TreeModelColumn<Glib::ustring> colour;
	};

private:
	// List of target_addobjectives entities
	DefColumns _defColumns;
	Glib::RefPtr<Gtk::ListStore> _defList;

	Glib::RefPtr<Gtk::TreeSelection> _defSelection;

	StageColumns _stageColumns;
	Glib::RefPtr<Gtk::ListStore> _stageList;

	Glib::RefPtr<Gtk::TreeSelection> _stageSelection;

	IParticlePreviewPtr _preview;

	// The position/size memoriser
	gtkutil::WindowPosition _windowPosition;

	// The currently selected rows in the model
	Gtk::TreeModel::iterator _selectedDefIter;
	Gtk::TreeModel::iterator _selectedStageIter;

	// The particle definition we're working on
	particles::ParticleDefPtr _particle;

	bool _callbacksDisabled;

private:
	ParticleEditor();

public:
	/**
	 * Static method to display the Particles Editor dialog.
	 */
	static void displayDialog(const cmd::ArgumentList& args);

private:
	// Override the delete event
	void _onDeleteEvent();

	void _preHide();
	void _preShow();
	void _postShow();

	// gtkmm callbacks
	void _onClose();
	void _onDefSelChanged();
	void _onStageSelChanged();

	void _onNewParticle();
	void _onSaveParticle();
	void _onSaveAsParticle();

	// Returns a new, not-already-existing particle def name, returns empty on cancel
	std::string queryNewParticleName();
	std::string queryParticleFile();

	// Highlight the named particle in the treeview
	void selectParticleDef(const std::string& particleDefName);

	void _onAddStage();
	void _onRemoveStage();
	void _onToggleStage();
	void _onMoveUpStage();
	void _onMoveDownStage();
	void _onDuplicateStage();

	void _onShaderControlsChanged();
	void _onCountTimeControlsChanged();
	void _onDistributionControlsChanged();
	void _onDirectionControlsChanged();
	void _onSizeControlsChanged();
	void _onPathControlsChanged();

	void updatePathWidgetSensitivity();

	// Returns the 0-based index of the current stage
	std::size_t getSelectedStageIndex();
	void selectStage(std::size_t index);

	void setupParticleDefList();
	void populateParticleDefList();

	void setupParticleStageList();
	void setupSettingsPages();

	void activateEditPanels();
	void deactivateEditPanels();

	void activateSettingsEditPanels();
	void deactivateSettingsEditPanels();

	bool particleHasUnsavedChanges();
	bool selectionChangeAllowed();

	// Asks the user whether to save the particle, returns true on "yes"
	bool askForSave();

	bool saveCurrentParticle();

	void setupEditParticle();
	void releaseEditParticle();

	void updateWidgetsFromParticle();
	void updateWidgetsFromStage();
	void reloadStageList();

	// Returns the current value of the given spin button as float/int
	float getSpinButtonValueAsFloat(const std::string& widgetName);
	int getSpinButtonValueAsInt(const std::string& widgetName);
};

} // namespace
