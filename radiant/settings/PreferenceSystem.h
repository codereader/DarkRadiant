#pragma once

class IPreferenceSystem;

/** 
 * greebo: Direct accessor method for the preferencesystem for situations
 * where the actual PreferenceSystemModule is not loaded yet.
 * Everything else should use the GlobalPreferenceSystem() access method.
 */
IPreferenceSystem& GetPreferenceSystem();
