#ifndef SHADERCHOOSER_H_
#define SHADERCHOOSER_H_

#include "ui/common/ShaderSelector.h"
#include <string>

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
	public ShaderSelector::Client
{
public:
	// Derive from this class to get notified upon shader changes.
	class Client
	{
	public:
		// greebo: This gets invoked upon selection changed to allow the client to react.
		virtual void shaderSelectionChanged(const std::string& shader) = 0;
	};
	
private:
	// The "parent" class that gets notified upon shaderchange
	Client* _client;

	// The widget this dialog is transient for.
	GtkWidget* _parent;
	
	// The text entry the chosen texture is written into (can be NULL)
	GtkWidget* _targetEntry;
	
	// Main dialog widget
	GtkWidget* _dialog;
	
	// The ShaderSelector widget, that contains the actual selection
	// tools (treeview etc.)
	ShaderSelector _selector;
	
	// The shader name at dialog startup (to allow proper behaviour on cancelling)
	std::string _initialShader;
	
public:
	/** greebo: Construct the dialog window and its contents.
	 * 
	 * @parent: The widget this dialog is transient for.
	 */
	ShaderChooser(Client* client, GtkWidget* parent, GtkWidget* targetEntry = NULL);
	
	/** greebo: Gets called upon shader selection change
	 */
	void shaderSelectionChanged(const std::string& shaderName, GtkListStore* listStore);
	
	// Constructor, delete widgets
	~ShaderChooser();
	
private:
	// Widget construction helpers
	GtkWidget* createButtons();
	
	/* GTK CALLBACKS */
	static void callbackCancel(GtkWidget*, ShaderChooser*);
	static void callbackOK(GtkWidget*, ShaderChooser*);
};

} // namespace ui

#endif /*SHADERCHOOSER_H_*/
