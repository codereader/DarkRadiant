#include "ShaderSelector.h"

#include "iuimanager.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/TextColumn.h"
#include "gtkutil/IconTextColumn.h"
#include "gtkutil/VFSTreePopulator.h"
#include "gtkutil/GLWidgetSentry.h"
#include "signal/isignal.h"
#include "texturelib.h"
#include "string/string.h"
#include "ishaders.h"
#include "iregistry.h"

#include <gtk/gtk.h>
#include <GL/glew.h>
#include <vector>
#include <string>
#include <map>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/case_conv.hpp>

namespace ui
{

/* CONSTANTS */

namespace {
	const char* FOLDER_ICON = "folder16.png";
	const char* TEXTURE_ICON = "icon_texture.png";
	
	// Column enum
	enum {
		NAME_COL, // shader name only (without path)
		FULLNAME_COL, // Full shader name 
		IMAGE_COL, // Icon
		N_COLUMNS
	};
}

// Constructor creates GTK elements
ShaderSelector::ShaderSelector(Client* client, const std::string& prefixes, bool isLightTexture) :
	_glWidget(true),
	_infoStore(gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING)),
	_client(client),
	_isLightTexture(isLightTexture)
{
	// Split the given comma-separated list into the vector
	boost::algorithm::split(_prefixes, prefixes, boost::algorithm::is_any_of(","));
	
	// Construct main VBox, and pack in TreeView and info panel
	_widget = gtk_vbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(_widget), createTreeView(), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(_widget), createPreview(), FALSE, FALSE, 0);
}

// Return the selection to the calling code
std::string ShaderSelector::getSelection() {

	// Get the selection
	GtkTreeIter iter;
	GtkTreeModel* model;
	if (gtk_tree_selection_get_selected(_selection, &model, &iter)) {
		return gtkutil::TreeModel::getString(model, &iter, FULLNAME_COL);
	}
	else {
		// Nothing selected, return empty string
		return "";
	}
}

// Set the selection in the treeview
void ShaderSelector::setSelection(const std::string& sel) {

	// If the selection string is empty, collapse the treeview and return with
	// no selection
	if (sel.empty()) {
		gtk_tree_view_collapse_all(GTK_TREE_VIEW(_treeView));
		return;
	}

	// Use the local SelectionFinder class to walk the TreeModel
	gtkutil::TreeModel::SelectionFinder finder(boost::algorithm::to_lower_copy(sel), FULLNAME_COL);
	GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(_treeView));
	gtk_tree_model_foreach(model, gtkutil::TreeModel::SelectionFinder::forEach, &finder);
	
	// Get the found TreePath (may be NULL)
	GtkTreePath* path = finder.getPath();
	if (path) {
		// Expand the treeview to display the target row
		gtk_tree_view_expand_to_path(GTK_TREE_VIEW(_treeView), path);
		// Highlight the target row
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(_treeView), path, NULL, false);
		// Make the selected row visible 
		gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(_treeView), path, NULL, true, 0.3f, 0.0f);
	}
}

// Local functor to populate the tree view with shader names

namespace {

	// VFSPopulatorVisitor to fill in column data for the populator tree nodes
	class DataInserter
	: public gtkutil::VFSTreePopulator::Visitor
	{
		// Required visit function
		void visit(GtkTreeStore* store, 
				   GtkTreeIter* iter, 
				   const std::string& path,
				   bool isExplicit)
		{

			// Get the display name by stripping off everything before the last
			// slash
			std::string displayName = path.substr(path.rfind("/") + 1);
			
			// Pathname is the model VFS name for a model, and blank for a folder
			std::string fullPath = isExplicit ? path : "";

			// Pixbuf depends on node type
			GdkPixbuf* pixBuf = isExplicit 
								? GlobalUIManager().getLocalPixbuf(TEXTURE_ICON)
								: GlobalUIManager().getLocalPixbuf(FOLDER_ICON);
			// Fill in the column values
			gtk_tree_store_set(store, iter, 
							   NAME_COL, displayName.c_str(),
							   FULLNAME_COL, fullPath.c_str(),
							   IMAGE_COL, pixBuf,
							   -1);
		} 	
		public:
		virtual ~DataInserter() {}
	};

