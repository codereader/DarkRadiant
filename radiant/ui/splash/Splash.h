#ifndef SPLASH_H_
#define SPLASH_H_

typedef struct _GtkWindow GtkWindow;

namespace ui {

class Splash
{
	// The top-level widget
	GtkWindow* _window;
public:
	// Constructor, creates all the widgets
	Splash();
	
	// Shows/hides the splash window
	void show();
	void hide();
	
	// Returns the widget, used to set other windows transient for the splash
	GtkWindow* getWindow();
	
	// Accessor method
	static Splash& Instance();
};

} // namespace ui

#endif /*SPLASH_H_*/
