#pragma once

#include "igame.h"
#include "string/convert.h"

namespace game
{

namespace current
{

/**
 * \brief
 * Get the value of the current game's tree in the registry and convert it 
 * to type T. If the key cannot be found or is not convertible to the 
 * required type, a default-constructed T will be returned.
 *
 * T must be default-constructible, copy-constructible and convertible from
 * an std::string using boost::lexical_cast.
 */
template<typename T> 
inline T getValue(const std::string& localXPath, T defaultVal = T())
{
	xml::NodeList list = GlobalGameManager().currentGame()->getLocalXPath(localXPath);

	return list.empty() ? defaultVal : string::convert<T>(list[0].getAttributeValue("value"));
}

} // namespace

} // namespace
