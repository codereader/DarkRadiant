#ifndef SURFACEINSPECTOR_H_
#define SURFACEINSPECTOR_H_

#include <map>
#include "icommandsystem.h"
#include "iselection.h"
#include "iregistry.h"
#include "iradiant.h"
#include "gtkutil/WindowPosition.h"
#include "gtkutil/RegistryConnector.h"
#include "ui/common/ShaderChooser.h"
#include "gtkutil/window/PersistentTransientWindow.h"

#include <boost/shared_ptr.hpp>

// Forward declarations to decrease compile times
typedef struct _GtkSpinButton GtkSpinButton;
typedef struct _GtkEditable GtkEditable;
typedef struct _GtkTable GtkTable;
typedef struct _GtkWidget GtkWidget;
namespace gtkutil { class ControlButton; }

namespace ui {

class SurfaceInspector;
typedef boost::shared_ptr<SurfaceInspector> SurfaceInspectorPtr;

class SurfaceInspector 
: public gtkutil::PersistentTransientWindow,
  public RegistryKeyObserver,
  public SelectionSystem::Observer,
  public ShaderChooser::ChooserClient,
  public RadiantEventListener
{
	typedef boost::shared_ptr<gtkutil::ControlButton> ControlButtonPtr;

	struct ManipulatorRow {
		GtkWidget* hbox;
		GtkWidget* label;
		GtkWidget* value;
		gulong valueChangedHandler;
		ControlButtonPtr smaller; 
		ControlButtonPtr larger;
		GtkWidget* step;
		GtkWidget* steplabel;
	};

	// This are the named manipulator rows (shift, scale, rotation, etc) 
	typedef std::map<std::string, ManipulatorRow> ManipulatorMap;
	ManipulatorMap _manipulators;

	// The "shader" entry field
	GtkWidget* _shaderEntry;
	GtkWidget* _selectShaderButton;
		
	struct FitTextureWidgets {
		GtkWidget* hbox;
		GtkObject* widthAdj;
		GtkObject* heightAdj;
		GtkWidget* width;
		GtkWidget* height;
		GtkWidget* button;
		GtkWidget* label;
	} _fitTexture;

	struct FlipTextureWidgets {
		GtkWidget* hbox;
		GtkWidget* flipX;
		GtkWidget* flipY;
		GtkWidget* label;
	} _flipTexture;
	
	struct ApplyTextureWidgets {
		GtkWidget* hbox;
		GtkWidget* label;
		GtkWidget* natural;
		GtkWidget* normalise;
	} _applyTex;
	
	GtkWidget* _defaultTexScale;
	GtkWidget* _texLockButton;
	
	// The window position tracker
	gtkutil::WindowPosition _windowPosition;
	
	// To avoid key changed loopbacks when the registry is updated 
	bool _callbackActive;

	// This member takes care of importing/exporting Registry
	// key values from and to widgets
	gtkutil::RegistryConnector _connector;

	// A reference to the SelectionInfo structure (with the counters)
	const SelectionInfo& _selectionInfo;

public:

	// Constructor
	SurfaceInspector();
	
	/** greebo: Delivers a reference to the singleton instance using InstancePtr()
	 */
	static SurfaceInspector& Instance();

	/** greebo: Gets called when the default texscale registry key changes
	 */
	void keyChanged(const std::string& key, const std::string& val);

	/** greebo: SelectionSystem::Observer implementation. Gets called by
	 * the SelectionSystem upon selection change to allow updating of the
	 * texture properties.
	 */
	void selectionChanged(const scene::INodePtr& node, bool isComponent);
	
	// Updates the widgets
	void update();
	
	/** greebo: Gets called upon shader selection change (during ShaderChooser display)
	 */
	virtual void shaderSelectionChanged(const std::string& shader);

	// Command target to toggle the dialog
	static void toggle(const cmd::ArgumentList& args);
	
	/** greebo: RadiantEventListener implementation: Some sort of 
	 *          "soft" destructor that de-registers this class from the 
	 *          SelectionSystem, saves the window state, etc.
	 */
	virtual void onRadiantShutdown();

private:
	// This is where the static shared_ptr of the singleton instance is held.
	static SurfaceInspectorPtr& InstancePtr();
	
	// TransientWindow events
	virtual void _preShow();
	virtual void _postShow();
	virtual void _preHide();
	
	/** greebo: This toggles the visibility of the surface dialog.
	 * The dialog is constructed only once and never destructed 
	 * during runtime.
	 */
	void toggleWindow();

	/** greebo: Creates a row consisting of label, value entry,
	 * two arrow buttons and a step entry field.
	 * 
	 * @table: the GtkTable the row should be packed into.
	 * @row: the target row number (first table row = 0).
	 * 
	 * @returns: the structure containing the widget pointers. 
	 */
	ManipulatorRow createManipulatorRow(const std::string& label, 
										GtkTable* table, 
										int row,
										bool vertical);

	// Adds all the widgets to the window
	void populateWindow();
	
	// Connect IEvents to the widgets 
	void connectEvents();
	
	// Updates the texture shift/scale/rotation fields
	void updateTexDef();
	
	// The counter-part of updateTexDef() - emits the TexCoords to the selection
	void emitTexDef();
	
	// Applies the entered shader to the current selection
	void emitShader();
	
	// Saves the connected widget content into the registry
	void saveToRegistry();
	
	// Executes the fit command for the selection 
	void fitTexture();
	
	// The callback when the "select shader" button is pressed, opens the ShaderChooser dialog
	static void onShaderSelect(GtkWidget* button, SurfaceInspector* self);
	
	// Gets called when the step entry fields get changed
	static void onStepChanged(GtkEditable* editable, SurfaceInspector* self);
	
	// Gets called when the value entry field is changed (shift/scale/rotation) - emits the texcoords
	static gboolean onDefaultScaleChanged(GtkSpinButton* spinbutton, SurfaceInspector* self);
	
	// The callback for the Fit Texture button
	static gboolean onFit(GtkWidget* widget, SurfaceInspector* self);
	static gboolean doUpdate(GtkWidget* widget, SurfaceInspector* self);
	
	// The keypress handler for catching the Enter key when in the shader entry field
	static gboolean onKeyPress(GtkWidget* entry, GdkEventKey* event, SurfaceInspector* self);
	
	// The keypress handler for catching the Enter key when in the value entry fields
	static gboolean onValueKeyPress(GtkWidget* entry, GdkEventKey* event, SurfaceInspector* self);

}; // class SurfaceInspector

} // namespace ui

#endif /*SURFACEINSPECTOR_H_*/
