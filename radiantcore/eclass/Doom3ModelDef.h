#pragma once

#include "ieclass.h"
#include "parser/DefTokeniser.h"
#include "decl/DeclarationBase.h"

namespace eclass
{

class Doom3ModelDef :
    public decl::DeclarationBase<IModelDef>
{
private:
    Ptr _parent;

    std::string _mesh;
    std::string _skin;
    Anims _anims;

public:
    using Ptr = std::shared_ptr<Doom3ModelDef>;

	Doom3ModelDef(const std::string& name) :
        DeclarationBase<IModelDef>(decl::Type::ModelDef, name)
	{}

    const std::string& getMesh() override
	{
        ensureParsed();
        return _mesh;
	}

    const IModelDef::Ptr& getParent() override
	{
        ensureParsed();
        return _parent;
	}

    const std::string& getSkin() override
	{
        ensureParsed();
        return _skin;
	}

    std::string getAnim(const std::string& animKey) override
	{
        ensureParsed();

        auto existing = _anims.find(animKey);
        return existing != _anims.end() ? existing->second : std::string();
	}

    const Anims& getAnims() override
	{
        ensureParsed();
        return _anims;
	}

protected:
    void onBeginParsing() override
    {
        clearContents();
    }

    // Reads the data from the given tokens into the member variables
    void parseFromTokens(parser::DefTokeniser& tokeniser) override
    {
        // State enum
        enum
        {
            NONE,   // usual state
            ANIM    // parsed anim, may get a { ... } block with further info
        } state = NONE;

        while (tokeniser.hasMoreTokens())
        {
            auto parameter = tokeniser.nextToken();

            if (parameter == "inherit")
            {
                inheritFrom(tokeniser.nextToken());
            }
            else if (parameter == "mesh")
            {
                _mesh = tokeniser.nextToken();
            }
            else if (parameter == "skin")
            {
                _skin = tokeniser.nextToken();
            }
            else if (parameter == "offset")
            {
                tokeniser.skipTokens(5);
            }
            else if (parameter == "channel")
            {
                // SYNTAX: "channel" <name> "(" <blah> [ <blah> ... ] ")"
                tokeniser.skipTokens(2);
                while (tokeniser.nextToken() != ")") continue;
            }
            else if (parameter == "anim")
            {
                // SYNTAX: "anim" <name> <md5file> [ "{" <blah> [ <blah> ... ] "}" ]
                auto animName = tokeniser.nextToken();
                auto file = tokeniser.nextToken();

                // Overwrite any existing animation with the same key
                _anims[animName] = file;
                state = ANIM; // check for the braces on the next iteration
            }
            else if (state == ANIM && parameter == "{")
            {
                // anim braces
                while (tokeniser.nextToken() != "}") continue;
                state = NONE;
            }
        }
    }

private:
    void clearContents()
    {
        // Don't clear the name
        _mesh.clear();
        _skin.clear();
        _parent.reset();
        _anims.clear();
    }

    void inheritFrom(const std::string& parentName)
    {
        auto parent = GlobalEntityClassManager().findModel(parentName);

        if (!parent)
        {
            rWarning() << "ModelDef " << getDeclName() << " inherits from unknown parent: " << parentName << std::endl;
            return;
        }

        _parent = parent;

        // greebo: Only inherit the "mesh" of the parent if the current declaration doesn't have one
        if (_mesh.empty())
        {
            _mesh = parent->getMesh();
        }

        // Only inherit the "skin" of the parent if the current declaration doesn't have one
        if (_skin.empty())
        {
            _skin = parent->getSkin();
        }

        // Append all inherited animations, if missing on the child
        const auto& parentAnims = parent->getAnims();
        _anims.insert(parentAnims.begin(), parentAnims.end());
    }
};

} // namespace
