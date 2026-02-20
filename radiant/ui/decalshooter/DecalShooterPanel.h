#pragma once

#include <sigc++/trackable.h>
#include "wxutil/DockablePanel.h"
#include "iselectiongroup.h"

class wxSpinCtrlDouble;
class wxTextCtrl;
class wxButton;
class wxCheckBox;
class wxChoice;

namespace ui
{

/**
 * Panel for configuring the Decal Shooter tool settings.
 */
class DecalShooterPanel :
    public wxutil::DockablePanel,
    public sigc::trackable
{
private:
    wxSpinCtrlDouble* _widthCtrl;
    wxSpinCtrlDouble* _heightCtrl;
    wxSpinCtrlDouble* _offsetCtrl;
    wxSpinCtrlDouble* _rotationCtrl;
    wxCheckBox* _randomRotationCheckbox;
    wxTextCtrl* _materialEntry;
    wxButton* _browseButton;
    wxCheckBox* _autogroupCheckbox;
    wxCheckBox* _flipCheckbox;
    wxChoice* _layerChoice;
    selection::ISelectionGroupPtr _currentSessionGroup;

    sigc::connection _mapEventConnection;
    sigc::connection _layersChangedConnection;

    static DecalShooterPanel* _instance;

public:
    DecalShooterPanel(wxWindow* parent);
    ~DecalShooterPanel() override;

    double getDecalWidth() const;
    double getDecalHeight() const;
    double getDecalOffset() const;
    double getDecalRotation() const;
    bool isRandomRotationEnabled() const;
    bool isFlipEnabled() const;
    std::string getDecalMaterial() const;
    bool isAutogroupEnabled() const;
    int getSelectedLayerId() const;

    // DecalShooterTool calls this when a decal is created
    void onDecalCreated(const scene::INodePtr& decalNode);

    void resetSessionGroup();

    static DecalShooterPanel* getInstance();

protected:
    void onPanelActivated() override;
    void onPanelDeactivated() override;

private:
    void populateWindow();
    void populateLayerChoice();
    void connectToMapRoot();
    void onBrowseMaterial(wxCommandEvent& ev);
    void onAutogroupToggled(wxCommandEvent& ev);
    void onRandomRotationToggled(wxCommandEvent& ev);
    void onMapEvent(IMap::MapEvent ev);
    void onLayersChanged();
};

} // namespace ui
