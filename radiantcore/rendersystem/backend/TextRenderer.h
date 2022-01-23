#pragma once

#include "irender.h"

namespace render
{

class TextRenderer final :
    public ITextRenderer
{
private:
    struct TextInfo
    {
        bool occupied;
        std::reference_wrapper<IRenderableText> text;

        TextInfo(IRenderableText& text_) :
            text(text_),
            occupied(true)
        {}
    };
    std::map<Slot, TextInfo> _slots;

    Slot _freeSlotMappingHint;

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
        // TODO
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
