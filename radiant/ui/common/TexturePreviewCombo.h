#ifndef TEXTUREPREVIEWCOMBO_H_
#define TEXTUREPREVIEWCOMBO_H_

#include <string>

#include "gtkutil/menu/PopupMenu.h"

#include "gtkutil/GLWidget.h"
#include <gtk/gtkwidget.h>
#include <gtk/gtkliststore.h>

namespace ui
{
	
/** An HBox containing an OpenGL preview widget displaying a texture, and
 * a List View showing information about that texture.
 */

class TexturePreviewCombo
{
	// Main container widget
	GtkWidget* _widget;
	
	// The OpenGL preview widget
	gtkutil::GLWidget _glWidget;
	
	// The texture to preview
	std::string _texName;
	
	// Info table list store and view
	GtkListStore* _infoStore;
	GtkWidget* _infoView;
	
	// Context menu
	gtkutil::PopupMenu _contextMenu;
	
private:

	/* gtkutil::PopupMenu callbacks */
	void _onCopyTexName();
	
	/* GTK CALLBACKS */
	static void  _onExpose(GtkWidget*, GdkEventExpose*, TexturePreviewCombo*);
	
	// Refresh info table utility function
	void refreshInfoTable();
	
public:

	/** Constructor creates GTK widgets.
	 */
	TexturePreviewCombo();
	
	/** Set the texture to preview.
	 * 
	 * @param tex
	 * String name of the texture to preview (e.g. "textures/common/caulk")
	 */
	void setTexture(const std::string& tex);
	
	/** Operator cast to GtkWidget* for packing into parent container.
	 */
	operator GtkWidget* () {
		return _widget;
	}
};

}

#endif /*TEXTUREPREVIEWCOMBO_H_*/
