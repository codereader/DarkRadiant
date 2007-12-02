#ifndef DOOM3MODELDEF_H_
#define DOOM3MODELDEF_H_

#include "ieclass.h"
#include "parser/DefTokeniser.h"

namespace eclass {

class Doom3ModelDef :
	public IModelDef
{
public:
	// Reads the data from the given tokens into the member variables
	void parseFromTokens(parser::DefTokeniser& tokeniser) {
		// Read the name
	    name = tokeniser.nextToken();
	    
	    tokeniser.assertNextToken("{");

	    // State enum
	    enum { 
	        NONE,   // usual state
	        ANIM    // parsed anim, may get a { ... } block with further info
	    } state = NONE;

	    while (true) {
	        const std::string parameter = tokeniser.nextToken();

	        if (parameter == "}") {
	            break;
	        }
	            
	        if (parameter == "inherit") {
	            parent = tokeniser.nextToken();
	        }
	        else if (parameter == "mesh") {
	            mesh = tokeniser.nextToken();
	        }
	        else if (parameter == "skin") {
	            skin = tokeniser.nextToken();
	        }
	        else if (parameter == "offset") {
	            tokeniser.skipTokens(5);
	        }
	        else if (parameter == "channel") {
	            // SYNTAX: "channel" <name> "(" <blah> [ <blah> ... ] ")"
	            tokeniser.skipTokens(2);
	            while (tokeniser.nextToken() != ")");
	        }
	        else if (parameter == "anim") {
	            // SYNTAX: "anim" <name> <md5file> [ "{" <blah> [ <blah> ... ] "}" ]
	            std::string name = tokeniser.nextToken();
	            std::string file = tokeniser.nextToken();
	            
	            anims.insert(Anims::value_type(name, file));
	            state = ANIM; // check for the braces on the next iteration
	        }
	        else if (state == ANIM && parameter == "{") { // anim braces
	            while (tokeniser.nextToken() != "}");
	            state = NONE;
	        }
	    }
	}
};
typedef boost::shared_ptr<Doom3ModelDef> Doom3ModelDefPtr;

} // namespace eclass

#endif /*DOOM3MODELDEF_H_*/
