#include "ModelSelector.h"

#include "referencecache.h"
#include "mainframe.h"
#include "gtkutil/image.h"
#include "gtkutil/glwidget.h"
#include "generic/vector.h"
#include "ifilesystem.h"
#include "irender.h"
#include "modelskin.h"
#include "os/path.h"

#include <cstdlib>
#include <iostream>
#include <vector>
#include <ext/hash_map>
#include <sstream>
#include <GL/glew.h>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/functional/hash/hash.hpp>
#include <boost/lexical_cast.hpp>

namespace ui
{

// CONSTANTS

namespace {
	
	const char* MODELSELECTOR_TITLE = "Choose model";
	const char* MODELS_FOLDER = "models/";

	const char* ASE_EXTENSION = ".ase";
	const char* LWO_EXTENSION = ".lwo";
	const char* LWO_ICON = "model16red.png";
	const char* ASE_ICON = "model16green.png";
	const char* SKIN_ICON = "skin16.png";
	const char* FOLDER_ICON = "folder16.png";
	
	const GLfloat PREVIEW_FOV = 60;

	// Treestore enum
	enum {
		NAME_COLUMN,		// e.g. "chair1.lwo"
		FULLNAME_COLUMN,	// e.g. "models/darkmod/props/chair1.lwo"
		SKIN_COLUMN,		// e.e. "chair1_brown_wood", or "" for no skin
		IMAGE_COLUMN,		// icon to display
		N_COLUMNS
	};
	
}

// Constructor.

ModelSelector::ModelSelector()
: _widget(gtk_window_new(GTK_WINDOW_TOPLEVEL)),
  _treeStore(gtk_tree_store_new(N_COLUMNS, 
  								G_TYPE_STRING,
  								G_TYPE_STRING,
  								G_TYPE_STRING,
  								GDK_TYPE_PIXBUF)),
  _infoStore(gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING)),
  _lastModel(""),
  _camDist(-5.0f)
{
	// Window properties
	
	gtk_window_set_transient_for(GTK_WINDOW(_widget), MainFrame_getWindow());
	gtk_window_set_modal(GTK_WINDOW(_widget), TRUE);
	gtk_window_set_title(GTK_WINDOW(_widget), MODELSELECTOR_TITLE);
    gtk_window_set_position(GTK_WINDOW(_widget), GTK_WIN_POS_CENTER_ON_PARENT);

	// Set the default size of the window
	
	GdkScreen* scr = gtk_window_get_screen(GTK_WINDOW(_widget));
	gint w = gdk_screen_get_width(scr);
	gint h = gdk_screen_get_height(scr);
	
	gtk_window_set_default_size(GTK_WINDOW(_widget), w / 2, gint(h / 1.5));

	// Signals
	
	g_signal_connect(G_OBJECT(_widget), "delete_event", G_CALLBACK(callbackHide), this);
	
	// Main window contains a VBox
	
	GtkWidget* vbx = gtk_vbox_new(FALSE, 3);
	gtk_box_pack_start(GTK_BOX(vbx), createTreeView(), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbx), createPreviewAndInfoPanel(), FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(vbx), createButtons(), FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(_widget), vbx);
}

// Show the dialog and enter recursive main loop

std::string ModelSelector::showAndBlock() {
	gtk_widget_show_all(_widget);
	initialisePreview();
	gtk_main(); // recursive main loop. This will block until the dialog is closed in some way.
	return _lastModel;
}

// Static function to display the instance, and return the selected
// model to the calling function

std::string ModelSelector::chooseModel() {
	static ModelSelector _selector;
	return _selector.showAndBlock();
}

// File-local functor object to retrieve model names from global VFS

namespace {

	struct ModelFileFunctor {
	
		typedef const char* first_argument_type;
		
		// Tree store to populate

		GtkTreeStore* _store;

		// Map between model directory names (e.g. "models/darkmod/architecture") and
		// a GtkTreeIter pointing to the equivalent row in the TreeModel. Subsequent
		// modelpaths with this directory will be added as children of this iter.

		typedef __gnu_cxx::hash_map<std::string, GtkTreeIter*, boost::hash<std::string> > DirIterMap;
		DirIterMap _dirIterMap;
		
