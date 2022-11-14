#include "Doom3ModelSkin.h"

namespace skins
{

Skin::Skin(const std::string& name) :
    DeclarationBase<decl::ISkin>(decl::Type::Skin, name)
{}

std::string Skin::getName() const
{
	return getDeclName();
}

std::string Skin::getSkinFileName() const
{
	return getDeclFilePath();
}

const std::set<std::string>& Skin::getModels()
{
    ensureParsed();

    return _matchingModels;
}

std::string Skin::getRemap(const std::string& name)
{
    ensureParsed();

    // The remaps are applied in the order they appear in the decl
    for (const auto& remapping : _remaps)
    {
        if (remapping.Original == "*" || remapping.Original == name)
        {
            return remapping.Replacement;
        }
    }

    return {};
}

void Skin::addRemap(const std::string& src, const std::string& dst)
{
    ensureParsed();

    _remaps.emplace_back(Remapping{ src, dst });
}

void Skin::foreachMatchingModel(const std::function<void(const std::string&)>& functor)
{
    ensureParsed();

    for (const auto& model : _matchingModels)
    {
        functor(model);
    }
}

const std::vector<decl::ISkin::Remapping>& Skin::getAllRemappings()
{
    return _remaps;
}

void Skin::onBeginParsing()
{
    _remaps.clear();
    _matchingModels.clear();
}

void Skin::parseFromTokens(parser::DefTokeniser& tokeniser)
{
    // [ "skin" ] <name>
    // "{"
    //      [ "model" <modelname> ]
    //      ( <sourceTex> <destTex> )*
    //      [ * <destTex> ]
    // "}"
    while (tokeniser.hasMoreTokens())
    {
        // Read key/value pairs until end of decl
        auto key = tokeniser.nextToken();
        auto value = tokeniser.nextToken();

        // If this is a model key, add to the model->skin map, otherwise assume
        // this is a remap declaration
        if (key == "model")
        {
            _matchingModels.insert(value);
        }
        else
        {
            // Add the pair, preserving any wildcards "*"
            _remaps.emplace_back(Remapping{ std::move(key), std::move(value) });
        }
    }
}

}
