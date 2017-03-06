#include "TexturePreviewCombo.h"

#include "i18n.h"
#include "wxutil/menu/IconTextMenuItem.h"
#include "wxutil/GLWidget.h"
#include "wxutil/KeyValueTable.h"
#include "wxutil/TreeModel.h"

#include "ishaders.h"
#include "texturelib.h"

#include <wx/sizer.h>
#include <wx/clipbrd.h>

#include <GL/glew.h>
#include <functional>

namespace ui
{

// Constructor. Create widgets.

TexturePreviewCombo::TexturePreviewCombo(wxWindow* parent) :
    wxPanel(parent, wxID_ANY),
    _glWidget(new wxutil::GLWidget(this, std::bind(&TexturePreviewCombo::_onRender, this), "TexturePreviewCombo")),
    _texName(""),
	_infoTable(NULL),
	_contextMenu(new wxutil::PopupMenu)
{
    _glWidget->SetMinSize(wxSize(128, 128));

    // Add info table
	_infoTable = new wxutil::KeyValueTable(this);
	_infoTable->Connect(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, 
		wxDataViewEventHandler(TexturePreviewCombo::_onContextMenu), NULL, this);

	SetSizer(new wxBoxSizer(wxHORIZONTAL));

	GetSizer()->Add(_glWidget, 0, wxEXPAND);
	GetSizer()->Add(_infoTable, 1, wxEXPAND);

    // Construct the context menu
    _contextMenu->addItem(
		new wxutil::StockIconTextMenuItem(_("Copy shader name"), wxART_COPY),
        std::bind(&TexturePreviewCombo::_onCopyTexName, this)
    );
}

// Update the selected texture

void TexturePreviewCombo::SetTexture(const std::string& tex)
{
    _texName = tex;
    refreshInfoTable();
    _glWidget->Refresh();
}

// Refresh the info table

void TexturePreviewCombo::refreshInfoTable()
{
    _infoTable->Clear();

    // Properties require a valid shader name
    if (_texName.empty())
    {
        return;
    }

    // Get shader info
    MaterialPtr shader = GlobalMaterialManager().getMaterialForName(_texName);

    _infoTable->Append(_("Shader"), shader->getName());
    _infoTable->Append(_("Defined in"), shader->getShaderFileName());
    _infoTable->Append(_("Description"), shader->getDescription());
    
    _infoTable->TriggerColumnSizeEvent();
}

// Popup menu callbacks

void TexturePreviewCombo::_onCopyTexName()
{
	// Store texture on the clipboard
	if (wxTheClipboard->Open())
	{
		if (wxTheClipboard->IsSupported(wxDF_UNICODETEXT))
		{
			wxTheClipboard->SetData(new wxTextDataObject(_texName));
		}

		wxTheClipboard->Close();
	}
}

void TexturePreviewCombo::_onContextMenu(wxDataViewEvent& ev)
{
	_contextMenu->show(_infoTable);
}

// CALLBACKS
void TexturePreviewCombo::_onRender()
{
	// Get the viewport size from the GL widget
	wxSize req = _glWidget->GetClientSize();

	if (req.GetWidth() == 0 || req.GetHeight() == 0) return;

	glPushAttrib(GL_ALL_ATTRIB_BITS);

	glViewport(0, 0, req.GetWidth(), req.GetHeight());

    // Initialise
    glClearColor(0.3f, 0.3f, 0.3f, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, req.GetWidth(), 0, req.GetHeight(), -100, 100);
    glEnable(GL_TEXTURE_2D);

    // If no texture is loaded, leave window blank
	if (!_texName.empty())
	{
		// Get a reference to the selected shader
		MaterialPtr shader = GlobalMaterialManager().getMaterialForName(_texName);

		// This is an "ordinary" texture, take the editor image
		TexturePtr tex = shader->getEditorImage();

		if (tex != NULL)
		{
			glBindTexture(GL_TEXTURE_2D, tex->getGLTexNum());

			// Calculate the correct aspect ratio for preview
			float aspect = float(tex->getWidth()) / float(tex->getHeight());
			float hfWidth, hfHeight;

			if (aspect > 1.0f)
			{
				hfWidth = 0.5 * req.GetWidth();
				hfHeight = 0.5 * req.GetHeight() / aspect;
			}
			else 
			{
				hfHeight = 0.5 * req.GetWidth();
				hfWidth = 0.5 * req.GetHeight() * aspect;
			}

			// Draw a quad to put the texture on
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glColor3f(1, 1, 1);

			glBegin(GL_QUADS);
			glTexCoord2i(0, 1);
			glVertex2f(0.5*req.GetWidth() - hfWidth, 0.5*req.GetHeight() - hfHeight);
			glTexCoord2i(1, 1);
			glVertex2f(0.5*req.GetWidth() + hfWidth, 0.5*req.GetHeight() - hfHeight);
			glTexCoord2i(1, 0);
			glVertex2f(0.5*req.GetWidth() + hfWidth, 0.5*req.GetHeight() + hfHeight);
			glTexCoord2i(0, 0);
			glVertex2f(0.5*req.GetWidth() - hfWidth, 0.5*req.GetHeight() + hfHeight);
			glEnd();
		}
	}

	glPopAttrib();
}

}
