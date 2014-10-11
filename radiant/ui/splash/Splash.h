#pragma once

#include <wx/frame.h>
#include <wx/gauge.h>

#include <string>
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
