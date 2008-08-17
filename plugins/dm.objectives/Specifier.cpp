#include "Specifier.h"
#include "Component.h"

#include "itextstream.h"
#include "string/string.h"

namespace objectives {

namespace {

inline std::string getPlural(int count, 
	const std::string& singular, const std::string& plural)
{
	return (count == 1) ? singular : plural;
}

inline std::string printEntity(int count) {
	return getPlural(count, "entity", "entities");
}

} // namespace

std::string Specifier::getSentence(Component& component) {

	int id = getType().getId();
	std::string result;

	std::string amountStr = component.getArgument(0);
	int amount = strToInt(amountStr);
	
	if (id == SpecifierType::SPEC_NONE().getId()) {
		result += "<not specified>";
	}
	else if (id == SpecifierType::SPEC_NAME().getId()) {
		result += "the entity " + _value;
	}
	else if (id == SpecifierType::SPEC_OVERALL().getId()) {
		result += amountStr + " " + printEntity(amount);
	}
	else if (id == SpecifierType::SPEC_GROUP().getId()) {

		if (_value == "loot_gold") {
			result += amountStr + " loot in gold";
		}
		else if (_value == "loot_goods") {
			result += amountStr + " loot in goods";
		}
		else if (_value == "loot_jewels") {
			result += amountStr + " loot in jewels";
		}
		else if (_value == "loot_total") {
			result += amountStr + " loot";
		}
		else {
			result += amountStr + " of \"" + _value + "\"";
		}
	}
	else if (id == SpecifierType::SPEC_CLASSNAME().getId()) {
		result += amountStr + " " + printEntity(amount) + " of type " + _value;
	}
	else if (id == SpecifierType::SPEC_SPAWNCLASS().getId()) {
		int amount = strToInt(component.getArgument(0));
		result += amountStr + " " + printEntity(amount) + " of spawnclass " + _value;
	}
	else if (id == SpecifierType::SPEC_AI_TYPE().getId()) {
		result += amountStr + " AI of type " + _value;
	}
	else if (id == SpecifierType::SPEC_AI_TEAM().getId()) {
		result += amountStr + " AI of team " + _value;
	}
	else if (id == SpecifierType::SPEC_AI_INNOCENCE().getId()) {
		result += amountStr + " AI of " + _value;
	}
	else {
		globalErrorStream() << "Unknown specifier ID " << id << "found!" << std::endl;
	}
	
	return result;
}

} // namespace objectives
