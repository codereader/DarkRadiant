#pragma once

#include <string>
#include <vector>
#include <memory>
#include "wxutil/dataview/DeclarationTreeView.h"

#include <wx/panel.h>

#include "TexturePreviewCombo.h"

// FORWARD DECLS
class Material;
typedef std::shared_ptr<Material> MaterialPtr;

class wxDataViewCtrl;

namespace ui
{

/**
 * A widget that allows the selection of a shader. The widget contains
 * three elements - a tree view displaying available shaders as
 * identified by the specified prefixes, a TexturePreviewComob displaying a
 * preview of the currently-selected shader and a table containing certain
 * information about the shader.
 *
 * Use the wxWindow* operator to incorporate this class into a dialog window.
 *
 * This widget populates its list of shaders automatically, and offers a method
 * that allows calling code to retrieve the user's selection. The set of
 * displayed textures can be defined by passing a list of texture prefixes to
 * the constructor (comma-separated, e.g. "fog,light").
 */
class ShaderSelector :
	public wxPanel
{
private:
    wxutil::DeclarationTreeView::Columns _shaderTreeColumns;
	wxutil::DeclarationTreeView* _treeView;

	std::function<void()> _selectionChanged;

    TexturePreviewCombo* _previewCombo;

public:
	// This is where the prefixes are stored (needed to filter the possible shaders)
	typedef std::vector<std::string> PrefixList;
	PrefixList _prefixes;

	/** Constructor.
	 *
	 * @selectionChanged: Functor invoked when the tree view selection changes.
	 * @prefixes: A comma-separated list of shader prefixes.
	 */
	ShaderSelector(wxWindow* parent, const std::function<void()>& selectionChanged,
		const std::string& prefixes);

	/** Return the shader selected by the user, or an empty string if there
	 * was no selection.
	 */
	std::string getSelection();

	/** Set the given shader name as the current selection, highlighting it
	 * in the tree view.
	 *
	 * @param selection
	 * The fullname of the shader to select, or the empty string if there
	 * should be no selection.
	 */
	void setSelection(const std::string& selection);

	// Get the selected Material
	MaterialPtr getSelectedShader();

private:
	// Create GUI elements
	void createTreeView();
	void createPreview();

	void _onSelChange(wxDataViewEvent& ev);
};

} // namespace ui
