#include "LightTextureSelector.h"

#include "gtkutil/glwidget.h"
#include "gtkutil/TreeModel.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/TextColumn.h"
#include "signal/isignal.h"
#include "texturelib.h"
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

namespace ui
{

/* CONSTANTS */

namespace {
	
	const char* LIGHT_PREFIX_XPATH = "game/light/texture//prefix";
	
	// Column enum
	enum {
		DISPLAYNAME_COL,
		FULLNAME_COL,
		N_COLUMNS
	};
	
}

// Constructor creates GTK elements
LightTextureSelector::LightTextureSelector()
: _infoStore(gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING))
{
	// Construct main VBox, and pack in TreeView and info panel
	_widget = gtk_vbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(_widget), createTreeView(), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(_widget), createPreview(), FALSE, FALSE, 0);
}

// Return the selection to the calling code
std::string LightTextureSelector::getSelection() {

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

namespace {

/* Local object that walks the GtkTreeModel and obtains a GtkTreePath locating
 * the given texture. The gtk_tree_model_foreach function requires a pointer to
 * a function, which in this case is a static member of the walker object that
 * accepts a void* pointer to the instance (like other GTK callbacks).
 */
class SelectionFinder {
	
	// String containing the texture to highlight
	std::string _texture;
	
	// The GtkTreePath* pointing to the required texture
	GtkTreePath* _path;
	
public:

	// Constructor
	SelectionFinder(const std::string& selection)
	: _texture(selection),
	  _path(NULL)
	{ }
	
	// Retrieve the found TreePath, which may be NULL if the texture was not
	// found
	GtkTreePath* getPath() {
		return _path;
	}
	
	// Static callback for GTK
	static gboolean forEach(GtkTreeModel* model,
							GtkTreePath* path,
							GtkTreeIter* iter,
							gpointer vpSelf)
	{
		// Get the self instance from the void pointer
		SelectionFinder* self = 
			reinterpret_cast<SelectionFinder*>(vpSelf);
			
		// If the visited row matches the texture to find, set the _path
		// variable and finish, otherwise continue to search
		if (gtkutil::TreeModel::getString(model, iter, FULLNAME_COL) 
			== self->_texture)
		{
			self->_path = gtk_tree_path_copy(path);
			return TRUE; // finish the walk
		}
		else 
		{
			return FALSE;
		}
	} 

};

} // local namespace

// Set the selection in the treeview
void LightTextureSelector::setSelection(const std::string& sel) {

	// If the selection string is empty, collapse the treeview and return with
	// no selection
	if (sel.empty()) {
		gtk_tree_view_collapse_all(GTK_TREE_VIEW(_treeView));
		return;
	}

	// Use the local SelectionFinder class to walk the TreeModel
	SelectionFinder finder(sel);
	GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(_treeView));
	gtk_tree_model_foreach(model, SelectionFinder::forEach, &finder);
	
	// Get the found TreePath (may be NULL)
	GtkTreePath* path = finder.getPath();
	if (path) {
		// Expand the treeview to display the target row
		gtk_tree_view_expand_to_path(GTK_TREE_VIEW(_treeView), path);
		// Highlight the target row
		gtk_tree_view_set_cursor(GTK_TREE_VIEW(_treeView), path, NULL, false);
	}
}

// Local functor to populate the tree view with shader names

namespace {

	struct ShaderNameFunctor {
	
		typedef const char* first_argument_type;

		// Interesting texture prefixes
		std::vector<std::string> _prefixes;

		// Tree Store to add to
		GtkTreeStore* _store;

		// Map of prefix to an path pointing to the top-level row that contains
		// instances of this prefix
		std::map<std::string, GtkTreePath*> _iterMap;
		
		// Constructor
		ShaderNameFunctor(GtkTreeStore* store)
		: _store(store)
		{
			// Get the list of light texture prefixes from the registry
			xml::NodeList prefList = 
				GlobalRegistry().findXPath(LIGHT_PREFIX_XPATH);
			
			// Copy the Node contents into the prefix vector	
			for (xml::NodeList::iterator i = prefList.begin();
				 i != prefList.end();
				 ++i)
			{
				_prefixes.push_back(i->getContent());
			}
		}
		
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
			std::string name(shaderName);
			for (std::vector<std::string>::iterator i = _prefixes.begin();
				 i != _prefixes.end();
				 i++)
			{
				if (boost::algorithm::istarts_with(name, (*i) + "/")) {

					// Prepare to find the path to the top-level parent of this texture entry
					GtkTreePath* pathToParent = NULL;

					// If this prefix hasn't been seen yet, add a top-level parent for it
					if (_iterMap.find(*i) == _iterMap.end()) {
						GtkTreeIter iter;
						gtk_tree_store_append(_store, &iter, NULL);
						gtk_tree_store_set(_store, &iter, 0, i->c_str(), 1, "", -1);
						_iterMap[*i] = pathToParent = gtk_tree_model_get_path(GTK_TREE_MODEL(_store), &iter);
					}

					// Look up pathToParent in the map, if we didn't just add it
					if (pathToParent == NULL) {
						pathToParent = _iterMap.find(*i)->second;
					}

					// Get the parent iter from the pathToParent TreePath
					GtkTreeIter parIter;
					gtk_tree_model_get_iter(GTK_TREE_MODEL(_store), &parIter, pathToParent);
				
					// Add the texture entry
					GtkTreeIter iter;
					gtk_tree_store_append(_store, &iter, &parIter);
					gtk_tree_store_set(_store, &iter, 
										0, name.substr(i->length() + 1).c_str(), // display name
										1, name.c_str(), // full shader name
										-1);
										
					break; // don't consider any further prefixes
				}
			}
		}
	};
}

