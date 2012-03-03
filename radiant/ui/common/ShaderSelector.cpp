#include "ShaderSelector.h"

#include "i18n.h"
#include "iuimanager.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/IconTextColumn.h"
#include "gtkutil/VFSTreePopulator.h"
#include "gtkutil/GLWidgetSentry.h"
#include "texturelib.h"
#include "string/string.h"
#include "ishaders.h"
#include "iregistry.h"

#include <GL/glew.h>
#include <gtkmm/treeview.h>
#include <gtkmm/treestore.h>
#include <vector>
#include <string>
#include <map>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/bind.hpp>

namespace ui
{

/* CONSTANTS */

namespace
{
	const char* const FOLDER_ICON = "folder16.png";
	const char* const TEXTURE_ICON = "icon_texture.png";
}

// Constructor creates GTK elements
ShaderSelector::ShaderSelector(Client* client, const std::string& prefixes, bool isLightTexture) :
	Gtk::VBox(false, 3),
	_glWidget(Gtk::manage(new gtkutil::GLWidget(true, "ShaderSelector"))),
	_client(client),
	_isLightTexture(isLightTexture),
	_infoStore(Gtk::ListStore::create(_infoStoreColumns))
{
	// Split the given comma-separated list into the vector
	boost::algorithm::split(_prefixes, prefixes, boost::algorithm::is_any_of(","));

	// Construct main VBox, and pack in TreeView and info panel
	pack_start(createTreeView(), true, true, 0);
	pack_start(createPreview(), false, false, 0);
}

// Return the selection to the calling code
std::string ShaderSelector::getSelection()
{
	Gtk::TreeModel::iterator iter = _selection->get_selected();

	if (iter)
	{
		Gtk::TreeModel::Row row = *iter;
		return row[_shaderTreeColumns.shaderName];
	}
	else
	{
		// Nothing selected, return empty string
		return "";
	}
}

// Set the selection in the treeview
void ShaderSelector::setSelection(const std::string& sel)
{
	// If the selection string is empty, collapse the treeview and return with
	// no selection
	if (sel.empty())
	{
		_treeView->collapse_all();
		return;
	}

	// Use the gtkutil TreeModel algorithms to select the shader
	gtkutil::TreeModel::findAndSelectString(
		_treeView, boost::algorithm::to_lower_copy(sel), _shaderTreeColumns.shaderName);
}

// Local functor to populate the tree view with shader names

namespace {

	// VFSPopulatorVisitor to fill in column data for the populator tree nodes
	class DataInserter :
		public gtkutil::VFSTreePopulator::Visitor
	{
	private:
		const ShaderSelector::ShaderTreeColumns& _columns;

	public:
		DataInserter(const ShaderSelector::ShaderTreeColumns& columns) :
			_columns(columns)
		{}

		virtual ~DataInserter() {}

		// Required visit function
		void visit(const Glib::RefPtr<Gtk::TreeStore>& store,
				   const Gtk::TreeModel::iterator& iter,
				   const std::string& path,
				   bool isExplicit)
		{

			// Get the display name by stripping off everything before the last
			// slash
			std::string displayName = path.substr(path.rfind("/") + 1);

			// Pathname is the model VFS name for a model, and blank for a folder
			std::string fullPath = isExplicit ? path : "";

			// Fill in the column values
			Gtk::TreeModel::Row row = *iter;

			row[_columns.displayName] = displayName;
			row[_columns.shaderName] = fullPath;
			row[_columns.icon] = GlobalUIManager().getLocalPixbuf(isExplicit ? TEXTURE_ICON : FOLDER_ICON);
		}
	};

	class ShaderNameFunctor
	{
	public:
		// Interesting texture prefixes
		ShaderSelector::PrefixList& _prefixes;

		// The populator that gets called to add the parsed elements
		gtkutil::VFSTreePopulator& _populator;

		// Constructor
		ShaderNameFunctor(gtkutil::VFSTreePopulator& populator, ShaderSelector::PrefixList& prefixes) :
			_prefixes(prefixes),
			_populator(populator)
		{}

