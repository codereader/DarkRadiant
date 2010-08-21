#ifndef SURFACEINSPECTOR_H_
#define SURFACEINSPECTOR_H_

#include <map>
#include "icommandsystem.h"
#include "iselection.h"
#include "iregistry.h"
#include "iundo.h"
#include "iradiant.h"
#include "gtkutil/WindowPosition.h"
#include "gtkutil/RegistryConnector.h"
#include "gtkutil/event/SingleIdleCallback.h"
#include "ui/common/ShaderChooser.h"
#include "gtkutil/window/PersistentTransientWindow.h"

#include <boost/shared_ptr.hpp>

namespace gtkutil { class ControlButton; }

// Forward declarations to decrease compile times
namespace Gtk
{
	class SpinButton;
	class Label;
	class Entry;
	class HBox;
	class Button;
	class ToggleButton;
	class Table;
}

namespace ui
{

class SurfaceInspector;
typedef boost::shared_ptr<SurfaceInspector> SurfaceInspectorPtr;

class SurfaceInspector 
: public gtkutil::PersistentTransientWindow,
  public RegistryKeyObserver,
  public SelectionSystem::Observer,
  public ShaderChooser::ChooserClient,
  public RadiantEventListener,
  public UndoSystem::Observer,
  public gtkutil::SingleIdleCallback
{
	struct ManipulatorRow
	{
		Gtk::HBox* hbox;
		Gtk::Label* label;
		Gtk::Entry* value;
		gtkutil::ControlButton* smaller; 
		gtkutil::ControlButton* larger;
		Gtk::Entry* step;
		Gtk::Label* steplabel;
	};

	// This are the named manipulator rows (shift, scale, rotation, etc) 
	typedef std::map<std::string, ManipulatorRow> ManipulatorMap;
	ManipulatorMap _manipulators;

	// The "shader" entry field
	Gtk::Entry* _shaderEntry;
	Gtk::Button* _selectShaderButton;
		
	struct FitTextureWidgets
	{
		Gtk::HBox* hbox;
		Gtk::Adjustment* widthAdj;
		Gtk::Adjustment* heightAdj;
		Gtk::SpinButton* width;
		Gtk::SpinButton* height;
		Gtk::Button* button;
		Gtk::Label* label;
	} _fitTexture;

	struct FlipTextureWidgets {
		Gtk::HBox* hbox;
		Gtk::Button* flipX;
		Gtk::Button* flipY;
		Gtk::Label* label;
	} _flipTexture;

	struct AlignTextureWidgets {
		Gtk::HBox* hbox;
		Gtk::Button* top;
		Gtk::Button* bottom;
		Gtk::Button* left;
		Gtk::Button* right;
		Gtk::Label* label;
	} _alignTexture;
	
	struct ApplyTextureWidgets {
		Gtk::HBox* hbox;
		Gtk::Label* label;
		Gtk::Button* natural;
		Gtk::Button* normalise;
	} _applyTex;
	
	Gtk::SpinButton* _defaultTexScale;
	Gtk::ToggleButton* _texLockButton;
	
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
	void queueUpdate();
	
	/** greebo: Gets called upon shader selection change (during ShaderChooser display)
	 */
	void shaderSelectionChanged(const std::string& shader);

	// Command target to toggle the dialog
	static void toggle(const cmd::ArgumentList& args);
	
	/** 
	 * greebo: RadiantEventListener implementation: Some sort of 
	 * "soft" destructor that de-registers this class from the 
	 * SelectionSystem, saves the window state, etc.
	 */
	void onRadiantShutdown();

	// UndoSystem::Observer implementation
	void postUndo();
	void postRedo();

	// Idle callback, used for deferred updates
	void onGtkIdle();

private:
	// Updates the widgets (this is private, use queueUpdate() instead)
	void update();

	// This is where the static shared_ptr of the singleton instance is held.
	static SurfaceInspectorPtr& InstancePtr();
	
	// TransientWindow events
	void _preShow();
	void _postShow();
	void _preHide();
	
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
										Gtk::Table& table, 
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
	void onShaderSelect();
	
	// Gets called when the step entry fields get changed
	void onStepChanged();
	
	// Gets called when the value entry field is changed (shift/scale/rotation) - emits the texcoords
	void onDefaultScaleChanged();
	
	// The callback for the Fit Texture button
	void onFit();
	void doUpdate();
	
	// The keypress handler for catching the Enter key when in the shader entry field
	bool onKeyPress(GdkEventKey* ev);
	
	// The keypress handler for catching the Enter key when in the value entry fields
	bool onValueKeyPress(GdkEventKey* ev);

}; // class SurfaceInspector

} // namespace ui

#endif /*SURFACEINSPECTOR_H_*/
