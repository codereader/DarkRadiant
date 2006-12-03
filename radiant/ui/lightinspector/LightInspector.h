#ifndef LIGHTINSPECTOR_H_
#define LIGHTINSPECTOR_H_

#include <gtk/gtkwidget.h>

namespace ui
{

/** Dialog to allow adjustment of properties on lights, including the conversion
 * between projected and point lights.
 */

class LightInspector
{
	// Main dialog widget
	GtkWidget* _widget;
	
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
	

public:

	/** Display the singleton dialog instance, constructing it if necessary.
	 */
	static void displayDialog();

};

}

#endif /*LIGHTINSPECTOR_H_*/
