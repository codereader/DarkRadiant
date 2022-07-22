#pragma once

#include "icommandsystem.h"
#include "wxutil/dialog/DialogBase.h"
#include "ui/idialogmanager.h"
#include "wxutil/XmlResourceBasedWidget.h"
#include "wxutil/preview/ParticlePreview.h"
#include "wxutil/dataview/TreeView.h"
#include "wxutil/WindowPosition.h"
#include "wxutil/PanedPosition.h"

class wxSpinCtrlDouble;
class wxSpinCtrl;
class wxSpinDoubleEvent;

namespace ui
{

/// Editor dialog for creating and modifying particle systems
class ParticleEditor :
    public wxutil::DialogBase,
    private wxutil::XmlResourceBasedWidget
{
private:
    // List of particle system defs
    wxutil::TreeModel::Ptr _defList;
	wxutil::TreeView* _defView;

    // List of stages in the current particle def
    wxutil::TreeModel::Ptr _stageList;
	wxutil::TreeView* _stageView;

    wxutil::ParticlePreviewPtr _preview;

    // The position/size memoriser
    wxutil::WindowPosition _windowPosition;
	wxutil::PanedPosition _panedPosition;

    // The currently selected rows in the model
    wxDataViewItem _selectedDefIter;
    wxDataViewItem _selectedStageIter;

    // The particle definition we're working on
    particles::IParticleDef::Ptr _currentDef;

    bool _callbacksDisabled;
    bool _saveInProgress;

private:
    ParticleEditor();

public:
    /**
     * Static method to display the Particles Editor dialog.
     */
    static void DisplayDialog(const cmd::ArgumentList& args);

	int ShowModal();

protected:
	// Override DialogBase
    bool _onDeleteEvent();

private:
	void handleDefSelChanged();
    void handleStageSelChanged();

    // callbacks
    void _onClose(wxCommandEvent& ev);
    void _onDefSelChanged(wxDataViewEvent& ev);
    void _onStageSelChanged(wxDataViewEvent& ev);

    void _onNewParticle(wxCommandEvent& ev);
	void _onSaveParticle(wxCommandEvent& ev);
    void _onCloneCurrentParticle(wxCommandEvent& ev);
    void setSaveButtonsSensitivity(bool sensitive);

    // Returns a new, not-already-existing particle def name, returns empty on cancel
    std::string queryNewParticleName();
    std::string queryParticleFile();

    // Creates a particle with a new name (queried from the user), selects it and returns the particle ref
    // Returns NULL if the user cancelled in some way
    particles::IParticleDef::Ptr createAndSelectNewParticle();

    // Highlight the named particle in the treeview
    void selectParticleDef(const std::string& particleDefName);

    void _onAddStage(wxCommandEvent& ev);
    void _onRemoveStage(wxCommandEvent& ev);
    void _onToggleStage(wxCommandEvent& ev);
    void _onMoveUpStage(wxCommandEvent& ev);
    void _onMoveDownStage(wxCommandEvent& ev);
    void _onDuplicateStage(wxCommandEvent& ev);

    // A pointer-to-member function typedef
    typedef void (ParticleEditor::*MemberMethod)(wxCommandEvent& ev);

	// Replace the given wxSpinCtrl with a wxSpinCtrlDouble
	wxSpinCtrlDouble* convertToSpinCtrlDouble(const std::string& name, double min, double max, double increment, int digits = 2);

    // Connect a spin button to call the given member method
	void connectSpinner(const std::string& name, MemberMethod func);

    void _onShaderControlsChanged(wxCommandEvent& ev);
    void _onCountTimeControlsChanged(wxCommandEvent& ev);
    void _onDistributionControlsChanged(wxCommandEvent& ev);
    void _onDirectionControlsChanged(wxCommandEvent& ev);
    void _onSizeControlsChanged(wxCommandEvent& ev);
    void _onPathControlsChanged(wxCommandEvent& ev);
    void _onDepthHackChanged(wxSpinDoubleEvent& ev);

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
	std::string getParticleNameFromIter(const wxDataViewItem& item);
	void setSpinCtrlValue(const std::string& name, double value);
};

} // namespace
