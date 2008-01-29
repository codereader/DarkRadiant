#include "SettingsLoaderEClass.h"

#include "DifficultySettings.h"
#include <boost/regex.hpp>

namespace difficulty {

SettingsLoaderEClass::SettingsLoaderEClass(const IEntityClassPtr& eclass, DifficultySettings& settings) :
	_eclass(eclass),
	_settings(settings)
{}

void SettingsLoaderEClass::visit(const EntityClassAttribute& attr) {
	
}

} // namespace difficulty
