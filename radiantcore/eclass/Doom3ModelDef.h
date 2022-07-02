#pragma once

#include "ieclass.h"
#include "parser/DefTokeniser.h"
#include "DeclarationBase.h"

namespace eclass
{

class Doom3ModelDef :
    public decl::DeclarationBase<IModelDef>
{
public:
    using Ptr = std::shared_ptr<Doom3ModelDef>;

	Doom3ModelDef(const std::string& modelDefName)
	{
		name = modelDefName;
	}

	void setModName(const std::string& newModName)
	{
		modName = newModName;
	}

    const std::string& getDeclName() const override
    {
        return name;
    }

    decl::Type getDeclType() const override
    {
        return decl::Type::ModelDef;
    }

	void clear()
	{
		// Don't clear the name
		resolved = false;
		mesh.clear();
		skin.clear();
		parent.clear();
		anims.clear();
		modName = "base";
        defFilename.clear();
	}

    void onSyntaxBlockAssigned(const decl::DeclarationBlockSyntax& block) override
    {
        parser::BasicDefTokeniser<std::string> tokeniser(block.contents);
        parseFromTokens(tokeniser);
    }

	// Reads the data from the given tokens into the member variables
	void parseFromTokens(parser::DefTokeniser& tokeniser)
	{
		clear();

	    // State enum
	    enum {
	        NONE,   // usual state
	        ANIM    // parsed anim, may get a { ... } block with further info
	    } state = NONE;

	    while (tokeniser.hasMoreTokens())
		{
	        auto parameter = tokeniser.nextToken();

	        if (parameter == "inherit")
            {
	            parent = tokeniser.nextToken();
	        }
	        else if (parameter == "mesh")
            {
	            mesh = tokeniser.nextToken();
	        }
	        else if (parameter == "skin")
            {
	            skin = tokeniser.nextToken();
	        }
	        else if (parameter == "offset")
            {
	            tokeniser.skipTokens(5);
	        }
	        else if (parameter == "channel")
            {
	            // SYNTAX: "channel" <name> "(" <blah> [ <blah> ... ] ")"
	            tokeniser.skipTokens(2);
	            while (tokeniser.nextToken() != ")");
	        }
	        else if (parameter == "anim")
            {
	            // SYNTAX: "anim" <name> <md5file> [ "{" <blah> [ <blah> ... ] "}" ]
	            auto animName = tokeniser.nextToken();
	            auto file = tokeniser.nextToken();

	            anims.emplace(animName, file);
	            state = ANIM; // check for the braces on the next iteration
	        }
	        else if (state == ANIM && parameter == "{")
            {
	            // anim braces
	            while (tokeniser.nextToken() != "}");
	            state = NONE;
	        }
	    }
	}
};

} // namespace
