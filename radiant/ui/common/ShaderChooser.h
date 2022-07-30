#pragma once

#include "ui/common/ShaderSelector.h"
#include "wxutil/WindowPosition.h"
#include "wxutil/dialog/DialogBase.h"
#include <string>
#include <sigc++/signal.h>

// Forward decls
class Material;

namespace ui
{

/* A dialog containing a ShaderSelector widget combo and OK/Cancel
 * buttons. The ShaderSelector subclass is automatically populated with
 * all shaders matching the "texture/" prefix.
 */
class ShaderChooser :
	public wxutil::DialogBase
{
	// The text entry the chosen texture is written into (can be NULL)
	wxTextCtrl* _targetEntry;

	// The ShaderSelector widget, that contains the actual selection
	// tools (treeview etc.)
	ShaderSelector* _selector;

	// The shader name at dialog startup (to allow proper behaviour on cancelling)
	std::string _initialShader;

	// The window position tracker
	wxutil::WindowPosition _windowPosition;

    sigc::signal<void> _shaderChangedSignal;

public:
	/** greebo: Construct the dialog window and its contents.
	 *
	 * @parent: The widget this dialog is transient for.
	 * @filter: Defines the texture set to show
	 * @targetEntry: The text entry where the selected shader can be written to.
	 *               Also, the initially selected shader will be read from
	 *               this field at startup.
	 */
	ShaderChooser(wxWindow* parent, ShaderSelector::TextureFilter filter, wxTextCtrl* targetEntry = nullptr);

    std::string getSelectedTexture();

    void setSelectedTexture(const std::string& textureName);

    /// Signal emitted when selected shader is changed
    sigc::signal<void> signal_shaderChanged() const
    {
        return _shaderChangedSignal;
    }

private:
	// greebo: Gets called upon shader selection change.
	void shaderSelectionChanged();

	// Saves the window position
	void shutdown();

	// Reverts the connected entry field to the value it had before
	void revertShader();

	// Widget construction helpers
	void createButtons(wxPanel* mainPanel, wxBoxSizer* dialogVBox);

	// button callbacks
	void callbackCancel(wxCommandEvent& ev);
	void callbackOK(wxCommandEvent& ev);
    void _onItemActivated( wxDataViewEvent& ev );
};

} // namespace ui
