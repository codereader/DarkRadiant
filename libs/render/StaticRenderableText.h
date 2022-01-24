#pragma once

#include "RenderableTextBase.h"

namespace render
{

/**
 *Simple RenderableText implementation that holds the text, colour and position
 * information as private fields.
 */
class StaticRenderableText :
    public RenderableTextBase
{
private:
    std::string _text;
    Vector3 _worldPosition;
    Vector4 _colour;

    bool _visible;

public:
    StaticRenderableText(const std::string& text, const Vector3& worldPosition, const Vector4& colour) :
        _text(text),
        _worldPosition(worldPosition),
        _colour(colour),
        _visible(true)
    {}

    // When hidden, this class will return an empty string in getText(),
    // causing this instance to be skipped by the ITextRenderer
    void setVisible(bool isVisible)
    {
        _visible = isVisible;
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
        // Return an empty text if this renderable is invisible
        static std::string EmptyText;
        return _visible ? _text : EmptyText;
    }

    void setText(const std::string& text)
    {
        _text = text;
    }

    const Vector4& getColour() override
    {
        return _colour;
    }

    void setColour(const Vector4& colour)
    {
        _colour = colour;
    }
};

}
