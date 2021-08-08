#pragma once

#include "iimage.h"
#include "imodule.h"
#include "MapExpression.h"
#include "parser/DefTokeniser.h"
#include "string/case_conv.h"

namespace shaders
{

class VideoMapExpression :
    public IVideoMapExpression,
    public NamedBindable
{
private:
    bool _looping;
    std::string _filePath;

    const char* const VIDEO_MAP_PLACEHOLDER = "videomap.png";

public:
    VideoMapExpression(const std::string& filePath, bool looping) :
        _filePath(filePath),
        _looping(looping)
    {}

    virtual bool isCubeMap() const override
    {
        return false;
    }

    virtual bool isLooping() const override
    {
        return _looping;
    }

    virtual std::string getExpressionString() override
    {
        return _filePath;
    }

    virtual std::string getIdentifier() const override
    {
        return "__videoMap__" + _filePath;
    }

    virtual TexturePtr bindTexture(const std::string& name, Role) const override
    {
        auto bitmapsPath = module::GlobalModuleRegistry().getApplicationContext().getBitmapsPath();
        auto img = GlobalImageLoader().imageFromFile(bitmapsPath + VIDEO_MAP_PLACEHOLDER);

        return img ? img->bindTexture(name) : TexturePtr();
    }

    static std::shared_ptr<VideoMapExpression> CreateForTokens(parser::DefTokeniser& tokeniser)
    {
        auto nextToken = tokeniser.nextToken();

        if (string::to_lower_copy(nextToken) == "loop")
        {
            return std::make_shared<VideoMapExpression>(tokeniser.nextToken(), true);
        }
        else
        {
            return std::make_shared<VideoMapExpression>(nextToken, false);
        }
    }
};

}
