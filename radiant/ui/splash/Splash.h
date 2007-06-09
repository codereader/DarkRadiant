#ifndef SPLASH_H_
#define SPLASH_H_

#include <string>

typedef struct _GtkWindow GtkWindow;
typedef struct _GtkWidget GtkWidget;

namespace ui {

class Splash
{
	// The top-level widget
	GtkWindow* _window;
	GtkWidget* _progressBar;
	GtkWidget* _vbox;
public:
	// Constructor, creates all the widgets
	Splash();
	
	// Shows/hides the splash window
	void show();
	void hide();
	
	// Returns the widget, used to set other windows transient for the splash
	GtkWindow* getWindow();
	
	/** greebo: Sets the text of the progress bar. 
	 */
	void setText(const std::string& text);
	
	// Accessor method
	static Splash& Instance();
};

} // namespace ui

#endif /*SPLASH_H_*/
