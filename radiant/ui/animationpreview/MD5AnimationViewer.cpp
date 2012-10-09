#include "MD5AnimationViewer.h"

#include "i18n.h"
#include "imainframe.h"
#include "imodelcache.h"
#include "imd5anim.h"

#include "gtkutil/MultiMonitor.h"
#include "gtkutil/LeftAlignment.h"
#include "gtkutil/RightAlignment.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/LeftAlignedLabel.h"
#include "gtkutil/VFSTreePopulator.h"

#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/stock.h>
#include <gtkmm/treeview.h>

namespace ui
{

MD5AnimationViewer::MD5AnimationViewer() :
	gtkutil::BlockingTransientWindow(_("MD5 Animation Viewer"), GlobalMainFrame().getTopLevelWindow()),
	_modelList(Gtk::TreeStore::create(_modelColumns)),
	_modelPopulator(_modelList),
	_animList(Gtk::ListStore::create(_animColumns)),
	_preview(new AnimationPreview)
{
	// Set the default border width in accordance to the HIG
	set_border_width(12);
	set_type_hint(Gdk::WINDOW_TYPE_HINT_DIALOG);

	// Set the default size of the window
	const Glib::RefPtr<Gtk::Window>& mainWindow = GlobalMainFrame().getTopLevelWindow();

	Gdk::Rectangle rect = gtkutil::MultiMonitor::getMonitorForWindow(mainWindow);
	int height = static_cast<int>(rect.get_height() * 0.6f);

	set_default_size(
		static_cast<int>(rect.get_width() * 0.8f), height
	);

	// Set the default size of the window
	_preview->setSize(rect.get_width() * 0.2f, height);

	// Main dialog vbox
	Gtk::VBox* vbox = Gtk::manage(new Gtk::VBox(false, 12));

	// Create a horizontal box to pack the treeview on the left and the preview on the right
	Gtk::HBox* hbox = Gtk::manage(new Gtk::HBox(false, 6));

	hbox->pack_start(createListPane(), true, true, 0);

	Gtk::VBox* previewBox = Gtk::manage(new Gtk::VBox(false, 0));
	previewBox->pack_start(*_preview, true, true, 0);

	hbox->pack_start(*previewBox, true, true, 0);

	vbox->pack_start(*hbox, true, true, 0);
	vbox->pack_end(createButtons(), false, false, 0);

	// Add main vbox to dialog
	add(*vbox);

	// Populate with model names
	populateModelList();
}

void MD5AnimationViewer::Show(const cmd::ArgumentList& args)
{
	MD5AnimationViewer viewer;
	viewer.show();
}

void MD5AnimationViewer::_postShow()
{
	// Initialise the GL widget after the widgets have been shown
	_preview->initialisePreview();

	// Call the base class, will enter main loop
	BlockingTransientWindow::_postShow();
}

void MD5AnimationViewer::_onOK()
{
	destroy();
}

Gtk::Widget& MD5AnimationViewer::createButtons()
{
	Gtk::HBox* hbx = Gtk::manage(new Gtk::HBox(true, 6));

	Gtk::Button* closeButton = Gtk::manage(new Gtk::Button(Gtk::Stock::CLOSE));

	closeButton->signal_clicked().connect(sigc::mem_fun(*this, &MD5AnimationViewer::_onOK));

	hbx->pack_end(*closeButton, true, true, 0);

	return *Gtk::manage(new gtkutil::RightAlignment(*hbx));
}

Gtk::Widget& MD5AnimationViewer::createListPane()
{
	Gtk::VBox* vbx = Gtk::manage(new Gtk::VBox(false, 6));

	Gtk::Label* modelLabel = Gtk::manage(
		new gtkutil::LeftAlignedLabel(std::string("<b>") + _("Model Definition") + "</b>"));

	vbx->pack_start(*modelLabel, false, false, 0);
	vbx->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(createModelTreeView(), 18, 1)), true, true, 0);

	Gtk::Label* animLabel = Gtk::manage(
		new gtkutil::LeftAlignedLabel(std::string("<b>") + _("Available Animations") + "</b>"));

	vbx->pack_start(*animLabel, false, false, 0);
	vbx->pack_start(*Gtk::manage(new gtkutil::LeftAlignment(createAnimTreeView(), 18, 1)), true, true, 0);

	return *vbx;
}

