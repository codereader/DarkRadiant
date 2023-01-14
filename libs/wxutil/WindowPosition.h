#pragma once

#include <array>
#include <string>
#include <wx/event.h>

#include "ui/iwindowstate.h"

class wxTopLevelWindow;

/**
 * greebo: A WindowPosition object keeps track of the window's size and position.
 *
 * It implements the IPersistableObject interface to save/restore the window
 * attributes to the registry.
 *
 * Use the connect() method to connect a wxTopLevelWindow to this object.
 */
namespace wxutil
{

class WindowPosition :
	public wxEvtHandler,
    public ui::IPersistableObject
{
private:
	// The size and position of this object
    std::array<int, 2> _position;
    std::array<int, 2> _size;

	// The connected window
	wxTopLevelWindow* _window;

public:
	WindowPosition();

    // All-in-one method to connect a window and load its state from the given path
    // or (if the key is not existent) set up a reasonable default position/size.
    void initialise(wxTopLevelWindow* window, const std::string& windowStateKey,
                    float defaultXFraction, float defaultYFraction);

    // All-in-one method to connect a window and load its state from the given path.
    // Default X/Y fractions will be read from the given key, otherwise a default size will be set.
    void initialise(wxTopLevelWindow* window, const std::string& windowStateKey);

	// Connect the passed window to this object
	void connect(wxTopLevelWindow* window);
	void disconnect(wxTopLevelWindow* window);

    const std::array<int, 2>& getPosition() const;
    const std::array<int, 2>& getSize() const;

	void setPosition(int x, int y);
	void setSize(int width, int height);

	// Loads/saves the window position to the given Registry path
	void saveToPath(const std::string& registryKey) override;
	void loadFromPath(const std::string& registryKey) override;

	// Applies the internally stored size/position info to the GtkWindow
	// The algorithm was adapted from original GtkRadiant code (window.h)
	void applyPosition();

	// Reads the position from the GtkWindow
	void readPosition();

	/**
	 * Fits the current position/dimension to the screen of the connected
	 * window. This object has to be connected to a GtkWindow before
	 * the method can function properly.
	 *
	 * @xfraction,yfraction: the fraction of the screen which the window
	 * should occupy. (e.g. Pass 0.5/0.66 to let the window half of the
	 * monitor width and two thirds of the monitor height.
	 *
	 * Note: applyPosition() has to be called for the changes to take effect.
	 */
	void fitToScreen(float xfraction = 1, float yfraction = 1);

	// Adjusts the position/dimensions to fit on the given screen (wxRect)
	void fitToScreen(const wxRect& screen, float xfraction = 1, float yfraction = 1);

private:
	// The callback that gets invoked on window size/position changes
	void onResize(wxSizeEvent& ev);
	void onMove(wxMoveEvent& ev);
};

} // namespace