// Create the Tree View
GtkWidget* LightTextureSelector::createTreeView() {
	// Tree model
	GtkTreeStore* store = gtk_tree_store_new(N_COLUMNS, 
											 G_TYPE_STRING, // display name in tree
											 G_TYPE_STRING); // full shader name
	
	ShaderNameFunctor func(store);
	GlobalShaderSystem().foreachShaderName(makeCallback1(func));
	
	// Tree view
	_treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(_treeView), FALSE);
	g_object_unref(store); // tree view owns the reference now

	// Single text column to display texture name	
	gtk_tree_view_append_column(GTK_TREE_VIEW(_treeView), 
								gtkutil::TextColumn("Texture", 
													DISPLAYNAME_COL));				
	
	// Get selection and connect the changed callback
	_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(_treeView));
	g_signal_connect(G_OBJECT(_selection), 
					 "changed", 
					 G_CALLBACK(_onSelChange), 
					 this);

	// Pack into scrolled window and frame
	GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), 
									GTK_POLICY_AUTOMATIC, 
										GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scroll), _treeView);
	
	GtkWidget* fr = gtk_frame_new(NULL);
	gtk_container_add(GTK_CONTAINER(fr), scroll);
	
	return fr;
}

// Create the preview panel (GL widget and info table)
GtkWidget* LightTextureSelector::createPreview() {

	// HBox contains the preview GL widget along with a texture attributes
	// pane.
	GtkWidget* hbx = gtk_hbox_new(FALSE, 3);

	// GtkGLExt widget
	_glWidget = glwidget_new(false);
	gtk_widget_set_size_request(_glWidget, 128, 128);
	g_signal_connect(G_OBJECT(_glWidget), 
					 "expose-event", 
					 G_CALLBACK(_onExpose), 
					 this);
	GtkWidget* glFrame = gtk_frame_new(NULL);
	gtk_container_add(GTK_CONTAINER(glFrame), _glWidget);
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
IShaderPtr LightTextureSelector::getSelectedShader() {
	return GlobalShaderSystem().getShaderForName(getSelection());	
}

// Callback to redraw the GL widget
void LightTextureSelector::_onExpose(GtkWidget* widget, 
									GdkEventExpose* ev,
									LightTextureSelector* self) 
{
	if (glwidget_make_current(widget) != FALSE) {
		// Get the viewport size from the GL widget
		GtkRequisition req;
		gtk_widget_size_request(widget, &req);
		glViewport(0, 0, req.width, req.height);

		// Initialise
		glClearColor(0.3, 0.3, 0.3, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, req.width, 0, req.height, -100, 100);
		glEnable (GL_TEXTURE_2D);

		// Get the selected texture, and set up OpenGL to render it on
		// the quad.
		IShaderPtr shader = self->getSelectedShader();
		const ShaderLayer* first = shader->firstLayer();
		if (first != NULL) {
			TexturePtr tex = shader->firstLayer()->texture();
			glBindTexture (GL_TEXTURE_2D, tex->texture_number);
		} else {
			goto swapAndRelease; // don't draw, leave window cleared
		}
		
		// Draw a quad to put the texture on
		glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
		glColor3f(1, 1, 1);
		glBegin(GL_QUADS);
		glTexCoord2i(0, 1);
		glVertex2i(0, 0);
		glTexCoord2i(1, 1);
		glVertex2i(req.height, 0);
		glTexCoord2i(1, 0);
		glVertex2i(req.height, req.height);
		glTexCoord2i(0, 0);
		glVertex2i(0, req.height);
		glEnd();

swapAndRelease:

		// Update GtkGlExt buffer
		glwidget_swap_buffers(widget);
		
		// Release the IShader
		shader->DecRef();
	}
}

// Callback for selection changed
void LightTextureSelector::_onSelChange(GtkWidget* widget, 
									   LightTextureSelector* self) 
{
	self->updateInfoTable();
	gtk_widget_queue_draw(self->_glWidget);
}

// Update the attributes table
void LightTextureSelector::updateInfoTable() {
	GtkTreeIter iter;
	gtk_list_store_clear(_infoStore);

	// Get the selected texture name. If nothing is selected, we just leave the
	// infotable empty.
	std::string selName = getSelection();
	if (selName.empty())
		return;

	gtk_list_store_append(_infoStore, &iter);
	gtk_list_store_set(_infoStore, &iter, 
					   0, "<b>Shader</b>",
					   1, selName.c_str(),
					   -1);

	// Get the shader, and its image map if possible
	IShaderPtr shader = getSelectedShader();

	const ShaderLayer* first = shader->firstLayer();
	std::string texName = "None";
	if (first != NULL) {
		TexturePtr tex = shader->firstLayer()->texture();
		texName = tex->name;
	}

	gtk_list_store_append(_infoStore, &iter);
	gtk_list_store_set(_infoStore, &iter, 
					   0, "<b>Image map</b>",
					   1, texName.c_str(),
					   -1);

	// Name of file containing the shader

	gtk_list_store_append(_infoStore, &iter);
	gtk_list_store_set(_infoStore, &iter, 
					   0, "<b>Defined in</b>",
					   1, shader->getShaderFileName(),
					   -1);

	// Light types, from the IShader

	std::string lightType;
	if (shader->isAmbientLight())
		lightType.append("ambient ");
	if (shader->isBlendLight())
		lightType.append("blend ");
	if (shader->isFogLight())
		lightType.append("fog");
	if (lightType.size() == 0)
		lightType.append("-");
	
	gtk_list_store_append(_infoStore, &iter);
	gtk_list_store_set(_infoStore, &iter, 
					   0, "<b>Light flags</b>",
					   1, lightType.c_str(),
					   -1);
					   
	// Release the IShader reference
	shader->DecRef();
	
}
	


}