Gtk::Widget& MD5AnimationViewer::createModelTreeView()
{
	_modelTreeView = Gtk::manage(new Gtk::TreeView(_modelList));
	_modelTreeView->set_headers_visible(false);
	_modelTreeView->set_size_request(300, -1);

	// Single text column
	_modelTreeView->append_column(*Gtk::manage(new gtkutil::TextColumn(_("Model Definition"), _modelColumns.name, false)));

	// Apply full-text search to the column
	_modelTreeView->set_search_equal_func(sigc::ptr_fun(gtkutil::TreeModel::equalFuncStringContains));

	// Connect up the selection changed callback
	_modelSelection = _modelTreeView->get_selection();
	_modelSelection->signal_changed().connect(sigc::mem_fun(*this, &MD5AnimationViewer::_onModelSelChanged));

	// Pack into scrolled window and return
	return *Gtk::manage(new gtkutil::ScrolledFrame(*_modelTreeView));
}

Gtk::Widget& MD5AnimationViewer::createAnimTreeView()
{
	_animTreeView = Gtk::manage(new Gtk::TreeView(_animList));
	_animTreeView->set_headers_visible(false);
	_animTreeView->set_size_request(300, -1);

	// Single text column
	_animTreeView->append_column(*Gtk::manage(new gtkutil::TextColumn(_("Animation"), _animColumns.name, false)));
	_animTreeView->append_column(*Gtk::manage(new gtkutil::TextColumn(_("File"), _animColumns.filename, false)));

	// Apply full-text search to the column
	_animTreeView->set_search_equal_func(sigc::ptr_fun(gtkutil::TreeModel::equalFuncStringContains));

	// Connect up the selection changed callback
	_animSelection = _animTreeView->get_selection();
	_animSelection->signal_changed().connect(sigc::mem_fun(*this, &MD5AnimationViewer::_onAnimSelChanged));

	// Pack into scrolled window and return
	return *Gtk::manage(new gtkutil::ScrolledFrame(*_animTreeView));
}

void MD5AnimationViewer::_onModelSelChanged()
{
	IModelDefPtr modelDef = getSelectedModel();

	if (!modelDef)
	{
		_animTreeView->set_sensitive(false);
		return;
	}

	_animTreeView->set_sensitive(true);

	scene::INodePtr modelNode =  GlobalModelCache().getModelNode(modelDef->mesh);
	_preview->setAnim(md5::IMD5AnimPtr());
	_preview->setModelNode(modelNode);
	
	populateAnimationList();
}

IModelDefPtr MD5AnimationViewer::getSelectedModel()
{
	if (!_modelSelection->get_selected())
	{
		return IModelDefPtr();
	}

	Gtk::TreeModel::Row row = *_modelSelection->get_selected();

	return GlobalEntityClassManager().findModel(row[_modelColumns.name]);
}

void MD5AnimationViewer::_onAnimSelChanged()
{
	IModelDefPtr modelDef = getSelectedModel();

	if (!modelDef) 
	{
		_preview->setAnim(md5::IMD5AnimPtr());
		return;
	}

	Gtk::TreeModel::iterator iter = _animSelection->get_selected();

	if (!iter)
	{
		_preview->setAnim(md5::IMD5AnimPtr());
		return;
	}

	std::string filename = (*iter)[_animColumns.filename];

	// Assign preview animation
	md5::IMD5AnimPtr anim = GlobalAnimationCache().getAnim(filename);
	_preview->setAnim(anim);
}

void MD5AnimationViewer::visit(const IModelDefPtr& modelDef)
{
	_modelPopulator.addPath(modelDef->getModName() + "/" + modelDef->name);
}

void MD5AnimationViewer::visit(const Glib::RefPtr<Gtk::TreeStore>& store,
	const Gtk::TreeModel::iterator& iter, const std::string& path, bool isExplicit)
{
	// Get the display path, everything after rightmost slash
	Gtk::TreeModel::Row row = *iter;

	row[_modelColumns.name] = path.substr(path.rfind("/") + 1);
}

void MD5AnimationViewer::populateModelList()
{
	_modelList->clear();

	GlobalEntityClassManager().forEachModelDef(*this);

	_modelPopulator.forEachNode(*this);
}

void MD5AnimationViewer::populateAnimationList()
{
	_animList->clear();

	IModelDefPtr modelDef = getSelectedModel();

	if (!modelDef) return;

	for (IModelDef::Anims::const_iterator i = modelDef->anims.begin(); i != modelDef->anims.end(); ++i)
	{
		Gtk::TreeModel::iterator iter = _animList->append();
		Gtk::TreeModel::Row row = *iter;

		row[_animColumns.name] = i->first;		// anim name
		row[_animColumns.filename] = i->second;	// anim filename
	}
}

} // namespace
