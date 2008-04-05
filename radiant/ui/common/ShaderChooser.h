#ifndef SHADERCHOOSER_H_
#define SHADERCHOOSER_H_

#include "ui/common/ShaderSelector.h"
#include "gtkutil/WindowPosition.h"
#include "gtkutil/window/BlockingTransientWindow.h"
#include <string>
#include <gtk/gtkwidget.h>

// Forward decls
class IShader;

namespace ui {

/* A GTK dialog containing a ShaderSelector widget combo and OK/Cancel
 * buttons. The ShaderSelector subclass is automatically populated with
 * all shaders matching the "texture/" prefix.
 * 
 * Use the LightShaderChooser class if you need an implementation to choose
 * light shaders only.
 */
class ShaderChooser : 
	public gtkutil::BlockingTransientWindow,
	public ShaderSelector::Client
{
public:
	// Derive from this class to get notified upon shader changes.
	class ChooserClient
	{
	public:
		// greebo: This gets invoked upon selection changed to allow the client to react.
		virtual void shaderSelectionChanged(const std::string& shader) = 0;
	};
	
private:
	// The "parent" class that gets notified upon shaderchange
	ChooserClient* _client;

	// The widget this dialog is transient for.
	GtkWindow* _parent;
	
	// The text entry the chosen texture is written into (can be NULL)
	GtkWidget* _targetEntry;
	
	// The ShaderSelector widget, that contains the actual selection
	// tools (treeview etc.)
	ShaderSelector _selector;
	
	// The shader name at dialog startup (to allow proper behaviour on cancelling)
	std::string _initialShader;
	
	// The window position tracker
	gtkutil::WindowPosition _windowPosition;
	
public:
	/** greebo: Construct the dialog window and its contents.
	 * 
	 * @parent: The widget this dialog is transient for.
	 * @targetEntry: The text entry where the selected shader can be written to.
	 *               Also, the initially selected shader will be read from 
	 *               this field at startup.
	 */
	ShaderChooser(ChooserClient* client, GtkWindow* parent, GtkWidget* targetEntry = NULL);
	
	/** greebo: Gets called upon shader selection change
	 */
	void shaderSelectionChanged(const std::string& shaderName, GtkListStore* listStore);
	
private:
	// Saves the window position
	void shutdown();

	// Reverts the connected entry field to the value it had before 
	void revertShader();

	// Widget construction helpers
	GtkWidget* createButtons();
	
	/* GTK CALLBACKS */
	static void callbackCancel(GtkWidget*, ShaderChooser*);
	static void callbackOK(GtkWidget*, ShaderChooser*);
	
	// The keypress handler for catching the Enter key when in the shader entry field
	static gboolean onKeyPress(GtkWidget* widget, GdkEventKey* event, ShaderChooser* self);
};

} // namespace ui

#endif /*SHADERCHOOSER_H_*/
