#pragma once

#include <map>
#include "wxutil/FormLayout.h"
#include "messages/TextureChanged.h"

#include <sigc++/connection.h>
#include <sigc++/trackable.h>

#include "wxutil/DockablePanel.h"
#include "wxutil/event/SingleIdleCallback.h"

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

/// Inspector for properties of a surface and its applied texture
class SurfaceInspector :
	public wxutil::DockablePanel,
	public sigc::trackable,
    public wxutil::SingleIdleCallback
{
    // Manipulatable value field with nudge buttons and a step size selector
	struct ManipulatorRow
	{
		wxTextCtrl* value;
		wxutil::ControlButton* smaller;
		wxutil::ControlButton* larger;
		wxTextCtrl* stepEntry;

        // Set the text control to show the given value
        void setValue(double val);
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
		wxButton* fitButton;
        wxToggleButton* preserveAspectButton;
		wxSpinCtrlDouble* width;
		wxSpinCtrlDouble* height;

        // Set sensitivity of all widgets
        void enable(bool enabled);
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
    wxButton* _useHorizScale;
    wxToggleButton* _scaleLinkToggle;
    wxButton* _useVertScale;

	// To avoid key changed loopbacks when the registry is updated
	bool _callbackActive;

	bool _updateNeeded;

	std::size_t _textureMessageHandler;
	sigc::connection _selectionChanged;
	sigc::connection _undoHandler;
	sigc::connection _redoHandler;

public:

	SurfaceInspector(wxWindow* parent);

    ~SurfaceInspector() override;

    /// Update the instance if it exists, otherwise do nothing
    void update();

	/** greebo: Gets called when the default texscale registry key changes
	 */
	void keyChanged();

protected:
    void onIdle() override;

    void onPanelActivated() override;
    void onPanelDeactivated() override;

private:
    void connectEventHandlers();
    void disconnectEventHandlers();

	void doUpdate();

	/** greebo: Creates a row consisting of label, value entry,
	 * two arrow buttons and a step entry field.
	 *
	 * @table: the sizer the row should be packed into.
	 *
	 * @returns: the structure containing the widget pointers.
	 */
    ManipulatorRow createManipulatorRow(const std::string& label, wxutil::FormLayout& table,
                                        const std::string& bitmapSmaller,
                                        const std::string& bitmapLarger);

    // Widget construction
	void populateWindow();
    wxBoxSizer* createFitTextureRow();
    void createScaleLinkButtons(wxutil::FormLayout& table);

	// Connect IEvents to the widgets
	void connectButtons();

	// Updates the texture shift/scale/rotation fields
	void updateTexDef();

	// The counter-part of updateTexDef() - emits the TexCoords to the selection
	void emitTexDef();

	// Applies the entered shader to the current selection
	void emitShader();

    // Fit texture on one or both axes
    enum class Axis { X, Y, BOTH };
    wxSpinCtrlDouble* makeFitSpinBox(Axis axis);
	void fitTexture(Axis axis);
	void onFit(Axis axis);

	// The callback when the "select shader" button is pressed, opens the ShaderChooser dialog
	void onShaderSelect(wxCommandEvent& ev);

	// If any of the control button get clicked, an update is performed
	void onUpdateAfterButtonClick(wxCommandEvent& ev);

	// The keypress handler for catching the Enter key when in the shader entry field
	void onShaderEntryActivate(wxCommandEvent& ev);

	// The keypress handler for catching the Enter key when in the value entry fields
	void onValueEntryActivate(wxCommandEvent& ev);

	void handleTextureChangedMessage(radiant::TextureChangedMessage& msg);

    void onScale(const std::string& scaleId, bool larger);
    void onHarmoniseScale(bool useHorizontal);

}; // class SurfaceInspector

} // namespace
