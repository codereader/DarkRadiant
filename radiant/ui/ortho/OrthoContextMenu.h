#ifndef ORTHOCONTEXTMENU_H_
#define ORTHOCONTEXTMENU_H_

#include <gtk/gtk.h>

#include "math/Vector3.h"

namespace ui
{

	namespace {
	
		/* CONSTANTS */
		
		const char* LIGHT_CLASSNAME = "light";
		const char* MODEL_CLASSNAME = "func_static";
	
		const char* ADD_MODEL_TEXT = "Create model...";
		const char* ADD_MODEL_ICON = "cmenu_add_model.png";
		const char* ADD_LIGHT_TEXT = "Create light...";
		const char* ADD_LIGHT_ICON = "cmenu_add_light.png";
		const char* ADD_ENTITY_TEXT = "Create entity...";
		const char* ADD_ENTITY_ICON = "cmenu_add_entity.png";
		const char* ADD_PREFAB_TEXT = "Insert prefab...";
		const char* ADD_PREFAB_ICON = "cmenu_add_prefab.png";
		
		const char* CONVERT_TO_STATIC_TEXT = "Convert brushes to static";
		const char* CONVERT_TO_STATIC_ICON = "cmenu_convert_static.png";
		
	}

/** Displays a menu when the mouse is right-clicked in the ortho window.
 * This is a singleton class which remains in existence once constructed,
 * and is hidden and displayed as appropriate.
 */

class OrthoContextMenu
{
private:
	
	// The GtkWidget representing the menu
	GtkWidget* _widget;
	
	// Last provided 3D point for action
	Vector3 _lastPoint;
	
	// GTK MENU ITEMS
	GtkWidget* _convertStatic;
	
private:

	// Enable or disable the "convert to static" option based on the number
	// of selected brushes.
	void checkConvertStatic();

	/* Gtk Callbacks */
	
	static void callbackAddEntity(GtkMenuItem* item, OrthoContextMenu* self);
	static void callbackAddLight(GtkMenuItem* item, OrthoContextMenu* self);
	static void callbackAddModel(GtkMenuItem* item, OrthoContextMenu* self);
	
	static void callbackConvertToStatic(GtkMenuItem* item, OrthoContextMenu* self);
	
public:

	/** Constructor. Create the GTK content here.
	 */

	OrthoContextMenu();

	/** Display the menu at the current mouse position, and act on the
	 * choice.
	 * 
	 * @param point
	 * The point in 3D space at which the chosen operation should take
	 * place.
	 */
	 
	void show(const Vector3& point);
	
	/** Static instance display function. Obtain the singleton instance and
	 * call its show() function.
	 * 
	 * @param point
	 * The point in 3D space at which the chosen operation should take
	 * place.
	 */
	 
	static void displayInstance(const Vector3& point);

};

}

#endif /*ORTHOCONTEXTMENU_H_*/
