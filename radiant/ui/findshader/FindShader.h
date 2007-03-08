#ifndef FINDSHADER_H_
#define FINDSHADER_H_

#include <string>
#include "gtk/gtkwidget.h"
#include "gtk/gtkeditable.h"
#include "gtk/gtkliststore.h"
#include "gtkutil/DialogWindow.h"

/* greebo: The dialog providing the Find & Replace shader functionality.
 * 
 * Note: Show the dialog by instantiating this class with NEW on the heap, 
 * as it's deriving from gtkutil::DialogWindow. It destroys itself upon dialog closure 
 * and frees the allocated memory. 
 */
namespace ui {

class FindAndReplaceShader :
	public gtkutil::DialogWindow
{
	// The entry fields
	GtkWidget* _findEntry;
	GtkWidget* _replaceEntry;
	
	// The buttons to select the shader
	GtkWidget* _findSelectButton;
	GtkWidget* _replaceSelectButton;
	
	// The checkbox "Search Selected Only"
	GtkWidget* _selectedOnly;
	
	// The counter "x shaders replaced."
	GtkWidget* _counterLabel;
	
	// The treeview containing the above liststore
	GtkWidget* _treeView;
	
public:
	// Constructor
	FindAndReplaceShader();
	~FindAndReplaceShader();
	
	// This is called to initialise the dialog window / create the widgets
	virtual void populateWindow();
	
	/** greebo: Shows the dialog (allocates on heap, dialog self-destructs)
	 */
	static void show();
	
private:

	/** greebo: As the name states, this runs the replace algorithm
	 */
	void performReplace();

	// Helper method to create the OK/Cancel button
	GtkWidget* createButtons();
	
	// The callback for the buttons
	static void onReplace(GtkWidget* widget, FindAndReplaceShader* self);
	static void onClose(GtkWidget* widget, FindAndReplaceShader* self);
	
	static void onChooseFind(GtkWidget* widget, FindAndReplaceShader* self);
	static void onChooseReplace(GtkWidget* widget, FindAndReplaceShader* self);
	
	static void onFindChanged(GtkEditable* editable, FindAndReplaceShader* self);
	static void onReplaceChanged(GtkEditable* editable, FindAndReplaceShader* self);
}; // class FindAndReplaceShader

} // namespace ui

#endif /*FINDSHADER_H_*/