		void visit(const std::string& shaderName)
		{
			for (ShaderSelector::PrefixList::iterator i = _prefixes.begin();
				 i != _prefixes.end();
				 ++i)
			{
				// Only allow the prefixes passed in the constructor
				if (!shaderName.empty() && boost::algorithm::istarts_with(shaderName, (*i) + "/"))
				{
					_populator.addPath(boost::algorithm::to_lower_copy(shaderName));
					break; // don't consider any further prefixes
				}
			}
		}
	};
}

// Create the Tree View
Gtk::Widget& ShaderSelector::createTreeView()
{
	Glib::RefPtr<Gtk::TreeStore> treeStore = Gtk::TreeStore::create(_shaderTreeColumns);
	// Set the tree store to sort on this column
	treeStore->set_sort_column(_shaderTreeColumns.displayName, Gtk::SORT_ASCENDING);

	// Instantiate the helper class that populates the tree according to the paths
	gtkutil::VFSTreePopulator populator(treeStore);

	ShaderNameFunctor func(populator, _prefixes);
	GlobalMaterialManager().foreachShaderName(boost::bind(&ShaderNameFunctor::visit, &func, _1));

	// Now visit the created iterators to load the actual data into the tree
	DataInserter inserter(_shaderTreeColumns);
	populator.forEachNode(inserter);

	// Tree view
	_treeView = Gtk::manage(new Gtk::TreeView(treeStore));
	_treeView->set_headers_visible(false);

	// Single visible column, containing the directory/shader name and the icon
	Gtk::TreeView::Column* col = Gtk::manage(new gtkutil::IconTextColumn(
		_("Value"), _shaderTreeColumns.displayName, _shaderTreeColumns.icon)
	);
	// The name column should
	col->set_sort_column(_shaderTreeColumns.displayName);

	_treeView->append_column(*col);

	// Use the TreeModel's full string search function
	_treeView->set_search_equal_func(sigc::ptr_fun(gtkutil::TreeModel::equalFuncStringContains));

	// Get selection and connect the changed callback
	_selection = _treeView->get_selection();
	_selection->signal_changed().connect(sigc::mem_fun(*this, &ShaderSelector::_onSelChange));

	// Pack into scrolled window and frame
	return *Gtk::manage(new gtkutil::ScrolledFrame(*_treeView));
}

// Create the preview panel (GL widget and info table)
Gtk::Widget& ShaderSelector::createPreview()
{
	// HBox contains the preview GL widget along with a texture attributes
	// pane.
	Gtk::HBox* hbx = Gtk::manage(new Gtk::HBox(false, 3));

	// Cast the GLWidget object to GtkWidget
	_glWidget->set_size_request(128, 128);
	_glWidget->signal_expose_event().connect(sigc::mem_fun(*this, &ShaderSelector::_onExpose));

	Gtk::Frame* glFrame = Gtk::manage(new Gtk::Frame);
	glFrame->add(*_glWidget);

	hbx->pack_start(*glFrame, false, false, 0);

	// Attributes table
	Gtk::TreeView* tree = Gtk::manage(new Gtk::TreeView(_infoStore));
	tree->set_headers_visible(false);

	tree->append_column(*Gtk::manage(new gtkutil::TextColumn(_("Attribute"), _infoStoreColumns.attribute)));
	tree->append_column(*Gtk::manage(new gtkutil::TextColumn(_("Value"), _infoStoreColumns.value)));

	hbx->pack_start(*Gtk::manage(new gtkutil::ScrolledFrame(*tree)), true, true, 0);

	return *hbx;
}

// Get the selected shader
MaterialPtr ShaderSelector::getSelectedShader() {
	return GlobalMaterialManager().getMaterialForName(getSelection());
}

// Update the attributes table
void ShaderSelector::updateInfoTable()
{
	_infoStore->clear();

	// Get the selected texture name. If nothing is selected, we just leave the
	// infotable empty.
	std::string selName = getSelection();

	// Notify the client of the change to give it a chance to update the infostore
	if (_client != NULL && !selName.empty())
	{
		_client->shaderSelectionChanged(selName, _infoStore);
	}
}

// Callback to redraw the GL widget
bool ShaderSelector::_onExpose(GdkEventExpose* ev)
{
	// The scoped object making the GL widget the current one
	gtkutil::GLWidgetSentry sentry(*_glWidget);

	// Get the viewport size from the GL widget
	Gtk::Requisition req = _glWidget->size_request();
	glViewport(0, 0, req.width, req.height);

	// Initialise
	glClearColor(0.3f, 0.3f, 0.3f, 0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, req.width, 0, req.height, -100, 100);
	glEnable (GL_TEXTURE_2D);

	// Get the selected texture, and set up OpenGL to render it on
	// the quad.
	MaterialPtr shader = getSelectedShader();

	bool drawQuad = false;
	TexturePtr tex;

	// Check what part of the shader we should display in the preview
	if (_isLightTexture) {
		// This is a light, take the first layer texture
		const ShaderLayer* first = shader->firstLayer();
		if (first != NULL) {
			tex = shader->firstLayer()->getTexture();
			glBindTexture (GL_TEXTURE_2D, tex->getGLTexNum());
			drawQuad = true;
		}
	}
	else {
		// This is an "ordinary" texture, take the editor image
		tex = shader->getEditorImage();
		if (tex != NULL) {
			glBindTexture (GL_TEXTURE_2D, tex->getGLTexNum());
			drawQuad = true;
		}
	}

	if (drawQuad)
    {
		// Calculate the correct aspect ratio for preview.
      float aspect = float(tex->getWidth()) / float(tex->getHeight());

		float hfWidth, hfHeight;
		if (aspect > 1.0) {
			hfWidth = 0.5*req.width;
			hfHeight = 0.5*req.height / aspect;
		}
		else {
			hfHeight = 0.5*req.width;
			hfWidth = 0.5*req.height * aspect;
		}

		// Draw a quad to put the texture on
		glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
		glColor3f(1, 1, 1);
		glBegin(GL_QUADS);
		glTexCoord2i(0, 1);
		glVertex2f(0.5*req.width - hfWidth, 0.5*req.height - hfHeight);
		glTexCoord2i(1, 1);
		glVertex2f(0.5*req.width + hfWidth, 0.5*req.height - hfHeight);
		glTexCoord2i(1, 0);
		glVertex2f(0.5*req.width + hfWidth, 0.5*req.height + hfHeight);
		glTexCoord2i(0, 0);
		glVertex2f(0.5*req.width - hfWidth, 0.5*req.height + hfHeight);
		glEnd();
	}

	return false;
}

void ShaderSelector::displayShaderInfo(const MaterialPtr& shader,
									   const Glib::RefPtr<Gtk::ListStore>& listStore,
									   int attrCol, int valueCol)
{
	// Update the infostore in the ShaderSelector
	Gtk::TreeModel::iterator iter = listStore->append();

	iter->set_value(attrCol, std::string("<b>") + _("Shader") + "</b>");
	iter->set_value(valueCol, shader->getName());

	// Containing MTR File
	iter = listStore->append();
	iter->set_value(attrCol, std::string("<b>") + _("Defined in") + "</b>");
	iter->set_value(valueCol, Glib::ustring(shader->getShaderFileName()));

	// Description
	iter = listStore->append();
	iter->set_value(attrCol, std::string("<b>") + _("Description") + "</b>");
	iter->set_value(valueCol, shader->getDescription());
}

void ShaderSelector::displayLightShaderInfo(const MaterialPtr& shader,
											const Glib::RefPtr<Gtk::ListStore>& listStore,
											int attrCol, int valueCol)
{
	const ShaderLayer* first = shader->firstLayer();
	std::string texName = _("None");
	if (first != NULL) {
		TexturePtr tex = shader->firstLayer()->getTexture();
		texName = tex->getName();
	}

	Gtk::TreeModel::iterator iter = listStore->append();

	iter->set_value(attrCol, std::string("<b>") + _("Image map") + "</b>");
	iter->set_value(valueCol, texName);

	// Name of file containing the shader
	iter = listStore->append();
	iter->set_value(attrCol, std::string("<b>") + _("Defined in") + "</b>");
	iter->set_value(valueCol, Glib::ustring(shader->getShaderFileName()));

	// Light types, from the Material
	std::string lightType;
	if (shader->isAmbientLight())
		lightType.append("ambient ");
	if (shader->isBlendLight())
		lightType.append("blend ");
	if (shader->isFogLight())
		lightType.append("fog");
	if (lightType.size() == 0)
		lightType.append("-");

	iter = listStore->append();
	iter->set_value(attrCol, std::string("<b>") + _("Light flags") + "</b>");
	iter->set_value(valueCol, lightType);

	// Description
	iter = listStore->append();
	iter->set_value(attrCol, std::string("<b>") + _("Description") + "</b>");

	std::string descr = shader->getDescription();
	iter->set_value(valueCol, descr.empty() ? "-" : descr);
}

// Callback for selection changed
void ShaderSelector::_onSelChange()
{
	updateInfoTable();
	_glWidget->queue_draw();
}

} // namespace ui
