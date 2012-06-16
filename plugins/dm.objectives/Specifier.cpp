#include "Specifier.h"
#include "Component.h"

#include "i18n.h"
#include "itextstream.h"
#include "string/convert.h"
#include <boost/format.hpp>

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
			result += (boost::format(_("%d loot in gold")) % amount).str();
		}
		else if (_value == "loot_goods") {
			result += (boost::format(_("%d loot in goods")) % amount).str();
		}
		else if (_value == "loot_jewels") {
			result += (boost::format(_("%d loot in jewels")) % amount).str();
		}
		else if (_value == "loot_total") {
			result += (boost::format(_("%d loot")) % amount).str();
		}
		else {
			result += (boost::format(_("%d of \"%s\"")) % amount % _value).str();
		}
	}
	else if (id == SpecifierType::SPEC_CLASSNAME().getId()) {
		result += (boost::format(_("%s of type %s")) % printEntityAmount(amountStr) % _value).str();
	}
	else if (id == SpecifierType::SPEC_SPAWNCLASS().getId()) {
		result += (boost::format(_("%s of spawnclass %s")) % printEntityAmount(amountStr) % _value).str();
	}
	else if (id == SpecifierType::SPEC_AI_TYPE().getId()) {
		result += (boost::format(_("%s AI of type %s")) % amountStr % _value).str();
	}
	else if (id == SpecifierType::SPEC_AI_TEAM().getId()) {
		result += (boost::format(_("%s AI of team %s")) % amountStr % _value).str();
	}
	else if (id == SpecifierType::SPEC_AI_INNOCENCE().getId()) {
		result += (boost::format(_("%s AI of %s")) % amountStr % _value).str();
	}
	else {
		rError() << "Unknown specifier ID " << id << "found!" << std::endl;
	}

	return result;
}

} // namespace objectives
