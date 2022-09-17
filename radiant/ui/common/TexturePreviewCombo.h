#pragma once

#include <string>
#include "wxutil/menu/PopupMenu.h"
#include <wx/panel.h>

#include "ui/ideclpreview.h"

namespace wxutil
{ 
	class KeyValueTable;
	class GLWidget;
}

class wxDataViewEvent;

namespace ui
{

/** 
 * An HBox containing an OpenGL preview widget displaying a texture, and
 * a List View showing information about that texture.
 */
class TexturePreviewCombo :
	public wxPanel,
    public IDeclarationPreview
{
	// The OpenGL preview widget
	wxutil::GLWidget* _glWidget;

	// The texture to preview
	std::string _texName;

    // Info table
    wxutil::KeyValueTable* _infoTable;

	// Context menu
	wxutil::PopupMenuPtr _contextMenu;

    std::vector<std::string> _lightTexturePrefixes;

public:

	/** Constructor creates widgets.
	 */
	TexturePreviewCombo(wxWindow* parent);

    wxWindow* GetPreviewWidget() override
    {
        return this;
    }

    void ClearPreview() override;

	/** Set the texture to preview.
	 *
	 * @param declName
	 * String name of the texture to preview (e.g. "textures/common/caulk")
	 */
    void SetPreviewDeclName(const std::string& declName) override;

private:
	/* gtkutil::PopupMenu callbacks */
	void _onCopyTexName();

	// render callback
	bool _onRender();

	// Refresh info table utility function
	void refreshInfoTable();

	// Popupmenu event
	void _onContextMenu(wxDataViewEvent& ev);

    void loadLightTexturePrefixes();

    bool isLightTexture();
};

} // namespace
