#pragma once

#include "ieclass.h"
#include "ientity.h"
#include "SREntity.h"
#include "StimTypes.h"

class SRPropertyLoader
{
private:
	// The reference to the list of possible key names
	SREntity::KeyList& _keys;

	// The container hosting the StimResponse objects
	SREntity& _srEntity;

	// The target string for storing the parse warnings
	std::string& _warnings;

	// Local helper class containing all the stimtypes
	StimTypes _stimTypes;

	// spawnarg prefixes (as defined in the .game file)
	std::string _prefix;
	std::string _responseEffectPrefix;

public:
	SRPropertyLoader(SREntity::KeyList& keys,
					 SREntity& srEntity,
					 std::string& warnings);

	void visitKeyValue(const std::string& key, const std::string& value);

	// operator() for IEntityClass::forEachEntityClass
	void operator() (const EntityClassAttribute& attribute);

private:
	StimResponse& findOrCreate(int index, bool inherited);

	/** greebo: Private helper method that does the attribute analysis
	 * 			and adds the SR items to the target list
	 *
	 * @inherited: reflects whether this key/value stems from a inheritance
	 */
	void parseAttribute(const std::string& key,
						const std::string& value,
						bool inherited);

};
