#pragma once

#include <map>
#include "icommandsystem.h"
#include "iregistry.h"
#include "iradiant.h"
#include "wxutil/window/TransientWindow.h"
#include "ui/common/ShaderChooser.h"

#include <sigc++/connection.h>
#include <sigc++/trackable.h>
#include <memory>

namespace wxutil { class ControlButton; }

// Forward declarations to decrease compile times
class wxTextCtrl;
class wxBitmapButton;
class wxFlexGridSizer;
class wxSpinCtrlDouble;
class wxButton;
class wxToggleButton;
class wxStaticText;

namespace ui
{

class SurfaceInspector;
typedef std::shared_ptr<SurfaceInspector> SurfaceInspectorPtr;

/// Inspector for properties of a surface and its applied texture
class SurfaceInspector : 
	public wxutil::TransientWindow,
	public sigc::trackable
{
	struct ManipulatorRow
	{
		wxTextCtrl* value;
		wxutil::ControlButton* smaller;
		wxutil::ControlButton* larger;
		wxTextCtrl* stepEntry;
	};

	// This are the named manipulator rows (shift, scale, rotation, etc)
	typedef std::map<std::string, ManipulatorRow> ManipulatorMap;
	ManipulatorMap _manipulators;

	// The "shader" entry field
	wxTextCtrl* _shaderEntry;
	wxBitmapButton* _selectShaderButton;

	struct FitTextureWidgets
	{
		wxStaticText* label;
		wxStaticText* x;
		wxButton* button;
		wxSpinCtrlDouble* width;
		wxSpinCtrlDouble* height;
	} _fitTexture;

	struct FlipTextureWidgets
	{
		wxStaticText* label;
		wxButton* flipX;
		wxButton* flipY;
	} _flipTexture;

	struct AlignTextureWidgets
	{
		wxStaticText* label;
		wxButton* top;
		wxButton* bottom;
		wxButton* left;
		wxButton* right;
	} _alignTexture;

	struct ModifyTextureWidgets
	{
		wxStaticText* label;
		wxButton* natural;
		wxButton* normalise;
	} _modifyTex;

	wxSpinCtrlDouble* _defaultTexScale;
	wxToggleButton* _texLockButton;

	// To avoid key changed loopbacks when the registry is updated
	bool _callbackActive;

	bool _updateNeeded;

	sigc::connection _brushFaceShaderChanged;
	sigc::connection _faceTexDefChanged;
	sigc::connection _patchTextureChanged;
	sigc::connection _selectionChanged;
	sigc::connection _undoHandler;
	sigc::connection _redoHandler;

public:

	// Constructor
	SurfaceInspector();

	/// Get the singletone instance
    static SurfaceInspector& Instance();

    /// Update the instance if it exists, otherwise do nothing
    static void update();

	/** greebo: Gets called when the default texscale registry key changes
	 */
	void keyChanged();

	// Command target to toggle the dialog
	static void toggle(const cmd::ArgumentList& args);

	void onRadiantShutdown();

private:
	void doUpdate();

	// This is where the static shared_ptr of the singleton instance is held.
	static SurfaceInspectorPtr& InstancePtr();

	// TransientWindow events
	void _preShow();
	void _postShow();
	void _preHide();

	/** greebo: Creates a row consisting of label, value entry,
	 * two arrow buttons and a step entry field.
	 *
	 * @table: the sizer the row should be packed into.
	 *
	 * @returns: the structure containing the widget pointers.
	 */
	ManipulatorRow createManipulatorRow(wxWindow* parent,
		const std::string& label, wxFlexGridSizer* table, bool vertical);

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

	// Executes the fit command for the selection
	void fitTexture();

	// The callback when the "select shader" button is pressed, opens the ShaderChooser dialog
	void onShaderSelect(wxCommandEvent& ev);

	// The callback for the Fit Texture button
	void onFit(wxCommandEvent& ev);

	// If any of the control button get clicked, an update is performed
	void onUpdateAfterButtonClick(wxCommandEvent& ev);

	// The keypress handler for catching the Enter key when in the shader entry field
	void onShaderEntryActivate(wxCommandEvent& ev);

	// The keypress handler for catching the Enter key when in the value entry fields
	void onValueEntryActivate(wxCommandEvent& ev);

	// Called by wxWidgets when the system is idle
	void onIdle(wxIdleEvent& ev);

}; // class SurfaceInspector

} // namespace ui