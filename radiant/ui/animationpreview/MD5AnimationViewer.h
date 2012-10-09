#pragma once

#include "gtkutil/window/BlockingTransientWindow.h"
#include "imd5model.h"
#include "ieclass.h"

#include "AnimationPreview.h"
#include "icommandsystem.h"
#include "gtkutil/VFSTreePopulator.h"

#include <gtkmm/liststore.h>
#include <gtkmm/treestore.h>
#include <gtkmm/treeselection.h>

namespace ui
{

class MD5AnimationViewer :
	public gtkutil::BlockingTransientWindow,
	public ModelDefVisitor,
	public gtkutil::VFSTreePopulator::Visitor
{
public:
	// Treemodel definitions
	struct ModelListColumns :
		public Gtk::TreeModel::ColumnRecord
	{
		ModelListColumns() { add(name); }

		Gtk::TreeModelColumn<std::string> name;
	};

	struct AnimListColumns :
		public Gtk::TreeModel::ColumnRecord
	{
		AnimListColumns() { add(name); add(filename); }

		Gtk::TreeModelColumn<std::string> name;
		Gtk::TreeModelColumn<std::string> filename;
	};

private:
	ModelListColumns _modelColumns;

	// Liststore for the model list, and its selection object
	Glib::RefPtr<Gtk::TreeStore> _modelList;
	Glib::RefPtr<Gtk::TreeSelection> _modelSelection;

	Gtk::TreeView* _modelTreeView;

	gtkutil::VFSTreePopulator _modelPopulator;

	AnimListColumns _animColumns;

	// Liststore for the anim list, and its selection object
	Glib::RefPtr<Gtk::ListStore> _animList;
	Glib::RefPtr<Gtk::TreeSelection> _animSelection;

	Gtk::TreeView* _animTreeView;

	// Animation preview widget
	AnimationPreviewPtr _preview;

public:
	MD5AnimationViewer();

	static void Show(const cmd::ArgumentList& args);

	void visit(const IModelDefPtr& modelDef);

	void visit(const Glib::RefPtr<Gtk::TreeStore>& store,
				const Gtk::TreeModel::iterator& iter,
				const std::string& path,
				bool isExplicit);

protected:
	// Override BlockingTransientWindow::_postShow()
	void _postShow();

private:
	// gtkmm callbacks
	void _onOK();
	void _onCancel();
	void _onModelSelChanged();
	void _onAnimSelChanged();

	Gtk::Widget& createButtons();
	Gtk::Widget& createListPane();
	Gtk::Widget& createModelTreeView();
	Gtk::Widget& createAnimTreeView();

	// Populate with names
	void populateModelList();
	void populateAnimationList();

	IModelDefPtr getSelectedModel();
};

}
