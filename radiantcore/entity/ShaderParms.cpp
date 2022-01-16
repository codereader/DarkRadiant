#include "ShaderParms.h"

#include "ColourKey.h"
#include "KeyObserverMap.h"
#include "string/convert.h"

#include <functional>
#include <sigc++/bind.h>

namespace entity
{

ShaderParms::ShaderParms(KeyObserverMap& keyObserverMap, ColourKey& colourKey) :
	_keyObserverMap(keyObserverMap),
	_colourKey(colourKey),
	_parmValues(MAX_ENTITY_SHADERPARMS, 0.0f)
{
	_parmValues[3] = 1.0f; // parm3 = alpha, defaults to 1.0f
}

float ShaderParms::getParmValue(int parmNum) const
{
	assert(parmNum >= 0 && parmNum < static_cast<int>(MAX_ENTITY_SHADERPARMS));

	// For parm0..parm2, use the colour keyobserver to retrieve the value
	return parmNum > 2 ? _parmValues[parmNum] : static_cast<float>(_colourKey.getColour()[parmNum]);
}

void ShaderParms::addKeyObservers()
{
	for (std::size_t i = MIN_SHADERPARM_NUM_TO_OBSERVE; i < MAX_ENTITY_SHADERPARMS; ++i)
	{
        _keyObserverMap.observeKey(
            "shaderParm" + string::to_string(i),
            sigc::bind<0>(sigc::mem_fun(this, &ShaderParms::onShaderParmKeyValueChanged), i)
        );
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
