#pragma once

#include "igl.h"
#include "irender.h"

namespace render
{

class TextRenderer final :
    public ITextRenderer
{
private:
    std::map<Slot, std::reference_wrapper<IRenderableText>> _slots;

    Slot _freeSlotMappingHint;

    IGLFont::Ptr _glFont;

public:
    TextRenderer() :
        _freeSlotMappingHint(0)
    {}

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
        if (!_glFont)
        {
            // TODO: make size and style configurable
            _glFont = GlobalOpenGL().getFont(IGLFont::Style::Sans, 14);
        }

        for (const auto& [_, ref] : _slots)
        {
            auto& renderable = ref.get();
            const auto& text = renderable.getText();

            if (text.empty()) continue;

            glColor4dv(renderable.getColour());
            glRasterPos3dv(renderable.getWorldPosition());

            _glFont->drawString(text);
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
