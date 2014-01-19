#pragma once

#include <wx/wxprec.h>

#include <string>
#include <gtkmm/window.h>
#include <boost/shared_ptr.hpp>

namespace ui
{

class Splash;
typedef Splash* SplashPtr;

class wxImagePanel;

class Splash :
	public wxFrame
{
private:
	wxGauge* _progressBar;
	wxImagePanel* _imagePanel;

public:
	// Private constructor, creates all the widgets
	Splash();

	// Shows the splash window
	void show_all();

	// Called by the mainframe to set the splash screen transient for the main window
	void setTopLevelWindow(const Glib::RefPtr<Gtk::Window>& window);

	/** greebo: Sets the text and/or progress of the progress bar.
	 */
	void setText(const std::string& text);
	void setProgress(float fraction);
	void setProgressAndText(const std::string& text, float fraction);

	// Use this static method to avoid instantiating the class just for this check
	static bool isVisible();
	static void destroy();

	static Splash& Instance();

private:
	// Accessor method
	static SplashPtr& InstancePtr();

	void createProgressBar();

	/** greebo: Triggers a redraw of the splash screen
	 */
	void queueDraw();
};

} // namespace ui
