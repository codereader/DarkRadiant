#ifndef LIGHTINSPECTOR_H_
#define LIGHTINSPECTOR_H_

#include <gtk/gtkwidget.h>

/* FORWARD DECLS */

class Entity;

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
	
	// The light entity to edit
	Entity* _entity;
	
private:

	// Constructor creates GTK widgets
	LightInspector();

	// Show this LightInspector dialog
	void show();

	// Widget construction functions
	GtkWidget* createPointLightPanel();
	GtkWidget* createProjectedPanel();
	GtkWidget* createTextureWidgets();
	GtkWidget* createButtons();

	// GTK CALLBACKS
	static void _onProjToggle(GtkWidget*, LightInspector*);	
	static void _onPointToggle(GtkWidget*, LightInspector*);	
	static void _onOK(GtkWidget*, LightInspector*);
	static void _onCancel(GtkWidget*, LightInspector*);

public:

	/** Display the singleton dialog instance, constructing it if necessary.
	 */
	static void displayDialog();

};

}

#endif /*LIGHTINSPECTOR_H_*/