		// Constructor
		
		ModelFileFunctor(GtkTreeStore* store)
		: _store(store) {}
		
		// Destructor. The GtkTreeIters are dynamically allocated so we must free them
		
		~ModelFileFunctor() {
			for (DirIterMap::iterator i = _dirIterMap.begin();
					i != _dirIterMap.end();
						++i) 
			{
				gtk_tree_iter_free(i->second);
			}
		}
		
		// Recursive function to add a given model path ("models/darkmod/something/model.lwo")
		// to its correct place in the tree. This is done by maintaining a cache of 
		// directory nodes ("models/darkmod/something", "models/darkmod") against 
		// GtkIters that point to the corresponding row in the tree model. On each call,
		// the parent node is recursively calculated, and the node provided as an argument
		// added as a child.
		
		GtkTreeIter* addRecursive(const std::string& dirPath) {
			
			// We first try to lookup the directory name in the map. Return it
			// if it exists, otherwise recursively obtain the parent of this directory name,
			// and add this directory as a child in the tree model. We also add this
			// directory to the map for future lookups.
			
			DirIterMap::iterator iTemp = _dirIterMap.find(dirPath);
			if (iTemp != _dirIterMap.end()) { // found in map
				return iTemp->second;
			}
			else { 

				// Perform the search for final "/" which identifies the parent
				// of this directory, and call recursively. If there is no slash, we
				// are looking at a toplevel directory in which case the parent is
				// NULL.
				unsigned int slashPos = dirPath.rfind("/");
				GtkTreeIter* parIter = NULL;
				
				if (slashPos != std::string::npos) {
					parIter = addRecursive(dirPath.substr(0, slashPos));
				}

				/* Add this directory to the treemodel. For the displayed tree, we
				** want the last component of the directory name, not the entire path
				** at each node.
				*/

				std::stringstream nodeName;
				nodeName << dirPath.substr(slashPos + 1);

				// Get the list of skins for this model. The number of skins is appended
				// to the model node name in brackets.
				ModelSkinList skins = GlobalModelSkinCache().getSkinsForModel(MODELS_FOLDER + dirPath);
				int numSk = skins.size();
				if (numSk > 0) {
					nodeName << " [" << numSk << (numSk == 1 ? " skin]" : " skins]");
				}

				// Decide which image to use, based on the file extension (or the folder
				// image if there is no extension). Also, set a flag indicating that we
				// have an actual model rather than a directory, so that the fullname
				// tree column can be populated
				
				std::string imgPath = FOLDER_ICON;
				bool isModel = false;

				if (boost::algorithm::iends_with(dirPath, LWO_EXTENSION)) {
					imgPath = LWO_ICON;
					isModel = true;
				}
				else if (boost::algorithm::iends_with(dirPath, ASE_EXTENSION)) {
					imgPath = ASE_ICON;
					isModel = true;
				}

				// Add the fields to the treeview
				
				GtkTreeIter iter;
				gtk_tree_store_append(_store, &iter, parIter);
				gtk_tree_store_set(_store, &iter, 
						NAME_COLUMN, nodeName.str().c_str(),
						FULLNAME_COLUMN, (isModel ? (MODELS_FOLDER + dirPath).c_str() : ""),
						SKIN_COLUMN, "",
						IMAGE_COLUMN, gtkutil::getLocalPixbuf(imgPath),
						-1);
				GtkTreeIter* dynIter = gtk_tree_iter_copy(&iter); // get a heap-allocated iter

				// Determine if this model has any associated skins, and add them as
				// children. We also set the fullpath column to the model name for each skin.
				
				for (ModelSkinList::iterator i = skins.begin(); i != skins.end(); ++i) {
					GtkTreeIter skIter;
					gtk_tree_store_append(_store, &skIter, &iter);
					gtk_tree_store_set(_store, &skIter, 
							NAME_COLUMN, i->c_str(),
							FULLNAME_COLUMN, (MODELS_FOLDER + dirPath).c_str(),
							SKIN_COLUMN, i->c_str(),
							IMAGE_COLUMN, gtkutil::getLocalPixbuf(SKIN_ICON),
							-1);
				}
				
				// Now add a map entry that maps our directory name to the row we just
				// added
				_dirIterMap[dirPath] = dynIter;
				
				// Return our new dynamic iter. 
				return dynIter;

			}				

		}

