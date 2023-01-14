#include "TexturePreviewCombo.h"

#include "i18n.h"
#include "wxutil/menu/IconTextMenuItem.h"
#include "wxutil/GLWidget.h"
#include "wxutil/dataview/KeyValueTable.h"
#include "gamelib.h"

#include "ishaders.h"
#include "texturelib.h"

#include <wx/sizer.h>
#include <wx/clipbrd.h>

#include <GL/glew.h>
#include <functional>

namespace ui
{

TexturePreviewCombo::TexturePreviewCombo(wxWindow* parent) :
    wxPanel(parent, wxID_ANY),
    _glWidget(new wxutil::GLWidget(this, std::bind(&TexturePreviewCombo::_onRender, this), "TexturePreviewCombo")),
    _texName(""),
	_infoTable(nullptr),
	_contextMenu(new wxutil::PopupMenu)
{
    _glWidget->SetMinSize(wxSize(128, 128));

    // Add info table
	_infoTable = new wxutil::KeyValueTable(this);
	_infoTable->Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &TexturePreviewCombo::_onContextMenu, this);

	SetSizer(new wxBoxSizer(wxHORIZONTAL));

	GetSizer()->Add(_glWidget, 0, wxEXPAND);
	GetSizer()->Add(_infoTable, 1, wxEXPAND);

    // Construct the context menu
    _contextMenu->addItem(
		new wxutil::StockIconTextMenuItem(_("Copy shader name"), wxART_COPY),
        std::bind(&TexturePreviewCombo::_onCopyTexName, this)
    );

    loadLightTexturePrefixes();
}

void TexturePreviewCombo::ClearPreview()
{
    SetPreviewDeclName({});
}

void TexturePreviewCombo::SetPreviewDeclName(const std::string& declName)
{
    _texName = declName;
    refreshInfoTable();

    _glWidget->Refresh(false);

#if defined(__WXGTK__) && !wxCHECK_VERSION(3, 1, 3)
	// Just calling Refresh doesn't cut it before wxGTK 3.1.3
	// the GLWidget::OnPaint event is never invoked unless we call Update()
    _glWidget->Update();
#endif
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
    auto shader = GlobalMaterialManager().getMaterial(_texName);

    // Regular shader info
    _infoTable->Append(_("Shader"), shader->getName());
    _infoTable->Append(_("Defined in"), shader->getShaderFileName());

    auto descr = shader->getDescription();
    _infoTable->Append(_("Description"), descr.empty() ? "-" : descr);

    if (isLightTexture())
    {
        auto first = shader->firstLayer();

        if (first)
        {
            auto tex = shader->firstLayer()->getTexture();
            _infoTable->Append(_("Image map"), tex->getName());
        }

        // Light types, from the Material
        std::string lightType;
        if (shader->isAmbientLight())
            lightType.append("ambient ");
        if (shader->isBlendLight())
            lightType.append("blend ");
        if (shader->isFogLight())
            lightType.append("fog");
        if (lightType.empty())
            lightType.append("-");

        _infoTable->Append(_("Light flags"), lightType);
    }

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

bool TexturePreviewCombo::_onRender()
{
	// Get the viewport size from the GL widget
	auto req = _glWidget->GetClientSize();

	if (req.GetWidth() == 0 || req.GetHeight() == 0) return false;

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
		auto shader = GlobalMaterialManager().getMaterial(_texName);

        // This is an "ordinary" texture, take the editor image
        auto tex = shader->getEditorImage();

        // If this is a light, take #the first layer texture, but prefer the editor image if we got one
        if (isLightTexture() && !tex)
        {
            if (auto first = shader->firstLayer(); first)
            {
                tex = shader->firstLayer()->getTexture();
            }
        }

		if (tex)
		{
			glBindTexture(GL_TEXTURE_2D, tex->getGLTexNum());

			// Calculate the correct aspect ratio for preview
			auto aspect = static_cast<float>(tex->getWidth()) / tex->getHeight();
			float hfWidth, hfHeight;

			if (aspect > 1.0f)
			{
				hfWidth = 0.5f * req.GetWidth();
				hfHeight = 0.5f * req.GetHeight() / aspect;
			}
			else
			{
				hfHeight = 0.5f * req.GetWidth();
				hfWidth = 0.5f * req.GetHeight() * aspect;
			}

			// Draw a quad to put the texture on
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glColor3f(1, 1, 1);

			glBegin(GL_QUADS);
			glTexCoord2i(0, 1);
			glVertex2f(0.5f*req.GetWidth() - hfWidth, 0.5f*req.GetHeight() - hfHeight);
			glTexCoord2i(1, 1);
			glVertex2f(0.5f*req.GetWidth() + hfWidth, 0.5f*req.GetHeight() - hfHeight);
			glTexCoord2i(1, 0);
			glVertex2f(0.5f*req.GetWidth() + hfWidth, 0.5f*req.GetHeight() + hfHeight);
			glTexCoord2i(0, 0);
			glVertex2f(0.5f*req.GetWidth() - hfWidth, 0.5f*req.GetHeight() + hfHeight);
			glEnd();
		}
	}

	glPopAttrib();

	return true;
}

void TexturePreviewCombo::loadLightTexturePrefixes()
{
    _lightTexturePrefixes = game::current::getLightTexturePrefixes();
}

bool TexturePreviewCombo::isLightTexture()
{
    for (const auto& prefix : _lightTexturePrefixes)
    {
        if (string::istarts_with(_texName, prefix))
        {
            return true;
        }
    }

    return false;
}

}
