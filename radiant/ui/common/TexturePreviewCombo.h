#pragma once

#include <string>
#include "wxutil/menu/PopupMenu.h"
#include <wx/panel.h>

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
	public wxPanel
{
	// The OpenGL preview widget
	wxutil::GLWidget* _glWidget;

	// The texture to preview
	std::string _texName;

    // Info table
    wxutil::KeyValueTable* _infoTable;

	// Context menu
	wxutil::PopupMenuPtr _contextMenu;

public:

	/** Constructor creates widgets.
	 */
	TexturePreviewCombo(wxWindow* parent);

	/** Set the texture to preview.
	 *
	 * @param tex
	 * String name of the texture to preview (e.g. "textures/common/caulk")
	 */
	void SetTexture(const std::string& tex);

private:
	/* gtkutil::PopupMenu callbacks */
	void _onCopyTexName();

	// render callback
	void _onRender();

	// Refresh info table utility function
	void refreshInfoTable();

	// Popupmenu event
	void _onContextMenu(wxDataViewEvent& ev);
};

} // namespace
