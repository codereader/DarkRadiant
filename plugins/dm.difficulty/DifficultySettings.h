#ifndef DIFFICULTY_SETTINGS_H_
#define DIFFICULTY_SETTINGS_H_

#include "ieclass.h"
#include <boost/shared_ptr.hpp>

namespace difficulty {

class DifficultySettings
{
	// the difficulty level, these settings are referring to
	int _level;


public:
	// Define the difficulty level in the constructor
	DifficultySettings(int level);

	// Returns the level these settings are referring to
	int getLevel() const;

	// Empties the internal structures
	void clear();

	// Loads all settings (matching the internal _level) from the given entityDef.
	void parseFromEntityDef(const IEntityClassPtr& def);
};
typedef boost::shared_ptr<DifficultySettings> DifficultySettingsPtr;

} // namespace difficulty

#endif /* DIFFICULTY_SETTINGS_H_ */
