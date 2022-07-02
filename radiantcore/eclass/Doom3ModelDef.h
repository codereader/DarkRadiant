#pragma once

#include "ieclass.h"
#include "parser/DefTokeniser.h"
#include "DeclarationBase.h"

namespace eclass
{

class Doom3ModelDef :
    public decl::DeclarationBase<IModelDef>
{
private:
    std::string _name;

    Ptr _parent;

    std::string _mesh;
    std::string _skin;
    Anims _anims;

    bool _parsed;

public:
    using Ptr = std::shared_ptr<Doom3ModelDef>;

	Doom3ModelDef(const std::string& modelDefName) :
        _parsed(false),
        _name(modelDefName)
	{}

    std::string getModName() const override
    {
        return getBlockSyntax().getModName();
    }

    const std::string& getDeclName() const override
    {
        return _name;
    }

    decl::Type getDeclType() const override
    {
        return decl::Type::ModelDef;
    }

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
    void onSyntaxBlockAssigned(const decl::DeclarationBlockSyntax& _) override
    {
        // Trigger a reparse next time any of the getters is accessed
        _parsed = false;
    }

private:
    void ensureParsed()
    {
        if (_parsed) return; // nothing to do

        _parsed = true;

        parser::BasicDefTokeniser<std::string> tokeniser(getBlockSyntax().contents);
        parseFromTokens(tokeniser);
    }

    void clear()
    {
        // Don't clear the name
        _parsed = false;
        _mesh.clear();
        _skin.clear();
        _parent.reset();
        _anims.clear();
    }

    // Reads the data from the given tokens into the member variables
    void parseFromTokens(parser::DefTokeniser& tokeniser)
    {
        clear();

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
                auto parentName = tokeniser.nextToken();
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

                _anims.emplace(animName, file);
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
};

} // namespace
