#include "TexturePreviewCombo.h"

#include "i18n.h"
#include "gtkutil/GLWidgetSentry.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/StockIconMenuItem.h"
#include "gtkutil/TextColumn.h"

#include "ishaders.h"
#include "texturelib.h"

#include <GL/glew.h>
#include <boost/bind.hpp>

#include <gtkmm/treeview.h>
#include <gtkmm/frame.h>
#include <gtkmm/clipboard.h>
#include <gtkmm/stock.h>

namespace ui
{

// Constructor. Create GTK widgets.

TexturePreviewCombo::TexturePreviewCombo() :
    Gtk::HBox(false, 0),
    _glWidget(Gtk::manage(new gtkutil::GLWidget(false, "TexturePreviewCombo"))),
    _texName(""),
    _contextMenu(&_infoTable)
{
    _glWidget->set_size_request(128, 128);
    _glWidget->signal_expose_event().connect(sigc::mem_fun(*this, &TexturePreviewCombo::_onExpose));

    Gtk::Frame* glFrame = Gtk::manage(new Gtk::Frame);
    glFrame->add(*_glWidget);
    pack_start(*glFrame, false, false, 0);

    // Add info table
    pack_start(*Gtk::manage(new gtkutil::ScrolledFrame(_infoTable)),
               true, true, 0);

    // Construct the context menu
    _contextMenu.addItem(
        Gtk::manage(new gtkutil::StockIconMenuItem(Gtk::Stock::COPY, _("Copy shader name"))),
        boost::bind(&TexturePreviewCombo::_onCopyTexName, this)
    );
}

TexturePreviewCombo::~TexturePreviewCombo()
{}

// Update the selected texture

void TexturePreviewCombo::setTexture(const std::string& tex)
{
    _texName = tex;
    refreshInfoTable();
    _glWidget->queue_draw();
}

// Refresh the info table

void TexturePreviewCombo::refreshInfoTable()
{
    _infoTable.clear();

    // Properties require a valid shader name
    if (_texName.empty())
    {
        return;
    }

    // Get shader info
    MaterialPtr shader = GlobalMaterialManager().getMaterialForName(_texName);
    _infoTable.append(_("Shader"), shader->getName());
    _infoTable.append(_("Defined in"),
                      Glib::ustring(shader->getShaderFileName()));
    _infoTable.append(_("Description"),
                      Glib::ustring(shader->getDescription()));
}

// Popup menu callbacks

void TexturePreviewCombo::_onCopyTexName()
{
    // Store texture on the clipboard
    Glib::RefPtr<Gtk::Clipboard> clipBoard = Gtk::Clipboard::get();
    clipBoard->set_text(_texName);
}

// GTK CALLBACKS

bool TexturePreviewCombo::_onExpose(GdkEventExpose* ev)
{
    // Grab the GLWidget with sentry
    gtkutil::GLWidgetSentry sentry(*_glWidget);

    // Get the viewport size from the GL widget
    Gtk::Requisition req = _glWidget->size_request();
    glViewport(0, 0, req.width, req.height);

    // Initialise
    glClearColor(0.3f, 0.3f, 0.3f, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, req.width, 0, req.height, -100, 100);
    glEnable (GL_TEXTURE_2D);

    // If no texture is loaded, leave window blank
    if (_texName.empty())
        return false;

    // Get a reference to the selected shader
    MaterialPtr shader = GlobalMaterialManager().getMaterialForName(_texName);

    // This is an "ordinary" texture, take the editor image
    TexturePtr tex = shader->getEditorImage();
    if (tex != NULL) {
        glBindTexture (GL_TEXTURE_2D, tex->getGLTexNum());

        // Calculate the correct aspect ratio for preview
        float aspect = float(tex->getWidth()) / float(tex->getHeight());
        float hfWidth, hfHeight;
        if (aspect > 1.0) {
            hfWidth = 0.5*req.width;
            hfHeight = 0.5*req.height / aspect;
        }
        else {
            hfHeight = 0.5*req.width;
            hfWidth = 0.5*req.height * aspect;
        }

        // Draw a quad to put the texture on
        glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
        glColor3f(1, 1, 1);
        glBegin(GL_QUADS);
        glTexCoord2i(0, 1);
        glVertex2f(0.5*req.width - hfWidth, 0.5*req.height - hfHeight);
        glTexCoord2i(1, 1);
        glVertex2f(0.5*req.width + hfWidth, 0.5*req.height - hfHeight);
        glTexCoord2i(1, 0);
        glVertex2f(0.5*req.width + hfWidth, 0.5*req.height + hfHeight);
        glTexCoord2i(0, 0);
        glVertex2f(0.5*req.width - hfWidth, 0.5*req.height + hfHeight);
        glEnd();
    }

    return false;
}

}
