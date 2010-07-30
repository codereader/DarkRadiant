#include "TexturePreviewCombo.h"

#include "i18n.h"
#include "gtkutil/GLWidgetSentry.h"
#include "gtkutil/ScrolledFrame.h"
#include "gtkutil/StockIconMenuItem.h"
#include "gtkutil/TextColumn.h"

#include "ishaders.h"
#include "texturelib.h"

#include "ShaderSelector.h"

#include <GL/glew.h>
#include <boost/bind.hpp>

#include <gtkmm/treeview.h>
#include <gtkmm/frame.h>
#include <gtkmm/clipboard.h>

namespace ui
{

// Constructor. Create GTK widgets.

TexturePreviewCombo::TexturePreviewCombo() :
	Gtk::HBox(false, 0),
	_glWidget(new gtkutil::GLWidget(false, "TexturePreviewCombo")),
	_texName(""),
	_infoStore(Gtk::ListStore::create(_infoStoreColumns)),
	_infoView(Gtk::manage(new Gtk::TreeView(_infoStore))),
	_contextMenu(GTK_WIDGET(_infoView->gobj()))
{
	// Set up the GL preview widget
	GtkWidget* glWidgetLegacy = *_glWidget; // cast to GtkWidget

	Gtk::Widget* glWidget = Glib::wrap(glWidgetLegacy, true);
	glWidget->set_size_request(128, 128);
	glWidget->signal_expose_event().connect(sigc::mem_fun(*this, &TexturePreviewCombo::_onExpose));
	
	Gtk::Frame* glFrame = Gtk::manage(new Gtk::Frame);
	glFrame->add(*glWidget);
	pack_start(*glFrame, false, false, 0);
	
	// Set up the info table
	_infoView->set_headers_visible(false);

	_infoView->append_column(*Gtk::manage(new gtkutil::TextColumnmm(_("Attribute"), _infoStoreColumns.attribute)));
	_infoView->append_column(*Gtk::manage(new gtkutil::TextColumnmm(_("Value"), _infoStoreColumns.value)));

	// Pack into main widget
	pack_start(*Gtk::manage(new gtkutil::ScrolledFramemm(*_infoView)), true, true, 0);
	
	// Construct the context menu
	_contextMenu.addItem(
		gtkutil::StockIconMenuItem(GTK_STOCK_COPY, _("Copy shader name")),
		boost::bind(&TexturePreviewCombo::_onCopyTexName, this)
	);
}

TexturePreviewCombo::~TexturePreviewCombo()
{
	_glWidget.reset();	
}

// Update the selected texture

void TexturePreviewCombo::setTexture(const std::string& tex)
{
	_texName = tex;
	refreshInfoTable();
	_glWidget->queueDraw();
}

// Refresh the info table

void TexturePreviewCombo::refreshInfoTable()
{
	// Prepare the list
	_infoStore->clear();

	// Other properties require a valid shader name	
	if (_texName.empty())
	{
		return;
	}

	MaterialPtr shader = GlobalMaterialManager().getMaterialForName(_texName);
	ShaderSelector::displayShaderInfo(shader, _infoStore);
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
	GtkWidget* glWidgetLegacy = *_glWidget;

	// Grab the GLWidget with sentry
	gtkutil::GLWidgetSentry sentry(glWidgetLegacy);
	
	// Get the viewport size from the GL widget
	GtkRequisition req;
	gtk_widget_size_request(glWidgetLegacy, &req);
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
