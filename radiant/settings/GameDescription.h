#ifndef GAMEDESCRIPTION_H_
#define GAMEDESCRIPTION_H_

#include <map>
#include <string>
#include "xmlutil/Document.h"

/*!
holds information for a given game
I'm a bit unclear on that still
it holds game specific configuration stuff
such as base names, engine names, some game specific features to activate in the various modules
it is not strictly a prefs thing since the user is not supposed to edit that (unless he is hacking
support for a new game)

what we do now is fully generate the information for this during the setup. We might want to
generate a piece that just says "the game pack is there", but put the rest of the config somwhere
else (i.e. not generated, copied over during setup .. for instance in the game tools directory)
*/
class GameDescription 
{
	typedef std::map<std::string, std::string> DescriptionMap;

	// xml::Document object constructed from the provided xmlDocPtr
	xml::Document _doc;

	DescriptionMap _descriptions;

public:
	GameDescription(const xml::Document& doc, const std::string& gameFile);

	std::string mGameFile; ///< the .game file that describes this game
	std::string mGameType; ///< the type of the engine

	/** greebo: Looks up the specified key
	 */
	const char* getKeyValue(const std::string& key) const;
	
	/** greebo: Emits an error if the keyvalue is empty
	 */
	const char* getRequiredKeyValue(const std::string& key) const;
};

#endif /*GAMEDESCRIPTION_H_*/
