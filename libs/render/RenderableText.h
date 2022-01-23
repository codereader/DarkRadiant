#pragma once

#include "irender.h"

namespace render
{

/**
 * Text instance which can be attached to an ITextRenderer
 * Automatially takes care of deregistering and registering 
 * on renderer change or destruction of this object.
 */
class RenderableText :
    public IRenderableText
{
private:
    Vector3 _worldPosition;
    Vector4 _colour;
    std::string _text;

    ITextRenderer::Ptr _renderer;
    ITextRenderer::Slot _slot;

public:
    RenderableText(const Vector4& colour = { 0,0,0,1 }) :
        _worldPosition(0,0,0),
        _colour(colour),
        _slot(ITextRenderer::InvalidSlot)
    {}

    // Noncopyable
    RenderableText(const RenderableText& other) = delete;
    RenderableText& operator=(const RenderableText& other) = delete;

    virtual ~RenderableText()
    {
        clear();
    }

    void update(const ITextRenderer::Ptr& renderer)
    {
        bool rendererChanged = _renderer != renderer;

        if (rendererChanged)
        {
            clear();
        }

        // Update our local reference
        _renderer = renderer;

        if (_renderer && _slot == ITextRenderer::InvalidSlot)
        {
            _slot = _renderer->addText(*this);
        }
    }

    void clear()
    {
        removeTextFromRenderer();
        _renderer.reset();
    }

    const Vector3& getWorldPosition() override
    {
        return _worldPosition;
    }

    void setWorldPosition(const Vector3& position)
    {
        _worldPosition = position;
    }

    const std::string& getText() override
    {
        return _text;
    }

    void setText(const std::string& text)
    {
        _text = text;
    }

    const Vector4& getColour() override
    {
        return _colour;
    }

private:
    // Removes the text from the attached renderer. Does nothing if no text has been added.
    void removeTextFromRenderer()
    {
        if (_renderer && _slot != ITextRenderer::InvalidSlot)
        {
            _renderer->removeText(_slot);
        }

        _slot = ITextRenderer::InvalidSlot;
    }
};

}
