#pragma once

#include <map>
#include "igl.h"
#include "irender.h"

namespace render
{

/**
 * Text renderer implementation drawing the attached IRenderableText
 * instances to the scene. Does not change any GL state/matrices.
 * 
 * Requires a valid IGLFont reference at construction time.
 */
class TextRenderer final :
    public ITextRenderer
{
private:
    std::map<Slot, std::reference_wrapper<IRenderableText>> _slots;

    Slot _freeSlotMappingHint;

    IGLFont::Ptr _font;

public:
    TextRenderer(const IGLFont::Ptr& font) :
        _font(font),
        _freeSlotMappingHint(0)
    {
        assert(_font);
    }

    Slot addText(IRenderableText& text) override
    {
        // Find a free slot
        auto newSlotIndex = getNextFreeSlotIndex();

        _slots.emplace(newSlotIndex, text);

        return newSlotIndex;
    }

    void removeText(Slot slot) override
    {
        // Remove the slot
        _slots.erase(slot);

        if (slot < _freeSlotMappingHint)
        {
            _freeSlotMappingHint = slot;
        }
    }

    void render()
    {
        for (const auto& [_, ref] : _slots)
        {
            auto& renderable = ref.get();
            const auto& text = renderable.getText();

            if (text.empty()) continue;

            glColor4dv(renderable.getColour());
            glRasterPos3dv(renderable.getWorldPosition());

            _font->drawString(text);
        }
    }

private:
    Slot getNextFreeSlotIndex()
    {
        for (auto i = _freeSlotMappingHint; i < std::numeric_limits<Slot>::max(); ++i)
        {
            if (_slots.count(i) == 0)
            {
                _freeSlotMappingHint = i + 1; // start searching here next time
                return i;
            }
        }

        throw std::runtime_error("TextRenderer ran out of slot numbers");
    }
};

}
