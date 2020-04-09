#include "SRPropertyLoader.h"

#include "iregistry.h"
#include "entitylib.h"
#include "string/convert.h"
#include "gamelib.h"
#include <regex>

SRPropertyLoader::SRPropertyLoader(SREntity::KeyList& keys, SREntity& srEntity, std::string& warnings) :
	_keys(keys),
	_srEntity(srEntity),
	_warnings(warnings),
	_prefix(game::current::getValue<std::string>(GKEY_STIM_RESPONSE_PREFIX)),
	_responseEffectPrefix(game::current::getValue<std::string>(GKEY_RESPONSE_EFFECT_PREFIX))
{}

void SRPropertyLoader::visitKeyValue(const std::string& key, const std::string& value)
{
	parseAttribute(key, value, false);
}

void SRPropertyLoader::operator() (const EntityClassAttribute& attribute)
{
	parseAttribute(attribute.getName(), attribute.getValue(), true);
}

StimResponse& SRPropertyLoader::findOrCreate(int index, bool inherited)
{
	auto& found = _srEntity.get(index);

	if (found.getIndex() == index)
	{
		return found;
	}

	// Create the S/R object in case we got an empty S/R back
	// Insert a new SR object with the given index
	auto& sr = _srEntity.add(index);
	sr.setInherited(inherited);

	return sr;
}

void SRPropertyLoader::parseAttribute(const std::string& key, const std::string& value, bool inherited)
{
	// Cycle through the possible key names and see if we have a match
	for (const auto& srKey : _keys) 
	{
		// Construct a regex with the number as match variable
		std::string exprStr = "^" + _prefix + srKey.key + "_([0-9]+)$";
		std::regex expr(exprStr);
		std::smatch matches;

		if (std::regex_match(key, matches, expr))
		{
			// Retrieve the S/R index number
			int index = string::convert<int>(matches[1].str());

			// Ensure the S/R index exists in the list
			auto& sr = findOrCreate(index, inherited);

			// Check if the property already exists
			if (!sr.get(srKey.key).empty())
			{
				// already existing, add to the warnings
				_warnings += "Warning on StimResponse #" + string::to_string(index) +
							 ": property " + srKey.key + " defined more than once.\n";
			}

			// Set the property value on the StimResponse object
			sr.set(srKey.key, value, inherited);
		}
	}

	// Check the key for a Response Effect definition
	{
		// This should search for something like "sr_effect_2_3_arg3"
		// (with the optional postfix "_argN" or "_state")
		std::string exprStr =
			"^" + _prefix + _responseEffectPrefix + "([0-9]+)_([0-9]+)(_arg[0-9]+|_state)*$";
		std::regex expr(exprStr);
		std::smatch matches;

		if (std::regex_match(key, matches, expr)) 
		{
			// The response index
			int index = string::convert<int>(matches[1].str());
			// The effect index
			int effectIndex = string::convert<int>(matches[2].str());

			// Ensure the S/R index exists in the list
			auto& sr = findOrCreate(index, inherited);

			// Get the response effect (or create a new one)
			ResponseEffect& effect = sr.getResponseEffect(effectIndex);

			std::string postfix = matches[3];

			if (postfix.empty()) 
			{
				// No "_arg1" found, the value is the effect name definition
				effect.setName(value, inherited);
			}
			else if (postfix == "_state") 
			{
				// This is a state variable
				effect.setActive(value != "0", inherited);
			}
			else 
			{
				// Get the argument index from the tail
				int argIndex = string::convert<int>(postfix.substr(4));

				// Load the value into argument with the index <argIndex>
				effect.setArgument(argIndex, value, inherited);
			}
		}
	}
}
