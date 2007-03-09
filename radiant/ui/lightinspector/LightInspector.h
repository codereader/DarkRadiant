#ifndef LIGHTINSPECTOR_H_
#define LIGHTINSPECTOR_H_

#include "iselection.h"
#include "ui/common/ShaderSelector.h"
#include "gtkutil/WindowPosition.h"

#include <gtk/gtkwidget.h>
#include <map>
#include <string>

/* FORWARD DECLS */

class Entity;
namespace gtkutil {
	class Vector3Entry;
}

namespace ui
{

/** Dialog to allow adjustment of properties on lights, including the conversion
 * between projected and point lights.
 */

class LightInspector :
	public SelectionSystem::Observer,
	public ShaderSelector::Client
{
	// Main dialog widget
	GtkWidget* _widget;
	
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
	Entity* _entity;
	
	// Table of original value keys, to avoid replacing them with defaults
	typedef std::map<std::string, std::string> StringMap;
	StringMap _valueMap;
	
	gtkutil::WindowPosition _windowPosition;
	
private:

	// Constructor creates GTK widgets
	LightInspector();

	// Show this LightInspector dialog
	void toggle();
	
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
	static gboolean onDelete(GtkWidget* widget, GdkEvent* event, LightInspector* self);

	// Update the dialog widgets from keyvals on the entity
	void getValuesFromEntity();
	
	// Write the widget contents to the entity
	void setValuesOnEntity();
	
	/** greebo: Gets called when the shader selection gets changed, so that
	 * 			the displayed texture info can be updated.
	 */
	void shaderSelectionChanged(const std::string& shader, GtkListStore* listStore);
	
public:

	// Gets called by the SelectionSystem when the selection is changed
	void selectionChanged(scene::Instance& instance);

	/** Toggle the visibility of the dialog instance, constructing it if necessary.
	 * 
	 * Note: This is declared static to make it usable as a target for FreeCaller<> stuff. 
	 */
	static void toggleInspector();
	
	/** greebo: This is the actual home of the static instance 
	 */
	static LightInspector& Instance();
	
	// Update the sensitivity of the widgets
	void update();
	
	// Safely disconnects this dialog from all the systems
	// and saves the window size/position to the registry
	void shutdown(); 
};

}

#endif /*LIGHTINSPECTOR_H_*/
