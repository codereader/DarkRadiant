#ifndef TEXTUREPREVIEWCOMBO_H_
#define TEXTUREPREVIEWCOMBO_H_

#include <string>

#include "gtkutil/menu/PopupMenu.h"
#include "gtkutil/GLWidget.h"

#include <gtkmm/box.h>
#include <gtkmm/liststore.h>

#include <boost/shared_ptr.hpp>

namespace Gtk
{
	class TreeView;
}

namespace ui
{
	
/** An HBox containing an OpenGL preview widget displaying a texture, and
 * a List View showing information about that texture.
 */

class TexturePreviewCombo :
	public Gtk::HBox
{
private:
	struct InfoStoreColumns : 
		public Gtk::TreeModel::ColumnRecord
	{
		InfoStoreColumns() { add(attribute); add(value); }

		Gtk::TreeModelColumn<Glib::ustring> attribute;
		Gtk::TreeModelColumn<Glib::ustring> value;
	};

	InfoStoreColumns _infoStoreColumns;

	// The OpenGL preview widget
	gtkutil::GLWidget* _glWidget;
	
	// The texture to preview
	std::string _texName;
	
	// Info table list store and view
	Glib::RefPtr<Gtk::ListStore> _infoStore;
	Gtk::TreeView* _infoView;
	
	// Context menu
	gtkutil::PopupMenu _contextMenu;
	
public:

	/** Constructor creates GTK widgets.
	 */
	TexturePreviewCombo();

	~TexturePreviewCombo();
	
	/** Set the texture to preview.
	 * 
	 * @param tex
	 * String name of the texture to preview (e.g. "textures/common/caulk")
	 */
	void setTexture(const std::string& tex);

private:
	/* gtkutil::PopupMenu callbacks */
	void _onCopyTexName();
	
	// gtkmm callback
	bool _onExpose(GdkEventExpose*);
	
	// Refresh info table utility function
	void refreshInfoTable();
};
typedef boost::shared_ptr<TexturePreviewCombo> TexturePreviewComboPtr;

} // namespace

#endif /*TEXTUREPREVIEWCOMBO_H_*/
