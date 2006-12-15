#ifndef LIGHTINSPECTOR_H_
#define LIGHTINSPECTOR_H_

#include "ui/common/LightTextureSelector.h"

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

class LightInspector
{
	// Main dialog widget
	GtkWidget* _widget;
	
	// Projected light flag
	bool _isProjected;

	// Light type toggle buttons
	GtkWidget* _pointLightToggle;
	GtkWidget* _projLightToggle;
	
	// Colour selection widget
	GtkWidget* _colour;
	
	// Texture selection combo
	ui::LightTextureSelector _texSelector;

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
	
private:

	// Constructor creates GTK widgets
	LightInspector();

	// Show this LightInspector dialog
	void show();

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
	static void _onCancel(GtkWidget*, LightInspector*);

	// Update the dialog widgets from keyvals on the entity
	void getValuesFromEntity();
	
	// Write the widget contents to the entity
	void setValuesOnEntity();
	
public:

	/** Display the singleton dialog instance, constructing it if necessary.
	 */
	static void displayDialog();

};

}

#endif /*LIGHTINSPECTOR_H_*/
