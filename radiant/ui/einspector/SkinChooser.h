#ifndef SKINCHOOSER_H_
#define SKINCHOOSER_H_

#include "modelskin.h"

#include "iradiant.h"
#include "ui/common/ModelPreview.h"

#include <gtk/gtkwidget.h>
#include <gtk/gtktreestore.h>
#include <gtk/gtktreeselection.h>

#include <string>

namespace ui
{

class SkinChooser;
typedef boost::shared_ptr<SkinChooser> SkinChooserPtr;

/** Dialog to allow selection of skins for a model entity. Skins are grouped
 * into two toplevel categories - matching skins which are associated with the
 * model, and all skins available.
 */
class SkinChooser :
	public RadiantEventListener
{
	// Main dialog widget
	GtkWidget* _widget;
	
	// Tree store, view and selection
	GtkWidget* _treeView;
	GtkTreeStore* _treeStore;
	GtkTreeSelection* _selection;
	
	// The model name to use for skin matching
	std::string _model;
	
	// The last skin selected, and the original (previous) skin
	std::string _lastSkin;
	std::string _prevSkin;

	// Model preview widget
	ModelPreviewPtr _preview;

private:

	// Constructor creates GTK widgets
	SkinChooser();
	
	// Widget creation functions
	GtkWidget* createTreeView(gint width);
	GtkWidget* createPreview(gint size);
	GtkWidget* createButtons();
	
	// Show the dialog and block until selection is made
	std::string showAndBlock(const std::string& model, const std::string& prev);
	
	// Populate the tree with skins
	void populateSkins();
	
	// GTK callbacks
	static void _onOK(GtkWidget*, SkinChooser*);
	static void _onCancel(GtkWidget*, SkinChooser*);
	static void _onSelChanged(GtkWidget*, SkinChooser*);
	static bool _onCloseButton(GtkWidget*, SkinChooser*);
	
	// Contains the static instance
	static SkinChooser& Instance();

	// This is where the static shared_ptr of the singleton instance is held.
	static SkinChooserPtr& InstancePtr();

public:

	// RadiantEventListener implementation
	void onRadiantShutdown();

	/** Display the dialog and return the skin chosen by the user, or an empty
	 * string if no selection was made. This static method enters are recursive 
	 * GTK main loop during skin selection.
	 * 
	 * @param model
	 * The full VFS path of the model for which matching skins should be found.
	 * 
	 * @param prevSkin
	 * The current skin set on the model, so that the original can be returned
	 * if the dialog is cancelled.
	 */
	static std::string chooseSkin(const std::string& model,
								  const std::string& prevSkin);

};

}

#endif /*SKINCHOOSER_H_*/
