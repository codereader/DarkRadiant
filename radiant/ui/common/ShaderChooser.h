#ifndef SHADERCHOOSER_H_
#define SHADERCHOOSER_H_

#include "ui/common/ShaderSelector.h"
#include "gtkutil/WindowPosition.h"
#include "gtkutil/window/BlockingTransientWindow.h"
#include <string>

// Forward decls
class Material;

namespace Gtk
{
	class Entry;
}

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
	// The text entry the chosen texture is written into (can be NULL)
	Gtk::Entry* _targetEntry;

	// The ShaderSelector widget, that contains the actual selection
	// tools (treeview etc.)
	ShaderSelector* _selector;

	// The shader name at dialog startup (to allow proper behaviour on cancelling)
	std::string _initialShader;

	// The window position tracker
	gtkutil::WindowPosition _windowPosition;

    sigc::signal<void> _shaderChangedSignal;

public:
	/** greebo: Construct the dialog window and its contents.
	 *
	 * @parent: The widget this dialog is transient for.
	 * @targetEntry: The text entry where the selected shader can be written to.
	 *               Also, the initially selected shader will be read from
	 *               this field at startup.
	 */
	ShaderChooser(const Glib::RefPtr<Gtk::Window>& parent,
                  Gtk::Entry* targetEntry = NULL);

    /// Signal emitted when selected shader is changed
    sigc::signal<void> signal_shaderChanged() const
    {
        return _shaderChangedSignal;
    }

	/**
	 * greebo: ShaderSelector::Client implementation
	 * Gets called upon shader selection change.
	 */
	void shaderSelectionChanged(const std::string& shaderName, const Glib::RefPtr<Gtk::ListStore>& listStore);

private:
	// Saves the window position
	void shutdown();

	// Reverts the connected entry field to the value it had before
	void revertShader();

	// Widget construction helpers
	Gtk::Widget& createButtons();

	// gtkmm callbacks
	void callbackCancel();
	void callbackOK();

	// The keypress handler for catching the Enter key when in the shader entry field
	bool onKeyPress(GdkEventKey* ev);
};

} // namespace ui

#endif /*SHADERCHOOSER_H_*/
