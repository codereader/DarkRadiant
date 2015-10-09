#pragma once

#include <wx/frame.h>
#include <wx/gauge.h>

#include <string>
#include <memory>
#include <sigc++/trackable.h>

namespace ui
{

class wxImagePanel;

class Splash :
	public wxFrame,
    public sigc::trackable
{
#if !defined(__linux__)
private:
	wxGauge* _progressBar;
	wxImagePanel* _imagePanel;

    // Private constructor, creates all the widgets
    Splash();
#endif

public:
	/** greebo: Sets the text and/or progress of the progress bar.
	 */
	void setText(const std::string& text);
	void setProgress(float fraction);
	void setProgressAndText(const std::string& text, float fraction);

	static Splash& Instance();

#if !defined(__linux__)
private:
	void createProgressBar();

	/** greebo: Triggers a redraw of the splash screen
	 */
	void queueDraw();
#endif
};

} // namespace ui
