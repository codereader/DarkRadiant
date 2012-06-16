#include "GLWidget.h"

#include "igl.h"
#include "itextstream.h"

#include <gtkmm/gl/widget.h>
#include <gtkmm/container.h>

// OpenGL widget based on GtkGLExtmm
namespace gtkutil
{

typedef int* attribs_t;
struct config_t
{
  const char* name;
  attribs_t attribs;
};
typedef const config_t* configs_iterator;

int config_rgba32[] = {
  Gdk::GL::RGBA,
  Gdk::GL::DOUBLEBUFFER,
  Gdk::GL::BUFFER_SIZE, 24,
  Gdk::GL::ATTRIB_LIST_NONE,
};

int config_rgba[] = {
  Gdk::GL::RGBA,
  Gdk::GL::DOUBLEBUFFER,
  Gdk::GL::BUFFER_SIZE, 16,
  Gdk::GL::ATTRIB_LIST_NONE,
};

const config_t configs[] = {
  {
    "colour-buffer = 32bpp, depth-buffer = none",
    config_rgba32,
  },
  {
    "colour-buffer = 16bpp, depth-buffer = none",
    config_rgba,
  }
};

int config_rgba32_depth32[] = {
  Gdk::GL::RGBA,
  Gdk::GL::DOUBLEBUFFER,
  Gdk::GL::BUFFER_SIZE, 24,
  Gdk::GL::DEPTH_SIZE, 32,
  Gdk::GL::ATTRIB_LIST_NONE,
};

int config_rgba32_depth24[] = {
  Gdk::GL::RGBA,
  Gdk::GL::DOUBLEBUFFER,
  Gdk::GL::BUFFER_SIZE, 24,
  Gdk::GL::DEPTH_SIZE, 24,
  Gdk::GL::ATTRIB_LIST_NONE,
};

int config_rgba32_depth16[] = {
  Gdk::GL::RGBA,
  Gdk::GL::DOUBLEBUFFER,
  Gdk::GL::BUFFER_SIZE, 24,
  Gdk::GL::DEPTH_SIZE, 16,
  Gdk::GL::ATTRIB_LIST_NONE,
};

int config_rgba32_depth[] = {
  Gdk::GL::RGBA,
  Gdk::GL::DOUBLEBUFFER,
  Gdk::GL::BUFFER_SIZE, 24,
  Gdk::GL::DEPTH_SIZE, 1,
  Gdk::GL::ATTRIB_LIST_NONE,
};

int config_rgba_depth16[] = {
  Gdk::GL::RGBA,
  Gdk::GL::DOUBLEBUFFER,
  Gdk::GL::BUFFER_SIZE, 16,
  Gdk::GL::DEPTH_SIZE, 16,
  Gdk::GL::ATTRIB_LIST_NONE,
};

int config_rgba_depth[] = {
  Gdk::GL::RGBA,
  Gdk::GL::DOUBLEBUFFER,
  Gdk::GL::BUFFER_SIZE, 16,
  Gdk::GL::DEPTH_SIZE, 1,
  Gdk::GL::ATTRIB_LIST_NONE,
};

const config_t configs_with_depth[] =
{
  {
    "colour-buffer = 32bpp, depth-buffer = 32bpp",
    config_rgba32_depth32,
  },
  {
    "colour-buffer = 32bpp, depth-buffer = 24bpp",
    config_rgba32_depth24,
  },
  {
    "colour-buffer = 32bpp, depth-buffer = 16bpp",
    config_rgba32_depth16,
  },
  {
    "colour-buffer = 32bpp, depth-buffer = auto",
    config_rgba32_depth,
  },
  {
    "colour-buffer = 16bpp, depth-buffer = 16bpp",
    config_rgba_depth16,
  },
  {
    "colour-buffer = auto, depth-buffer = auto",
    config_rgba_depth,
  },
};

// Constructor, pass TRUE to enable depth-buffering
GLWidget::GLWidget(bool zBuffer, const std::string& debugName) :
	Gtk::GL::DrawingArea(),
	_zBuffer(zBuffer)
{
#ifdef DEBUG_GL_WIDGETS
    std::cout << "GLWidget: constructed with name '" << debugName << "'"
              << std::endl;
#endif

    // Name the widget
    if (!debugName.empty())
    {
		set_name(debugName);
    }

    // Connect to signals
	signal_hierarchy_changed().connect(
        sigc::mem_fun(*this, &GLWidget::onHierarchyChanged)
    );
	signal_realize().connect(
        sigc::bind(
            sigc::mem_fun(GlobalOpenGL(), &OpenGLBinding::registerGLWidget),
            this
        )
    );
	signal_unrealize().connect(
        sigc::bind(
            sigc::mem_fun(GlobalOpenGL(), &OpenGLBinding::unregisterGLWidget),
            this
        )
    );
}

Glib::RefPtr<Gdk::GL::Config> GLWidget::createGLConfigWithDepth()
{
	Glib::RefPtr<Gdk::GL::Config> glconfig;

	for (configs_iterator i = configs_with_depth, end = configs_with_depth + 6;
		 i != end; ++i)
	{
		glconfig = Gdk::GL::Config::create(i->attribs);

		if (glconfig)
		{
			rMessage() << "OpenGL window configuration: " << i->name << std::endl;
			return glconfig;
		}
	}

	rMessage() << "OpenGL window configuration: colour-buffer = auto, depth-buffer = auto (fallback)\n";
	return Gdk::GL::Config::create(Gdk::GL::MODE_RGBA | Gdk::GL::MODE_DOUBLE | Gdk::GL::MODE_DEPTH);
}

Glib::RefPtr<Gdk::GL::Config>GLWidget::createGLConfig()
{
	Glib::RefPtr<Gdk::GL::Config> glconfig;

	for (configs_iterator i = configs, end = configs + 2; i != end; ++i)
	{
		glconfig = Gdk::GL::Config::create(i->attribs);

		if (glconfig)
		{
			rMessage() << "OpenGL window configuration: " << i->name << std::endl;
			return glconfig;
		}
	}

	rMessage() << "OpenGL window configuration: colour-buffer = auto, depth-buffer = none\n";
	return Gdk::GL::Config::create(Gdk::GL::MODE_RGBA | Gdk::GL::MODE_DOUBLE);
}

bool GLWidget::makeCurrent()
{
#ifdef DEBUG_GL_WIDGETS
    std::cout << "GLWidget: widget '" << get_name()
              << "' made current." << std::endl;
#endif

    return get_gl_drawable()->make_current(get_gl_context());
}

void GLWidget::swapBuffers()
{
#ifdef DEBUG_GL_WIDGETS
    std::cout << "GLWidget: widget '" << get_name()
              << "' swapped buffers." << std::endl;
#endif

    get_gl_drawable()->gl_end();
	get_gl_drawable()->swap_buffers();
}

void GLWidget::onHierarchyChanged(Gtk::Widget* previous_toplevel)
{
	if (previous_toplevel == NULL && !is_gl_capable())
    {
		// Create a new GL config structure
		Glib::RefPtr<Gdk::GL::Config> glconfig = _zBuffer
                                               ? createGLConfigWithDepth()
                                               : createGLConfig();
		assert(glconfig);

		Gtk::Widget* context = GlobalOpenGL().getGLContextWidget();

		set_gl_capability(
			glconfig,
			context ? Gtk::GL::widget_get_gl_context(*context) : Glib::RefPtr<Gdk::GL::Context>(),
			true,
			Gdk::GL::RGBA_TYPE
		);

		realize();
	}
}

} // namespace gtkutil