		// Functor operator
		
		void operator() (const char* file) {

			std::string rawPath(file);			

			// Test the extension. If it is not LWO or ASE (case-insensitive),
			// not interested
			if (!boost::algorithm::iends_with(rawPath, LWO_EXTENSION) 
					&& !boost::algorithm::iends_with(rawPath, ASE_EXTENSION)) 
			{
				return;
			}
			else 
			{
				addRecursive(rawPath);
			}
							   
		}

	
	};

}

// Helper function to create the TreeView

GtkWidget* ModelSelector::createTreeView() {

	// Populate the treestore using the VFS callback functor
	
	ModelFileFunctor functor(_treeStore);
	GlobalFileSystem().forEachFile(MODELS_FOLDER, "*", makeCallback1(functor), 0);

	GtkWidget* treeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(_treeStore));

	// Single visible column, containing the directory/model name and the icon
	
    GtkTreeViewColumn* col = gtk_tree_view_column_new();
    gtk_tree_view_column_set_title(col, "Value");
    gtk_tree_view_column_set_spacing(col, 3);

	GtkCellRenderer* pixRenderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(col, pixRenderer, FALSE);
    gtk_tree_view_column_set_attributes(col, pixRenderer, "pixbuf", IMAGE_COLUMN, NULL);

	GtkCellRenderer* rend = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, rend, FALSE);
	gtk_tree_view_column_set_attributes(col, rend, "text", NAME_COLUMN, NULL);

	gtk_tree_view_append_column(GTK_TREE_VIEW(treeView), col);				
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeView), FALSE);

	// Get the selection object and connect to its changed signal

	_selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeView));
	g_signal_connect(G_OBJECT(_selection), "changed", G_CALLBACK(callbackSelChanged), this);

	// Pack treeview into a scrolled window and frame, and return
	
	GtkWidget* scrollWin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollWin),
								   GTK_POLICY_AUTOMATIC,
								   GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrollWin), treeView);
	
	GtkWidget* fr = gtk_frame_new(NULL);
	gtk_container_add(GTK_CONTAINER(fr), scrollWin);
	
	return fr;
	
}

// Create the buttons panel at bottom of dialog

GtkWidget* ModelSelector::createButtons() {
	GtkWidget* hbx = gtk_hbox_new(FALSE, 3);
	
	GtkWidget* okButton = gtk_button_new_from_stock(GTK_STOCK_OK);
	GtkWidget* cancelButton = gtk_button_new_from_stock(GTK_STOCK_CANCEL);
	
	g_signal_connect(G_OBJECT(okButton), "clicked", G_CALLBACK(callbackOK), this);
	g_signal_connect(G_OBJECT(cancelButton), "clicked", G_CALLBACK(callbackCancel), this);
	
	gtk_box_pack_end(GTK_BOX(hbx), okButton, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(hbx), cancelButton, FALSE, FALSE, 0);
	return hbx;
}

// Create the preview widget and info panel

GtkWidget* ModelSelector::createPreviewAndInfoPanel() {

	// This is an HBox with the preview GL widget on the left, and an info TreeView on the
	// right
	
	GtkWidget* hbx = gtk_hbox_new(FALSE, 3);
	
	// GL Widget
	
	gtk_box_pack_start(GTK_BOX(hbx), createGLWidget(), FALSE, FALSE, 0);
	
	// Info table. Has key and value columns.
	
	GtkWidget* infTreeView = gtk_tree_view_new_with_model(GTK_TREE_MODEL(_infoStore));
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(infTreeView), FALSE);
	
	GtkCellRenderer* rend;
	GtkTreeViewColumn* col;
	
	rend = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes("Attribute",
												   rend,
												   "text", 0,
												   NULL);
	g_object_set(G_OBJECT(rend), "weight", 700, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(infTreeView), col);
	
	rend = gtk_cell_renderer_text_new();
	col = gtk_tree_view_column_new_with_attributes("Value",
												   rend,
												   "text", 1,
												   NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(infTreeView), col);
	
	// Pack into scroll window and frame
	
	GtkWidget* scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
								   GTK_POLICY_AUTOMATIC,
								   GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scroll), infTreeView);

	GtkWidget* frame = gtk_frame_new(NULL);
	gtk_container_add(GTK_CONTAINER(frame), scroll);

	gtk_box_pack_start(GTK_BOX(hbx), frame, TRUE, TRUE, 0);
	
	// Return the HBox
	return hbx;	
	
}

