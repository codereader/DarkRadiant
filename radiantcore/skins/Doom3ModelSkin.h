#pragma once

#include "modelskin.h"
#include "decl/DeclarationBase.h"

#include <string>
#include <map>
#include <memory>

namespace skins
{

/**
 * A single instance of a Doom 3 model skin. This structure stores a set of
 * maps between an existing texture and a new texture, and possibly the name of
 * the model that this skin is associated with.
 */
class Skin :
    public decl::DeclarationBase<decl::ISkin>
{
    // The list of models this skin is matching
    std::set<std::string> _matchingModels;

	// Map of texture switches
    std::map<std::string, std::string> _remaps;

public:
    Skin(const std::string& name) :
        DeclarationBase<decl::ISkin>(decl::Type::Skin, name)
	{}

	std::string getName() const {
		return getDeclName();
	}

	std::string getSkinFileName() const {
		return getDeclFilePath();
	}

	// Get this skin's remap for the provided material name (if any).
	std::string getRemap(const std::string& name) override
    {
        ensureParsed();

		auto i = _remaps.find(name);
        return  i != _remaps.end() ? i->second : std::string();
	}

	// Add a remap pair to this skin
	void addRemap(const std::string& src, const std::string& dst)
    {
		_remaps.emplace(src, dst);
	}

    // Visit the functor with the name of each model mentioned in this skin declaration
    void foreachMatchingModel(const std::function<void(const std::string&)>& functor)
    {
        ensureParsed();

        for (const auto& model : _matchingModels)
        {
            functor(model);
        }
    }

protected:
    void onBeginParsing() override
    {
        _remaps.clear();
        _matchingModels.clear();
    }

    void parseFromTokens(parser::DefTokeniser& tokeniser) override
    {
        // [ "skin" ] <name>
        // "{"
        //      [ "model" <modelname> ]
        //      ( <sourceTex> <destTex> )*
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
                addRemap(key, value);
            }
        }
    }
};


}
