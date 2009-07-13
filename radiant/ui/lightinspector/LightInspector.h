#ifndef LIGHTINSPECTOR_H_
#define LIGHTINSPECTOR_H_

#include "iselection.h"
#include "icommandsystem.h"
#include "iundo.h"
#include "iradiant.h"
#include "ui/common/ShaderSelector.h"
#include "gtkutil/WindowPosition.h"
#include "gtkutil/window/PersistentTransientWindow.h"

#include <gtk/gtkwidget.h>
#include <map>
#include <string>

/* FORWARD DECLS */
typedef struct _GtkColorButton GtkColorButton;
typedef struct _GtkToggleButton GtkToggleButton;
class Entity;
namespace gtkutil {
	class Vector3Entry;
}

namespace ui
{

/** Dialog to allow adjustment of properties on lights, including the conversion
 * between projected and point lights.
 */
class LightInspector;
typedef boost::shared_ptr<LightInspector> LightInspectorPtr;

class LightInspector 
: public gtkutil::PersistentTransientWindow,
  public SelectionSystem::Observer,
  public ShaderSelector::Client,
  public RadiantEventListener,
  public UndoSystem::Observer
{
	// The overall vbox
	GtkWidget* _mainVBox;
	
	// Projected light flag
	bool _isProjected;

	// Light type toggle buttons
	GtkWidget* _pointLightToggle;
	GtkWidget* _projLightToggle;
	
	// Colour selection widget
	GtkWidget* _colour;
	
	// Texture selection combo
	ShaderSelector _texSelector;

	// Checkbox to enable start/end for projected lights
	GtkWidget* _useStartEnd;

	// Options checkboxes
	typedef std::map<std::string, GtkWidget*> WidgetMap;
	WidgetMap _options;
	
	// The light entity to edit
    typedef std::vector<Entity*> EntityList;
    EntityList _lightEntities;
	
	// Table of original value keys, to avoid replacing them with defaults
	typedef std::map<std::string, std::string> StringMap;
	StringMap _valueMap;
	
	gtkutil::WindowPosition _windowPosition;
	
	// Disables GTK callbacks if set to TRUE (during widget updates)
	bool _updateActive;
	
private:
	// This is where the static shared_ptr of the singleton instance is held.
	static LightInspectorPtr& InstancePtr();

	// Constructor creates GTK widgets
	LightInspector();

	// Show this LightInspector dialog
	void toggle();
	
	// TransientWindow callbacks
	virtual void _preShow();
	virtual void _preHide();
	
	// Widget construction functions
	GtkWidget* createPointLightPanel();
	GtkWidget* createProjectedPanel();
	GtkWidget* createOptionsPanel();
	GtkWidget* createTextureWidgets();
	GtkWidget* createButtons();

	// GTK CALLBACKS
	static void _onProjToggle(GtkWidget*, LightInspector*);	
	static void _onPointToggle(GtkWidget*, LightInspector*);	
	static void _onOK(GtkWidget*, LightInspector*);
	static void _onColourChange(GtkColorButton* widget, LightInspector* self); 
	static void _onOptionsToggle(GtkToggleButton* togglebutton, LightInspector *self);

	// Update the dialog widgets from keyvals on the first selected entity
	void getValuesFromEntity();
	
	// Write the widget contents to the given entity
	void setValuesOnEntity(Entity* entity);

    // Write contents to all light entities
    void writeToAllEntities();

    // Set the given key/value pair on ALL entities in the list of lights
    void setKeyValueAllLights(const std::string& k, const std::string& v);
	
	/** greebo: Gets called when the shader selection gets changed, so that
	 * 			the displayed texture info can be updated.
	 */
	void shaderSelectionChanged(const std::string& shader, GtkListStore* listStore);
	
public:

	// Gets called by the SelectionSystem when the selection is changed
	void selectionChanged(const scene::INodePtr& node, bool isComponent);

	/** Toggle the visibility of the dialog instance, constructing it if necessary.
	 * 
	 * Note: This is declared static to make it usable as a target for FreeCaller<> stuff. 
	 */
	static void toggleInspector(const cmd::ArgumentList& args);
	
	/** greebo: This is the actual home of the static instance 
	 */
	static LightInspector& Instance();
	
	// Update the sensitivity of the widgets
	void update();

	// UndoSystem::Observer implementation
	void postUndo();
	void postRedo();
	
	// Safely disconnects this dialog from all the systems
	// and saves the window size/position to the registry
	virtual void onRadiantShutdown(); 
};

}

#endif /*LIGHTINSPECTOR_H_*/