	class ShaderNameFunctor 
	{
	public:
		typedef const char* first_argument_type;

		// Interesting texture prefixes
		ShaderSelector::PrefixList& _prefixes;

		// The populator that gets called to add the parsed elements
		gtkutil::VFSTreePopulator& _populator;

		// Map of prefix to an path pointing to the top-level row that contains
		// instances of this prefix
		std::map<std::string, GtkTreePath*> _iterMap;
		
		// Constructor
		ShaderNameFunctor(gtkutil::VFSTreePopulator& populator, ShaderSelector::PrefixList& prefixes) :
			_prefixes(prefixes), 
			_populator(populator)
		{}
		
		// Destructor. Each GtkTreePath needs to be explicitly freed
		~ShaderNameFunctor() {
			for (std::map<std::string, GtkTreePath*>::iterator i = _iterMap.begin();
					i != _iterMap.end();
				 		++i) 
			{
				gtk_tree_path_free(i->second);
			}
		}
	
		// Functor operator
		void operator() (const char* shaderName) {
			std::string name = boost::algorithm::to_lower_copy(std::string(shaderName));
			
			for (ShaderSelector::PrefixList::iterator i = _prefixes.begin();
				 i != _prefixes.end();
				 i++)
			{
				if (!name.empty() && boost::algorithm::istarts_with(name, (*i) + "/")) {
					_populator.addPath(name);
					break; // don't consider any further prefixes
				}
			}
		}
	};
}

// Create the Tree View
GtkWidget* ShaderSelector::createTreeView() {
	// Tree model
	GtkTreeStore* store = gtk_tree_store_new(N_COLUMNS,
											 G_TYPE_STRING, // display name in tree
											 G_TYPE_STRING, // full shader name
											 GDK_TYPE_PIXBUF); 
	
	// Instantiate the helper class that populates the tree according to the paths
	gtkutil::VFSTreePopulator populator(store);
	
	ShaderNameFunctor func(populator, _prefixes);
	GlobalMaterialManager().foreachShaderName(makeCallback1(func));
	
	// Now visit the created GtkTreeIters to load the actual data into the tree
	DataInserter inserter;
	populator.forEachNode(inserter);
	
	// Tree view
	_treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(_treeView), FALSE);
	g_object_unref(store); // tree view owns the reference now

	// Single visible column, containing the directory/shader name and the icon
	gtk_tree_view_append_column(GTK_TREE_VIEW(_treeView),
								gtkutil::IconTextColumn("Value",
														NAME_COL,
														IMAGE_COL));

	// Set the tree store to sort on this column
    gtk_tree_sortable_set_sort_column_id(
        GTK_TREE_SORTABLE(store),
        NAME_COL,
        GTK_SORT_ASCENDING
    );

	// Use the TreeModel's full string search function
	gtk_tree_view_set_search_equal_func(GTK_TREE_VIEW(_treeView), gtkutil::TreeModel::equalFuncStringContains, NULL, NULL);

	// Get selection and connect the changed callback
	_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_treeView));
	g_signal_connect(G_OBJECT(_selection), 
					 "changed", 
					 G_CALLBACK(_onSelChange), 
					 this);

	// Pack into scrolled window and frame
	return gtkutil::ScrolledFrame(_treeView);
}

