#ifndef LIGHTTEXTURECHOOSER_H_
#define LIGHTTEXTURECHOOSER_H_

#include "ui/common/ShaderSelector.h"
#include <gtk/gtk.h>
#include <string>

// Forward decls
class IShader;

namespace ui {

/* A GTK dialog containing a ShaderSelector widget combo and OK/Cancel
 * buttons.
 */
class LightTextureChooser :
	public ShaderSelector::Client
{
	// The text entry in the PropertyEditor to write the chosen texture
	// into
	GtkWidget* _entry;
	
	// Main dialog widget
	GtkWidget* _widget;	
	
	// The ShaderSelector widget group, that contains the actual selection
	// tools (treeview etc.)
	ShaderSelector _selector;
	
public:
	// Construct the dialog window and its contents.
	LightTextureChooser(GtkWidget* combo, const std::string& prefixes);
	
	// Constructor, delete widgets
	~LightTextureChooser() {
		gtk_widget_destroy(_widget);
	}
	
	/** greebo: Gets called upon selection change, updates the infostore
	 * 			of the contained ShaderSelector helper class accordingly.
	 */
	void shaderSelectionChanged(const std::string& shaderName, GtkListStore* listStore);

private:
	// Widget construction helpers
	GtkWidget* createButtons();
	
	/* GTK CALLBACKS */
	static void callbackCancel(GtkWidget*, LightTextureChooser*);
	static void callbackOK(GtkWidget*, LightTextureChooser*);
};

} // namespace ui

#endif /*LIGHTTEXTURECHOOSER_H_*/
