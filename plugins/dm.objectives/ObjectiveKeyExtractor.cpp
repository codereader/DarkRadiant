#include "ObjectiveKeyExtractor.h"

#include "itextstream.h"

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/classification.hpp>

#include "string/convert.h"

namespace objectives {

// Shortcut for boost::algorithm::split
typedef std::vector<std::string> StringParts;

// Required entity visit function
void ObjectiveKeyExtractor::visit(const std::string& key,
								  const std::string& value)
{
	// Quick discard of any non-objective keys
	if (key.substr(0, 3) != "obj")
		return;

	// Extract the objective number
	static const boost::regex reObjNum("obj(\\d+)_(.*)");
	boost::smatch results;
	int iNum;

	if (boost::regex_match(key, results, reObjNum)) {
		// Get the objective number
		iNum = string::convert<int>(results[1]);
	}
	else {
		// No match, abort
		return;
	}

	// We now have the objective number and the substring (everything after
	// "obj<n>_" which applies to this objective.
	std::string objSubString = results[2];

	// Switch on the substring
	if (objSubString == "desc") {
		_objMap[iNum].description = value;
	}
	else if (objSubString == "ongoing") {
		_objMap[iNum].ongoing = (value == "1");
	}
	else if (objSubString == "mandatory") {
		_objMap[iNum].mandatory = (value == "1");
	}
	else if (objSubString == "visible") {
		_objMap[iNum].visible = (value == "1");
	}
	else if (objSubString == "irreversible") {
		_objMap[iNum].irreversible = (value == "1");
	}
	else if (objSubString == "state") {
		_objMap[iNum].state =
			static_cast<Objective::State>(string::convert<int>(value));
	}
	else if (objSubString == "difficulty") {
		_objMap[iNum].difficultyLevels = value;
	}
	else if (objSubString == "enabling_objs") {
		_objMap[iNum].enablingObjs = value;
	}
	else if (objSubString == "script_complete") {
		_objMap[iNum].completionScript = value;
	}
	else if (objSubString == "script_failed") {
		_objMap[iNum].failureScript = value;
	}
	else if (objSubString == "target_complete") {
		_objMap[iNum].completionTarget = value;
	}
	else if (objSubString == "target_failed") {
		_objMap[iNum].failureTarget = value;
	}
	else if (objSubString == "logic_success") {
		_objMap[iNum].logic.successLogic = value;
	}
	else if (objSubString == "logic_failure") {
		_objMap[iNum].logic.failureLogic = value;
	}
	else {

		// Use another regex to check for components (obj1_1_blah)
		static const boost::regex reComponent("(\\d+)_(.*)");
		boost::smatch results;

		if (!boost::regex_match(objSubString, results, reComponent)) {
			return;
		}
		else {

			// Get the component number and key string
			int componentNum = string::convert<int>(results[1]);
			std::string componentStr = results[2];

			Component& comp = _objMap[iNum].components[componentNum];

			// Switch on the key string
			if (componentStr == "type") {
				comp.setType(ComponentType::getComponentType(value));
			}
			else if (componentStr == "state") {
				comp.setSatisfied(value == "1");
			}
			else if (componentStr == "not") {
				comp.setInverted(value == "1");
			}
			else if (componentStr == "irreversible") {
				comp.setIrreversible(value == "1");
			}
			else if (componentStr == "player_responsible") {
				comp.setPlayerResponsible(value == "1");
			}
			else if (componentStr == "args") {
				// We have a component argument string
				StringParts parts;
				boost::algorithm::split(parts, value, boost::algorithm::is_any_of(" "));

				comp.clearArguments();

				// Add all found arguments to the component
				for (std::size_t i = 0; i < parts.size(); i++) {
					comp.addArgument(parts[i]);
				}
			}
			else if (componentStr == "clock_interval") {
				comp.setClockInterval(string::convert<float>(value));
			}
			// Check for the spec_val first
			else if (boost::algorithm::starts_with(componentStr, "spec_val")) {
				// We have a component specifier value, see if the specifier itself is there already
				Specifier::SpecifierNumber specNum = getSpecifierNumber(
					string::convert<int>(componentStr.substr(8), -1)
				);

				if (specNum == Specifier::MAX_SPECIFIERS) {
					rError() <<
						"[ObjectivesEditor]: Could not parse specifier value spawnarg " <<
						key << std::endl;
					return;
				}

				if (comp.getSpecifier(specNum) == NULL) {
					// No specifier exists yet, allocate a new one
					comp.setSpecifier(specNum, SpecifierPtr(new Specifier));
				}

				// Component exists, store the variable
				comp.getSpecifier(specNum)->setValue(value);
			}
			// if "check_val" didn't match, look for "spec" alone
			else if (boost::algorithm::starts_with(componentStr, "spec")) {
				// We have a component specifier, see which one
				Specifier::SpecifierNumber specNum = getSpecifierNumber(
					string::convert<int>(componentStr.substr(4), -1)
				);

				if (specNum == Specifier::MAX_SPECIFIERS) {
					rError() <<
						"[ObjectivesEditor]: Could not parse specifier spawnarg " <<
						key << std::endl;
					return;
				}

				if (comp.getSpecifier(specNum) == NULL) {
					// No specifier exists yet, allocate a new one
					comp.setSpecifier(specNum, SpecifierPtr(new Specifier));
				}

				// At this point, a specifier exists, set the parsed type
				comp.getSpecifier(specNum)->setType(
					SpecifierType::getSpecifierType(value)
				);
			}
		}
	}
}

Specifier::SpecifierNumber ObjectiveKeyExtractor::getSpecifierNumber(int specNum) {
	// greebo: specifier numbers start with 1 for user convenience,
	// but we need valid array indices starting from 0, so subtract 1
	specNum--;

	// Sanity-check the incoming index
	if (specNum < Specifier::FIRST_SPECIFIER || specNum >= Specifier::MAX_SPECIFIERS) {
		return Specifier::MAX_SPECIFIERS;
	}

	// Sanity-checks passed
	return static_cast<Specifier::SpecifierNumber>(specNum);
}

} // namespace objectives
