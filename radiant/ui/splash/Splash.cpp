#include "Splash.h"

#include "gtkutil/LeftAlignedLabel.h"

#include <gtkmm/progressbar.h>
#include <gtkmm/image.h>
#include <gtkmm/box.h>
#include <gtkmm/main.h>

#include "modulesystem/ModuleRegistry.h"

namespace ui
{

namespace
{
	const char* const SPLASH_FILENAME = "darksplash.png";
}

Splash::Splash() :
	Gtk::Window(Gtk::WINDOW_TOPLEVEL),
	_progressBar(NULL)
{
	set_decorated(false);
	set_resizable(false);
	set_modal(true);
	set_default_size(-1, -1);
	set_position(Gtk::WIN_POS_CENTER);
	set_border_width(0);

	const ApplicationContext& ctx = module::getRegistry().getApplicationContext();
	std::string fullFileName(ctx.getBitmapsPath() + SPLASH_FILENAME);

	Gtk::Image* image = Gtk::manage(new Gtk::Image(
		Gdk::Pixbuf::create_from_file(fullFileName))
	);

	_vbox = Gtk::manage(new Gtk::VBox(false, 0));
	_vbox->pack_start(*image, true, true, 0);

	add(*_vbox);

	set_size_request(-1, -1);
}

bool Splash::isVisible()
{
	return InstancePtr() != NULL && Instance().is_visible();
}

void Splash::setTopLevelWindow(const Glib::RefPtr<Gtk::Window>& window)
{
	if (!window) return;

	Gtk::Container* toplevel = window->get_toplevel();

	if (toplevel != NULL && toplevel->is_toplevel() &&
		dynamic_cast<Gtk::Window*>(toplevel) != NULL)
	{
		set_transient_for(*static_cast<Gtk::Window*>(toplevel));
	}
}

void Splash::createProgressBar()
{
	_progressBar = Gtk::manage(new Gtk::ProgressBar);
	_vbox->pack_start(*_progressBar, false, false, 0);

	set_size_request(-1, -1);

	_progressBar->show_all();
}

void Splash::setText(const std::string& text)
{
	if (_progressBar == NULL)
	{
		createProgressBar();
	}

	_progressBar->set_text(text);
	queueDraw();
}

void Splash::setProgress(float fraction)
{
	if (_progressBar == NULL)
	{
		createProgressBar();
	}

	_progressBar->set_fraction(fraction);
	queueDraw();
}

void Splash::setProgressAndText(const std::string& text, float fraction)
{
	setText(text);
	setProgress(fraction);
}

void Splash::show_all()
{
	Gtk::Window::show_all();
	queueDraw();
}

void Splash::queueDraw()
{
	// Trigger a (re)draw, just to make sure that it gets displayed
	queue_draw();

	while (Gtk::Main::events_pending())
	{
		Gtk::Main::iteration();
	}
}

void Splash::destroy()
{
	InstancePtr().reset();
}

SplashPtr& Splash::InstancePtr()
{
	static SplashPtr _instancePtr;
	return _instancePtr;
}

Splash& Splash::Instance()
{
	SplashPtr& instancePtr = InstancePtr();

	if (instancePtr == NULL)
	{
		instancePtr.reset(new Splash);
	}

	return *instancePtr;
}

} // namespace ui
