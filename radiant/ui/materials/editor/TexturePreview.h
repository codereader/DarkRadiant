#pragma once

#include "igl.h"
#include "ishaderlayer.h"
#include "wxutil/GLWidget.h"

namespace ui
{

/**
 * An openGL preview widget rendering the map of a single material stage,
 * e.g. the bumpmap only.
 */
class TexturePreview :
    public wxutil::GLWidget
{
public:
    enum class ImageType
    {
        EditorImage,
        Diffuse,
        Bump,
        Specular
    };

private:
    ImageType _type;
    MaterialPtr _material;

public:
    TexturePreview(wxWindow* parent, ImageType type = ImageType::EditorImage) :
        GLWidget(parent, std::bind(&TexturePreview::onRender, this), "TexturePreview"),
        _type(type)
    {}

    void SetImageType(ImageType type)
    {
        _type = type;
        QueueDraw();
    }

    void SetMaterial(const MaterialPtr& material)
    {
        _material = material;
        QueueDraw();
    }

    void QueueDraw()
    {
        Refresh(false);

#if defined(__WXGTK__) && !wxCHECK_VERSION(3, 1, 3)
        // Just calling Refresh doesn't cut it before wxGTK 3.1.3
        // the GLWidget::OnPaint event is never invoked unless we call Update()
        Update();
#endif
    }

private:

    bool onRender()
    {
        // Get the viewport size from the GL widget
        wxSize req = GetClientSize();

        if (req.GetWidth() == 0 || req.GetHeight() == 0) return false;

        glPushAttrib(GL_ALL_ATTRIB_BITS);

        glViewport(0, 0, req.GetWidth(), req.GetHeight());

        // Initialise
        glClearColor(0.9f, 0.9f, 0.9f, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDisable(GL_DEPTH_TEST);
        glMatrixMode(GL_TEXTURE);
        glLoadIdentity();
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, req.GetWidth(), 0, req.GetHeight(), -100, 100);
        glEnable(GL_TEXTURE_2D);

        // This is an "ordinary" texture, take the editor image
        auto texture = getLayerTexture();

        if (texture)
        {
            glBindTexture(GL_TEXTURE_2D, texture->getGLTexNum());

            // Calculate the correct aspect ratio for preview
            float aspect = float(texture->getWidth()) / float(texture->getHeight());
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
            glVertex2f(0.5 * req.GetWidth() - hfWidth, 0.5 * req.GetHeight() - hfHeight);
            glTexCoord2i(1, 1);
            glVertex2f(0.5 * req.GetWidth() + hfWidth, 0.5 * req.GetHeight() - hfHeight);
            glTexCoord2i(1, 0);
            glVertex2f(0.5 * req.GetWidth() + hfWidth, 0.5 * req.GetHeight() + hfHeight);
            glTexCoord2i(0, 0);
            glVertex2f(0.5 * req.GetWidth() - hfWidth, 0.5 * req.GetHeight() + hfHeight);
            glEnd();
        }

        glPopAttrib();

        return true;
    }

    TexturePtr getLayerTexture()
    {
        if (!_material)
        {
            return TexturePtr();
        }

        if (_type == ImageType::EditorImage)
        {
            return _material->getEditorImage();
        }

        auto layerToFind = IShaderLayer::DIFFUSE;

        switch (_type)
        {
        case ImageType::Diffuse: layerToFind = IShaderLayer::DIFFUSE; break;
        case ImageType::Bump: layerToFind = IShaderLayer::BUMP; break;
        case ImageType::Specular: layerToFind = IShaderLayer::SPECULAR; break;
        }

        TexturePtr result;

        _material->foreachLayer([&](const IShaderLayer::Ptr& layer)
        {
            if (layer->getType() == layerToFind)
            {
                result = layer->getTexture();
                return false;
            }

            return true;
        });

        return result;
    }
};

}
