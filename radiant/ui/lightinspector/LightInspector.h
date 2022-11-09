#pragma once

#include "icommandsystem.h"
#include "ui/materials/MaterialSelector.h"
#include "wxutil/XmlResourceBasedWidget.h"

#include <map>
#include <string>
#include <sigc++/connection.h>
#include <sigc++/trackable.h>

#include "wxutil/DockablePanel.h"

/* FORWARD DECLS */
class Entity;
class wxColourPickerEvent;

class wxSlider;

namespace ui
{

/**
 * \brief Dialog to allow adjustment of properties on lights
 */
class LightInspector :
    public wxutil::DockablePanel,
    public sigc::trackable,
    private wxutil::XmlResourceBasedWidget
{
    // Projected light flag
    bool _isProjected;

    // Internal widgets
    MaterialSelector* _texSelector;
    wxSlider* _brightnessSlider;

    // The light entities to edit
    typedef std::vector<Entity*> EntityList;
    EntityList _lightEntities;

    // Table of original value keys, to avoid replacing them with defaults
    typedef std::map<std::string, std::string> StringMap;
    StringMap _valueMap;

    // Disables callbacks if set to TRUE (during widget updates)
    bool _updateActive;

    // Track if a brightness adjustment is in progress (which needs an undo
    // command when finished)
    bool _adjustingBrightness = false;

    bool _supportsAiSee;

    sigc::connection _selectionChanged;
    sigc::connection _undoHandler;
    sigc::connection _redoHandler;

public:
    LightInspector(wxWindow* parent);
    ~LightInspector() override;

protected:
    void onPanelActivated() override;
    void onPanelDeactivated() override;

private:
    void connectListeners();
    void disconnectListeners();

    // Widget construction functions
    void setupLightShapeOptions();
    void bindSpawnargToCheckbox(std::string spawnarg, std::string checkbox);
    void setupOptionsPanel();
    void setupTextureWidgets();

    // Callbacks
    void _onProjToggle(wxCommandEvent& ev);
    void _onColourChange(wxColourPickerEvent& ev);
    void adjustBrightness() const;

    // Get value of a checkbox as a spawnarg string (1 or 0)
	std::string checkboxValue(std::string cbName) const;

    void updateColourPicker();
    void updateColourWidgets();
    void updateLightShapeWidgets();

    // Get the highest RGB component of ALL selected light colours
    float highestComponentAllLights() const;

    // Update the dialog widgets from keyvals on the first selected entity
    void getValuesFromEntity();

    // Write the widget contents to the given entity
    void setLightVectorsOnEntity(Entity* entity);

    // Write contents to all light entities. Pass a string map containing
    // explicit values to set, or an empty map to call setLightVectorsOnEntity() for
    // each entity.
    void writeToAllEntities(StringMap newValues = {});

    // Set the given key/value pair on ALL entities in the list of lights
    void setKeyValueAllLights(const std::string& k, const std::string& v);

    // greebo: Gets called when the light texture selection has changed
    void shaderSelectionChanged();

    // Update the sensitivity of the widgets
    void update();
};

} // namespace
