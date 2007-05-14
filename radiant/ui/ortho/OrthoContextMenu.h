#ifndef ORTHOCONTEXTMENU_H_
#define ORTHOCONTEXTMENU_H_

#include <gtk/gtk.h>

#include "math/Vector3.h"

namespace ui
{

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
	GtkWidget* _revertWorldspawn;
	GtkWidget* _addEntity;
	GtkWidget* _addPlayerStart;
	GtkWidget* _addLight;
	GtkWidget* _addSpkr;
	
private:

	// Enable or disable the "convert to static" option based on the number
	// of selected brushes.
	void checkConvertStatic();
	
	/** greebo: Enable or disables the "revert to worldspawn" option 
	 */
	void checkRevertToWorldspawn();
	
	/** greebo: Disables the "entity/light/speaker" options according to the selection.
	 */
	void checkAddOptions();

	/* Gtk Callbacks */
	
	static void callbackAddEntity(GtkMenuItem* item, OrthoContextMenu* self);
	static void callbackAddPlayerStart(GtkMenuItem* item, OrthoContextMenu* self);
	static void callbackAddLight(GtkMenuItem* item, OrthoContextMenu* self);
	static void callbackAddModel(GtkMenuItem* item, OrthoContextMenu* self);
	static void callbackAddPrefab(GtkMenuItem* item, OrthoContextMenu* self);
	static void _onAddSpeaker(GtkMenuItem*, OrthoContextMenu*);
	
	static void callbackConvertToStatic(GtkMenuItem* item, OrthoContextMenu* self);
	static void callbackRevertToWorldspawn(GtkMenuItem* item, OrthoContextMenu* self);
	
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
