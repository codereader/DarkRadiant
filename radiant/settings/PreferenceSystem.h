#ifndef PREFERENCESYSTEM_H_
#define PREFERENCESYSTEM_H_

#include <string>
class IPreferenceSystem;

/** greebo: Direct accessor method for the preferencesystem for situations
 * 			where the actual PreferenceSystemModule is not loaded yet.
 * 
 * 			Everything else should use the GlobalPreferenceSystem() access method.
 */
IPreferenceSystem& GetPreferenceSystem();

/** greebo: Renames the user.xml in the settings folder.
 */
void resetPreferences();

#endif /*PREFERENCESYSTEM_H_*/
