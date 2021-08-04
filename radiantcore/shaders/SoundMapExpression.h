#pragma once

#include "iimage.h"
#include "imodule.h"
#include "MapExpression.h"
#include "parser/DefTokeniser.h"
#include "string/case_conv.h"

namespace shaders
{

class SoundMapExpression :
    public ISoundMapExpression,
    public NamedBindable
{
private:
    bool _waveform;
    std::string _filePath;

    const char* const SOUND_MAP_PLACEHOLDER = "soundmap.png";
    const char* const SOUND_MAP_PLACEHOLDER_WAVE = "soundmap_wave.png";

public:
    SoundMapExpression(bool waveform) :
        _waveform(waveform)
    {}

    virtual bool isCubeMap() const override
    {
        return false;
    }

    virtual bool isWaveform() const override
    {
        return _waveform;
    }

    virtual std::string getExpressionString() override
    {
        return _filePath;
    }

    virtual std::string getIdentifier() const override
    {
        return isWaveform() ? "__soundMapWave__" : "__soundMap__";
    }

    virtual TexturePtr bindTexture(const std::string& name, Role) const override
    {
        auto imagePath = module::GlobalModuleRegistry().getApplicationContext().getBitmapsPath();
        imagePath += isWaveform() ? SOUND_MAP_PLACEHOLDER_WAVE : SOUND_MAP_PLACEHOLDER;

        auto img = GlobalImageLoader().imageFromFile(imagePath);

        return img ? img->bindTexture(name) : TexturePtr();
    }

    static std::shared_ptr<SoundMapExpression> CreateForTokens(parser::DefTokeniser& tokeniser)
    {
        bool waveform = string::to_lower_copy(tokeniser.peek()) == "waveform";

        return std::make_shared<SoundMapExpression>(waveform);
    }
};

}
