#ifndef TEXTUREPREVIEWCOMBO_H_
#define TEXTUREPREVIEWCOMBO_H_

#include <string>

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
	GtkWidget* _glWidget;
	
	// The texture to preview
	std::string _texName;
	
	// Info table list store
	GtkListStore* _infoStore;
	
private:

	/* GTK CALLBACKS */
	
	static void  _onExpose(GtkWidget*, GdkEventExpose*, TexturePreviewCombo*);
	
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
