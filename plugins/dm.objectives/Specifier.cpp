#include "Specifier.h"
#include "Component.h"

#include "i18n.h"
#include "itextstream.h"
#include "string/convert.h"
#include <fmt/format.h>

namespace objectives {

namespace {

inline std::string getPlural(int count,
	const std::string& singular, const std::string& plural)
{
	return (count == 1) ? singular : plural;
}

inline std::string printEntity(int count) {
	return getPlural(count, _("entity"), _("entities"));
}

inline std::string printEntityAmount(const std::string& amountStr) {
	if (amountStr.empty()) {
		return printEntity(1);
	}
	else {
		return amountStr + " " + printEntity(string::convert<int>(amountStr));
	}
}

} // namespace

std::string Specifier::getSentence(Component& component) {

	int id = getType().getId();
	std::string result;

	std::string amountStr = component.getArgument(0);
	int amount = string::convert<int>(amountStr);

	if (id == SpecifierType::SPEC_NONE().getId()) {
		result += _("<not specified>");
	}
	else if (id == SpecifierType::SPEC_NAME().getId()) {
		result += _("entity ") + _value;
	}
	else if (id == SpecifierType::SPEC_OVERALL().getId()) {
		result += printEntityAmount(amountStr);
	}
	else if (id == SpecifierType::SPEC_GROUP().getId()) {

		if (_value == "loot_gold") {
			result += fmt::format(_("{0:d} loot in gold"), amount);
		}
		else if (_value == "loot_goods") {
			result += fmt::format(_("{0:d} loot in goods"), amount);
		}
		else if (_value == "loot_jewels") {
			result += fmt::format(_("{0:d} loot in jewels"), amount);
		}
		else if (_value == "loot_total") {
			result += fmt::format(_("{0:d} loot"), amount);
		}
		else {
			result += fmt::format(_("{0:d} of \"{1}\""), amount, _value);
		}
	}
	else if (id == SpecifierType::SPEC_CLASSNAME().getId()) {
		result += fmt::format(_("{0} of type {1}"), printEntityAmount(amountStr), _value);
	}
	else if (id == SpecifierType::SPEC_SPAWNCLASS().getId()) {
		result += fmt::format(_("{0} of spawnclass {1}"), printEntityAmount(amountStr), _value);
	}
	else if (id == SpecifierType::SPEC_AI_TYPE().getId()) {
		result += fmt::format(_("{0} AI of type {1}"), amountStr, _value);
	}
	else if (id == SpecifierType::SPEC_AI_TEAM().getId()) {
		result += fmt::format(_("{0} AI of team {1}"), amountStr, _value);
	}
	else if (id == SpecifierType::SPEC_AI_INNOCENCE().getId()) {
		result += fmt::format(_("{0} AI of {1}"), amountStr, _value);
	}
	else {
		rError() << "Unknown specifier ID " << id << "found!" << std::endl;
	}

	return result;
}

} // namespace objectives