// Create the preview panel (GL widget and info table)
GtkWidget* ShaderSelector::createPreview() {

	// HBox contains the preview GL widget along with a texture attributes
	// pane.
	GtkWidget* hbx = gtk_hbox_new(FALSE, 3);

	// Cast the GLWidget object to GtkWidget
	GtkWidget* glWidget = _glWidget;
	gtk_widget_set_size_request(glWidget, 128, 128);
	g_signal_connect(G_OBJECT(glWidget), 
					 "expose-event", 
					 G_CALLBACK(_onExpose), 
					 this);
	GtkWidget* glFrame = gtk_frame_new(NULL);
	gtk_container_add(GTK_CONTAINER(glFrame), glWidget);
	gtk_box_pack_start(GTK_BOX(hbx), glFrame, FALSE, FALSE, 0);
	
	// Attributes table

	GtkWidget* tree = gtk_tree_view_new_with_model(GTK_TREE_MODEL(_infoStore));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), FALSE);
	
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree),
								gtkutil::TextColumn("Attribute", 0));
	
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree),
								gtkutil::TextColumn("Value", 1));

	gtk_box_pack_start(GTK_BOX(hbx), 
					   gtkutil::ScrolledFrame(tree), 
					   TRUE, TRUE, 0);
	return hbx;
}

// Get the selected shader
MaterialPtr ShaderSelector::getSelectedShader() {
	return GlobalMaterialManager().getMaterialForName(getSelection());	
}

// Update the attributes table
void ShaderSelector::updateInfoTable() {
	
	gtk_list_store_clear(_infoStore);

	// Get the selected texture name. If nothing is selected, we just leave the
	// infotable empty.
	std::string selName = getSelection();
	
	// Notify the client of the change to give it a chance to update the infostore
	if (_client != NULL && !selName.empty()) {
		_client->shaderSelectionChanged(selName, _infoStore);
	}
}

// Callback to redraw the GL widget
void ShaderSelector::_onExpose(GtkWidget* widget, 
								GdkEventExpose* ev,
								ShaderSelector* self) 
{
	// The scoped object making the GL widget the current one
	gtkutil::GLWidgetSentry sentry(widget);
	
	// Get the viewport size from the GL widget
	GtkRequisition req;
	gtk_widget_size_request(widget, &req);
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
	MaterialPtr shader = self->getSelectedShader();
	
	bool drawQuad = false;
	TexturePtr tex;
	
	// Check what part of the shader we should display in the preview 
	if (self->_isLightTexture) {
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
}

void ShaderSelector::displayShaderInfo(MaterialPtr shader, GtkListStore* listStore) 
{
	// Update the infostore in the ShaderSelector
	GtkTreeIter iter;
	
	gtk_list_store_append(listStore, &iter);
	gtk_list_store_set(listStore, &iter, 
					   0, "<b>Shader</b>",
					   1, shader->getName().c_str(),
					   -1);
	
	// Containing MTR	
	gtk_list_store_append(listStore, &iter);
	gtk_list_store_set(listStore, &iter, 
					   0, "<b>Defined in</b>",
					   1, shader->getShaderFileName(),
					   -1);

	// Description	
	gtk_list_store_append(listStore, &iter);
	gtk_list_store_set(listStore, &iter, 
					   0, "<b>Description</b>",
					   1, shader->getDescription().c_str(),
					   -1);
}

void ShaderSelector::displayLightShaderInfo(MaterialPtr shader, GtkListStore* listStore) {
	
	const ShaderLayer* first = shader->firstLayer();
	std::string texName = "None";
	if (first != NULL) {
		TexturePtr tex = shader->firstLayer()->getTexture();
		texName = tex->getName();
	}

	GtkTreeIter iter;
	gtk_list_store_append(listStore, &iter);
	gtk_list_store_set(listStore, &iter, 
					   0, "<b>Image map</b>",
					   1, texName.c_str(),
					   -1);

	// Name of file containing the shader
	gtk_list_store_append(listStore, &iter);
	gtk_list_store_set(listStore, &iter, 
					   0, "<b>Defined in</b>",
					   1, shader->getShaderFileName(),
					   -1);

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
	
	gtk_list_store_append(listStore, &iter);
	gtk_list_store_set(listStore, &iter, 
					   0, "<b>Light flags</b>",
					   1, lightType.c_str(),
					   -1);
}

// Callback for selection changed
void ShaderSelector::_onSelChange(GtkWidget* widget, ShaderSelector* self) {
	self->updateInfoTable();
	gtk_widget_queue_draw(self->_glWidget);
}

} // namespace ui
