#pragma once

#include "icommandsystem.h"
#include "iradiant.h"
#include "ui/common/ShaderSelector.h"
#include "wxutil/window/TransientWindow.h"
#include "wxutil/XmlResourceBasedWidget.h"

#include <map>
#include <string>
#include <sigc++/connection.h>
#include <sigc++/trackable.h>

/* FORWARD DECLS */
class Entity;
class wxColourPickerEvent;

class wxSlider;

namespace ui
{

class LightInspector;
typedef std::shared_ptr<LightInspector> LightInspectorPtr;

/**
 * \brief Dialog to allow adjustment of properties on lights
 */
class LightInspector :
    public wxutil::TransientWindow,
    public ShaderSelector::Client,
    public sigc::trackable,
    private wxutil::XmlResourceBasedWidget
{
    // Projected light flag
    bool _isProjected;

    // Internal widgets
    ShaderSelector* _texSelector;
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

private:
    // This is where the static shared_ptr of the singleton instance is held.
    static LightInspectorPtr& InstancePtr();

    // Constructor creates GTK widgets
    LightInspector();

    // TransientWindow callbacks
    virtual void _preShow();
    virtual void _preHide();

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

    /** greebo: Gets called when the shader selection gets changed, so that
     *          the displayed texture info can be updated.
     */
    void shaderSelectionChanged(const std::string& shader, wxutil::TreeModel& listStore);

    // Safely disconnects this dialog from all the systems
    // and saves the window size/position to the registry
    void onMainFrameShuttingDown();

public:

    /** Toggle the visibility of the dialog instance, constructing it if necessary.
     */
    static void toggleInspector(const cmd::ArgumentList& args);

    /** greebo: This is the actual home of the static instance
     */
    static LightInspector& Instance();

    // Update the sensitivity of the widgets
    void update();
};

} // namespace
