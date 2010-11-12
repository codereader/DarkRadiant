#ifndef SOUNDCHOOSER_H_
#define SOUNDCHOOSER_H_

#include "gtkutil/window/BlockingTransientWindow.h"

#include "ui/common/SoundShaderPreview.h"
#include <string>
#include <gtkmm/treestore.h>
#include <gtkmm/treeselection.h>

namespace Gtk
{
	class TreeView;
}

namespace ui
{

/**
 * Dialog for listing and selection of sound shaders.
 */
class SoundChooser :
	public gtkutil::BlockingTransientWindow
{
public:
	// Treemodel definition
	struct TreeColumns :
		public Gtk::TreeModel::ColumnRecord
	{
		TreeColumns()
		{
			add(displayName);
			add(shaderName);
			add(isFolder);
			add(icon);
		}

		Gtk::TreeModelColumn<std::string> displayName;
		Gtk::TreeModelColumn<std::string> shaderName;
		Gtk::TreeModelColumn<bool> isFolder;
		Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf> > icon;
	};

private:
	TreeColumns _columns;

	// Tree store for shaders, and the tree selection
	Glib::RefPtr<Gtk::TreeStore> _treeStore;
	Gtk::TreeView* _treeView;
	Glib::RefPtr<Gtk::TreeSelection> _treeSelection;

	// The preview widget group
	SoundShaderPreview* _preview;

	// Last selected shader
	std::string _selectedShader;

private:

	// Widget construction
	Gtk::Widget& createTreeView();
	Gtk::Widget& createButtons();

	// gtkmm callbacks
	void _onOK();
	void _onCancel();
	void _onSelectionChange();

	// Implement custom action on window delete
	void _onDeleteEvent();

public:

	/**
	 * Constructor creates widgets.
	 */
	SoundChooser();

	// Retrieve the selected sound shader
	const std::string& getSelectedShader() const;

	// Set the selected sound shader, and focuses the treeview to the new selection
	void setSelectedShader(const std::string& shader);
};

} // namespace

#endif /*SOUNDCHOOSER_H_*/
