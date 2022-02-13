#pragma once

#include "irender.h"

namespace render
{

/**
 * Text instance which can be attached to an ITextRenderer
 * Automatially takes care of deregistering and registering 
 * on renderer change or destruction of this object.
 * 
 * This is an incomplete implementation of IRenderableText,
 * use the StaticRenderableText type for a simple working class.
 */
class RenderableTextBase :
    public IRenderableText
{
private:
    ITextRenderer::Ptr _renderer;
    ITextRenderer::Slot _slot;

protected:
    RenderableTextBase() :
        _slot(ITextRenderer::InvalidSlot)
    {}

    // Noncopyable
    RenderableTextBase(const RenderableTextBase& other) = delete;
    RenderableTextBase& operator=(const RenderableTextBase& other) = delete;

public:
    virtual ~RenderableTextBase()
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
