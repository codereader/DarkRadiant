#ifndef RESPONSESCRIPT_H_
#define RESPONSESCRIPT_H_

#include <map>
#include <string>

/** greebo: The representation of a ResponseScript.
 */
struct ResponseScript {
	// TRUE, if this stems from an base entity class 
	bool inherited;
	
	// The target stimType (e.g. STIM_FIRE)
	std::string stimType;
	
	// The target script that gets fired upon response
	std::string script;
};

// ResponseScripts (each ResponseScript has its own id)
typedef std::vector<ResponseScript> ResponseScripts;

struct ResponseScriptArgument {
	// TRUE, if this is inherited from a base entity class
	bool inherited;
	
	// The target entity
	std::string entityName;
};
typedef std::vector<ResponseScriptArgument> ResponseScriptArgs;

#endif /*RESPONSESCRIPT_H_*/
