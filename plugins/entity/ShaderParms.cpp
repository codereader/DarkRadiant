#include "ShaderParms.h"

#include "ColourKey.h"
#include "KeyObserverMap.h"
#include "string/convert.h"

#include <boost/bind.hpp>

namespace entity
{

ShaderParms::ShaderParms(KeyObserverMap& keyObserverMap, ColourKey& colourKey) :
	_keyObserverMap(keyObserverMap),
	_colourKey(colourKey),
	_shaderParmObservers(MAX_ENTITY_SHADERPARMS),
	_parmValues(MAX_ENTITY_SHADERPARMS, 0.0f)
{
	_parmValues[3] = 1.0f; // parm3 = alpha, defaults to 1.0f

	// Set the callbacks for the key observers
	for (std::size_t i = MIN_SHADERPARM_NUM_TO_OBSERVE; i < MAX_ENTITY_SHADERPARMS; ++i)
	{
		_shaderParmObservers[i].setCallback(
			boost::bind(&ShaderParms::onShaderParmKeyValueChanged, this, i, _1));
	}
}

float ShaderParms::getParmValue(int parmNum) const
{
	assert(parmNum >= 0 && parmNum < MAX_ENTITY_SHADERPARMS);

	// For parm0..parm2, use the colour keyobserver to retrieve the value
	return parmNum > 2 ? _parmValues[parmNum] : static_cast<float>(_colourKey.getColour()[parmNum]);
}

void ShaderParms::addKeyObservers()
{
	for (std::size_t i = MIN_SHADERPARM_NUM_TO_OBSERVE; i < MAX_ENTITY_SHADERPARMS; ++i)
	{
		_keyObserverMap.insert("shaderParm" + string::to_string(i), _shaderParmObservers[i]);
	}
}

void ShaderParms::removeKeyObservers()
{
	for (int i = MIN_SHADERPARM_NUM_TO_OBSERVE; i < MAX_ENTITY_SHADERPARMS; ++i)
	{
		_keyObserverMap.erase("shaderParm" + string::to_string(i), _shaderParmObservers[i]);
	}
}

void ShaderParms::onShaderParmKeyValueChanged(std::size_t parm, const std::string& value)
{
	// For empty spawnarg values, revert to our default values
	if (value.empty())
	{
		// parm0 defaults to 3
		_parmValues[parm] = parm == 3 ? 1.0f : 0.0f;
	}
	else
	{
		// Get the floating point value and cache it locally
		_parmValues[parm] = string::convert<float>(value);
	}
}

} // namespace
