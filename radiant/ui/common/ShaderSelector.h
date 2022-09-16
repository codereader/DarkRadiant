#pragma once

#include <memory>
#include "DeclarationSelector.h"

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
 * This widget populates its list of shaders automatically, and offers a method
 * that allows calling code to retrieve the user's selection. The set of
 * displayed textures can be defined by passing the corresponding TextureFilter
 * value to the constructor.
 */
class ShaderSelector :
	public DeclarationSelector
{
public:
    enum class TextureFilter
    {
        Regular, // prefix: "textures/"
        Lights,  // prefix: light/, fog/
    };

private:
    TextureFilter _textureFilter;

	std::function<void()> _selectionChanged;

    TexturePreviewCombo* _previewCombo;

public:
	/** Constructor.
	 *
	 * @selectionChanged: Functor invoked when the tree view selection changes.
	 * @filter: which texture set to show in the selector
	 */
	ShaderSelector(wxWindow* parent, const std::function<void()>& selectionChanged, TextureFilter filter);

	// Get the selected Material
	MaterialPtr getSelectedShader();

protected:
    void onTreeViewSelectionChanged() override;
};

} // namespace ui
