#pragma once

#include "icommandsystem.h"
#include "gtkutil/window/BlockingTransientWindow.h"
#include "gtkutil/GladeWidgetHolder.h"
#include "gtkutil/WindowPosition.h"
#include "gtkutil/dialog/Dialog.h"
#include "gtkutil/preview/ParticlePreview.h"

#include <gtkmm/liststore.h>
#include <gtkmm/treeselection.h>

#include "../ParticleDef.h"

namespace ui
{

/// Editor dialog for creating and modifying particle systems
class ParticleEditor :
    public gtkutil::BlockingTransientWindow,
    private gtkutil::GladeWidgetHolder
{
    // List of particle system defs
    Glib::RefPtr<Gtk::ListStore> _defList;
    Glib::RefPtr<Gtk::TreeSelection> _defSelection;

    // List of stages in the current particle def
    Glib::RefPtr<Gtk::ListStore> _stageList;
    Glib::RefPtr<Gtk::TreeSelection> _stageSelection;

    gtkutil::ParticlePreviewPtr _preview;

    // The position/size memoriser
    gtkutil::WindowPosition _windowPosition;

    // The currently selected rows in the model
    Gtk::TreeModel::iterator _selectedDefIter;
    Gtk::TreeModel::iterator _selectedStageIter;

    // The particle definition we're working on
    particles::ParticleDefPtr _currentDef;

    bool _callbacksDisabled;
    bool _saveInProgress;

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
    void cloneCurrentParticle();
    void setSaveButtonsSensitivity(bool sensitive);

    // Returns a new, not-already-existing particle def name, returns empty on cancel
    std::string queryNewParticleName();
    std::string queryParticleFile();

    // Creates a particle with a new name (queried from the user), selects it and returns the particle ref
    // Returns NULL if the user cancelled in some way
    particles::ParticleDefPtr createAndSelectNewParticle();

    // Highlight the named particle in the treeview
    void selectParticleDef(const std::string& particleDefName);

    void _onAddStage();
    void _onRemoveStage();
    void _onToggleStage();
    void _onMoveUpStage();
    void _onMoveDownStage();
    void _onDuplicateStage();

    // A pointer-to-member function typedef
    typedef void (ParticleEditor::*MemberMethod)();

    // Connect a spin button to call the given member method
    void connectSpinner(const std::string& name, MemberMethod func);
    bool _onSpinButtonKeyRelease(GdkEventKey*, MemberMethod func);

    void _onShaderControlsChanged();
    void _onCountTimeControlsChanged();
    void _onDistributionControlsChanged();
    void _onDirectionControlsChanged();
    void _onSizeControlsChanged();
    void _onPathControlsChanged();
    void _onDepthHackChanged();

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

    // Asks the user whether to save the particle, returns the dialog run result
    IDialog::Result askForSave();

    // Returns true if leaving the current working particle went ok (was saved,
    // or no unsaved changes)
    bool promptUserToSaveChanges(bool requireSelectionChange = true);
    bool defSelectionHasChanged();

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
