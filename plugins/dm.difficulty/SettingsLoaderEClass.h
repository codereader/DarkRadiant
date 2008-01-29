#ifndef __DIFFICULTY_SETTINGS_LOADER_ECLASS_H__
#define __DIFFICULTY_SETTINGS_LOADER_ECLASS_H__

#include "ieclass.h"

namespace difficulty {

class DifficultySettings;

class SettingsLoaderEClass :
	public EntityClassAttributeVisitor
{
	// The eclass we're working with
	IEntityClassPtr _eclass;

	DifficultySettings& _settings;

public:
	// The constructor needs the eclass we're working with and the DifficultySettings
	// object which should be filled with the parsed Settings.
	SettingsLoaderEClass(const IEntityClassPtr& eclass, DifficultySettings& settings);

	// EntityClassAttributeVisitor implementation
	// Visit each attribute and check for relevance
	virtual void visit(const EntityClassAttribute& attr);
};

} // namespace difficulty

#endif /* __DIFFICULTY_SETTINGS_LOADER_ECLASS_H__ */