// Create the preview GL widget

GtkWidget* ModelSelector::createGLWidget() {

	// Create the widget and connect up the signals
	_glWidget = glwidget_new(TRUE);
	gtk_widget_set_size_request(_glWidget, 256, 256);
	gtk_widget_set_events(_glWidget, GDK_DESTROY | GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_SCROLL_MASK);
	g_signal_connect(G_OBJECT(_glWidget), "expose-event", G_CALLBACK(callbackGLDraw), this);
	g_signal_connect(G_OBJECT(_glWidget), "motion-notify-event", G_CALLBACK(callbackGLMotion), this);
	g_signal_connect(G_OBJECT(_glWidget), "scroll-event", G_CALLBACK(callbackGLScroll), this);
	
	// Pack into a frame and return
	GtkWidget* glFrame = gtk_frame_new(NULL);
	gtk_container_add(GTK_CONTAINER(glFrame), _glWidget);
	return glFrame;
}

// Initialise the preview GL stuff

void ModelSelector::initialisePreview() {

	// Clear the window and set up the initial transformations
	if (glwidget_make_current(_glWidget) != FALSE) {

		// Depth buffer and polygon mode
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LEQUAL);

		// Clear the window
		glClearColor(0.0, 0.0, 0.0, 0);
		glClearDepth(100.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		// Set up the camera
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(PREVIEW_FOV, 1, 1.0, 100);
		
		glMatrixMode(GL_MODELVIEW);
		
		// Set up the lights
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
		
		GLfloat l0Dif[] = { 1.0, 1.0, 1.0, 1.0 };
		GLfloat l0Pos[] = { 1.0, 1.0, 3.0, 1.0 };
		
		glLightfv(GL_LIGHT0, GL_DIFFUSE, l0Dif);
		glLightfv(GL_LIGHT0, GL_POSITION, l0Pos);
	}		
	
}

// Get the value from the selected column

std::string ModelSelector::getSelectedValue(gint colNum) {

	// Get the selection

	GtkTreeIter iter;
	GtkTreeModel* model;

	if (gtk_tree_selection_get_selected(_selection, &model, &iter)) {
		// Get the value
		GValue val = {0, 0};
		gtk_tree_model_get_value(model, &iter, colNum, &val);
		// Get the string
		return g_value_get_string(&val);	
	}
	else {
		// Nothing selected, return empty string
		return "";
	}
	
}

// Update the info table and model preview based on the current selection

void ModelSelector::updateSelected() {

	// Prepare to populate the info table
	gtk_list_store_clear(_infoStore);
	GtkTreeIter iter;
	
	// Get the model name, if this is blank we are looking at a directory,
	// so leave the table empty
	std::string mName = getSelectedValue(FULLNAME_COLUMN);
	if (mName.empty())
		return;
	
	// Update the previewed model
	
	ModelLoader* loader = ModelLoader_forType(os::getExtension(mName).c_str());
	if (loader != NULL) {
		_model = loader->loadModelFromPath(mName);
	}
	
	gtk_widget_queue_draw(_widget);
	
	// Update the text in the info table
	
	gtk_list_store_append(_infoStore, &iter);
	gtk_list_store_set(_infoStore, &iter, 
					   0, "Model name",
					   1, mName.c_str(),
					   -1);
					   
	gtk_list_store_append(_infoStore, &iter);
	gtk_list_store_set(_infoStore, &iter,
					   0, "Material surfaces",
					   1, boost::lexical_cast<std::string>(_model->getSurfaceCount()).c_str(),
					   -1);

	gtk_list_store_append(_infoStore, &iter);
	gtk_list_store_set(_infoStore, &iter,
					   0, "Total vertices",
					   1, boost::lexical_cast<std::string>(_model->getVertexCount()).c_str(),
					   -1);

	gtk_list_store_append(_infoStore, &iter);
	gtk_list_store_set(_infoStore, &iter,
					   0, "Total polys",
					   1, boost::lexical_cast<std::string>(_model->getPolyCount()).c_str(),
					   -1);

}

