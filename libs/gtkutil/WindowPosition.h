#ifndef WINDOWPOSITION_H_
#define WINDOWPOSITION_H_

#include "gtkmm/window.h"
#include "math/Vector2.h"
#include <string>

/* greebo: A WindowPosition object keeps track of the window's size and position.
 *
 * Use the connect() method to connect a GtkWindow to this object.
 *
 * Use the loadFromNode() and saveToNode() methods to save the internal
 * size info into the given xml::Node
 *
 * This is used by the XYWnd classes to save/restore the window state upon restart.
 */
namespace gtkutil
{

class WindowPosition
{
public:
	typedef BasicVector2<int> Position;
	typedef BasicVector2<int> Size;

private:
	// The size and position of this object
	Position _position;
	Size _size;

	// The connected window
	Gtk::Window* _window;

public:
	WindowPosition();

	// Connect the passed window to this object
	void connect(Gtk::Window* window);

	const Position& getPosition() const;
	const Size& getSize() const;

	void setPosition(int x, int y);
	void setSize(int width, int height);

	// Loads/saves the window position to the given Registry path
	void saveToPath(const std::string& path);
	void loadFromPath(const std::string& path);

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

	// Adjusts the position/dimensions to fit on the given screen (Gdk::Rectangle)
	void fitToScreen(Gdk::Rectangle screen, float xfraction = 1, float yfraction = 1);

private:
	// The GTKmm callback that gets invoked on window size/position changes
	bool onConfigure(GdkEventConfigure* ev);

}; // class WindowPosition

} // namespace gtkutil

#endif /*WINDOWPOSITION_H_*/
