#pragma once

#include <vector>

#include "KeyObserverDelegate.h"

namespace entity
{

class ColourKey;
class KeyObserverMap;

/**
 * @brief Helper class observing the "shaderParmNN" spawnargs and caching their values.
 *
 * Since the parm0..parm2 values are hardwired to the entity colour components as defined in the
 * "_color" spawnarg this class needs a reference to the corresponding ColourKey class.
 */
class ShaderParms: public sigc::trackable
{
	// The key observer map this class is adding the observers to
	KeyObserverMap& _keyObserverMap;

	ColourKey& _colourKey;

	static const std::size_t MAX_ENTITY_SHADERPARMS = 12;

	// We don't observer shaderParm0..shaderParm2 spawnargs
	static const std::size_t MIN_SHADERPARM_NUM_TO_OBSERVE = 3;

	// The cached values
	std::vector<float> _parmValues;

public:
	ShaderParms(KeyObserverMap& keyObserverMap, ColourKey& colourKey);

	// Return the parm0..parm11 value
	float getParmValue(int parmNum) const;

	void addKeyObservers();

private:
	void onShaderParmKeyValueChanged(std::size_t parm, const std::string& value);
};

} // namespace