/* GTK CALLBACKS */

void ModelSelector::callbackHide(GtkWidget* widget, GdkEvent* ev, ModelSelector* self) {
	gtk_main_quit(); // exit recursive main loop
	gtk_widget_hide(self->_widget);
}

void ModelSelector::callbackSelChanged(GtkWidget* widget, ModelSelector* self) {
	self->updateSelected();
}

void ModelSelector::callbackOK(GtkWidget* widget, ModelSelector* self) {
	// Remember the selected model then exit from the recursive main loop
	self->_lastModel = self->getSelectedValue(FULLNAME_COLUMN);
	gtk_main_quit();
	gtk_widget_hide(self->_widget);
}

void ModelSelector::callbackCancel(GtkWidget* widget, ModelSelector* self) {
	self->_lastModel = "";
	gtk_main_quit();
	gtk_widget_hide(self->_widget);
}

void ModelSelector::callbackGLDraw(GtkWidget* widget, GdkEventExpose* ev, ModelSelector* self) {
	if (glwidget_make_current(widget) != FALSE) {

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Push the current rotation matrix, then premultiply with the
		// view transformation. We must keep the translation separate so
		// it does not get mixed up with the incremental rotations.

		glPushMatrix();

		GLfloat curMv[16];
		glGetFloatv(GL_MODELVIEW_MATRIX, curMv); // store current modelview
		
		glLoadIdentity();
		glTranslatef(0, 0, self->_camDist); // construct translation
		glMultMatrixf(curMv); // post multiply with previous

		// Render the actual model.
		if (self->_model.get() != NULL)
			self->_model->render(RENDER_DEFAULT);

		// Pop back to rotation-only matrix
		glPopMatrix(); 
		
		// Swap buffers to display the result in GTK
		glwidget_swap_buffers(widget);
	}	
}

void ModelSelector::callbackGLMotion(GtkWidget* widget, GdkEventMotion* ev, ModelSelector* self) {
	if (ev->state & GDK_BUTTON1_MASK) { // dragging with mouse button
		static gdouble _lastX = ev->x;
		static gdouble _lastY = ev->y;

		// Calculate the mouse delta as a vector in the XY plane, and store the
		// current position for the next event.
		Vector3 deltaPos(ev->x - _lastX,
						 _lastY - ev->y,
						 0);
		_lastX = ev->x;
		_lastY = ev->y;
		
		// Calculate the axis of rotation. This is the mouse vector crossed with the Z axis,
		// to give a rotation axis in the XY plane at right-angles to the mouse delta.
		static Vector3 _zAxis(0, 0, 1);
		Vector3 axisRot = deltaPos.crossProduct(_zAxis);
		
		// Grab the GL widget, and update the modelview matrix with the additional
		// rotation (TODO: may not be the best way to do this).
		if (glwidget_make_current(widget) != FALSE) {

			// Premultiply the current modelview matrix with the rotation,
			// in order to achieve rotations in eye space rather than object
			// space.
			GLfloat curMv[16];
			glGetFloatv(GL_MODELVIEW_MATRIX, curMv);

			glLoadIdentity();
			glRotatef(-2, axisRot.x(), axisRot.y(), axisRot.z());
			glMultMatrixf(curMv);

			gtk_widget_queue_draw(widget); // trigger the GLDraw method to draw the actual model
		}
		
	}
}

void ModelSelector::callbackGLScroll(GtkWidget* widget, GdkEventScroll* ev, ModelSelector* self) {
	if (ev->direction == GDK_SCROLL_UP)
		self->_camDist += 1.0f;
	else if (ev->direction == GDK_SCROLL_DOWN)
		self->_camDist -= 1.0f;
	gtk_widget_queue_draw(widget);
}


} // namespace ui
