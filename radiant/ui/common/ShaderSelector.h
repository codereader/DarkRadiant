#ifndef SHADERSELECTOR_H_
#define SHADERSELECTOR_H_

#include <string>
#include <vector>
#include <boost/shared_ptr.hpp>

/* FORWARD DECLS */
typedef struct _GtkListStore GtkListStore;
typedef struct _GtkWidget GtkWidget;
typedef struct _GtkTreeSelection GtkTreeSelection;
typedef struct _GdkEventExpose GdkEventExpose;
class IShader;
typedef boost::shared_ptr<IShader> IShaderPtr;

namespace ui
{

/** A widget that allows the selection of a shader. The widget contains
 * three elements - a tree view displaying available shaders as
 * identified by the specified prefixes, an OpenGL widget displaying a 
 * preview of the currently-selected shader, and a table containing certain
 * information about the shader.
 * 
 * Use the GtkWidget* operator to incorporate this class into a dialog window.
 * 
 * This widget populates its list of shaders automatically, and offers a method
 * that allows calling code to retrieve the user's selection.
 * 
 * The client class has to derive from the abstract ShaderSelector::Client class
 * providing an interface to allow the update of the info liststore.
 * 
 * Additionally the ShaderNameFunctor has to be passed in order to filter the
 * displayed shaders.
 */

class ShaderSelector
{
public:
	
	// Derive from this class to update the info widget
	class Client
	{
	public:
		/** greebo: This gets invoked upon selection changed to allow the client
		 * 			class to display custom information in the selector's liststore.  
		 */
		virtual void shaderSelectionChanged(const std::string& shader, GtkListStore* listStore) = 0;
	};
	
private:
	// Main widget container
	GtkWidget* _widget;
	
	// Tree view and selection object
	GtkWidget* _treeView;
	GtkTreeSelection* _selection;
	
	// GL preview widget
	GtkWidget* _glWidget;
	
	// List store for info table
	GtkListStore* _infoStore;
	
	// The client of this class.
	Client* _client;
	
public:
	// This is where the prefixes are stored (needed to filter the possible shaders)
	typedef std::vector<std::string> PrefixList;
	PrefixList _prefixes;

	/** Constructor.
	 * 
	 * @client: The client class that gets notified on selection change
	 * @prefixes: A comma-separated list of shader prefixes.
	 */
	ShaderSelector(Client* client, const std::string& prefixes);
	
	/** Operator cast to GtkWidget*, for packing into parent widget.
	 */
	operator GtkWidget* () {
		return _widget;
	}
	
	/** Return the shader selected by the user, or an empty string if there
	 * was no selection.
	 */
	std::string getSelection();
	
	/** Set the given shader name as the current selection, highlighting it
	 * in the tree view.
	 * 
	 * @param selection
	 * The fullname of the shader to select, or the empty string if there 
	 * should be no selection.
	 */
	void setSelection(const std::string& selection);
	
private:

	// Create GUI elements
	GtkWidget* createTreeView();
	GtkWidget* createPreview();
	
	// Update the info in the table (passes the call to the client class)
	void updateInfoTable();
	
	// Get the selected IShader
	IShaderPtr getSelectedShader();
	
	/* GTK CALLBACKS */
	static void _onExpose(GtkWidget*, GdkEventExpose*, ShaderSelector*);
	static void _onSelChange(GtkWidget*, ShaderSelector*);
};

} // namespace ui

#endif /*SHADERSELECTOR_H_*/
