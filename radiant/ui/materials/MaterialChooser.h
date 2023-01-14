#pragma once

#include "wxutil/decl/DeclarationSelectorDialog.h"
#include <sigc++/signal.h>

#include "MaterialSelector.h"

// Forward decls
class Material;

namespace ui
{

/* A dialog containing a MaterialSelector widget combo and OK/Cancel
 * buttons. The MaterialSelector subclass is automatically populated with
 * all shaders matching the "texture/" prefix.
 */
class MaterialChooser :
	public wxutil::DeclarationSelectorDialog
{
	// The text entry the chosen texture is written into (can be NULL)
	wxTextCtrl* _targetEntry;

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
	MaterialChooser(wxWindow* parent, MaterialSelector::TextureFilter filter, wxTextCtrl* targetEntry = nullptr);

    int ShowModal() override;

    /// Signal emitted when selected shader is changed
    sigc::signal<void>& signal_shaderChanged();

private:
	// greebo: Gets called upon shader selection change.
	void shaderSelectionChanged();
};

} // namespace ui
