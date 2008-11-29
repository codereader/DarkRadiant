#include "ConversationKeyExtractor.h"

#include "itextstream.h"

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/classification.hpp>

#include "string/string.h"

namespace conversation {

// Shortcut for boost::algorithm::split
typedef std::vector<std::string> StringParts;
	
// Required entity visit function
void ConversationKeyExtractor::visit(const std::string& key, const std::string& value) {
	// Quick discard of any non-conversation keys
	if (key.substr(0, 4) != "conv") return;
		
	// Extract the objective number
	static const boost::regex reConvNum("conv_(\\d+)_(.*)");
	boost::smatch results;
	int iNum;
	
	if (!boost::regex_match(key, results, reConvNum)) {
		// No match, abort
		return;
	}

	// Get the conversation number
	iNum = strToInt(results[1]);

	// We now have the conversation number and the substring (everything after
	// "conv_<n>_" which applies to this conversation.
	std::string convSubString = results[2];
	
	// Switch on the substring
	/*if (convSubString == "desc") {
		_objMap[iNum].description = value;			
	}
	else if (convSubString == "ongoing") {
		_objMap[iNum].ongoing = (value == "1");			
	}
	else if (convSubString == "mandatory") {
		_objMap[iNum].mandatory = (value == "1");			
	}
	else if (convSubString == "visible") {
		_objMap[iNum].visible = (value == "1");			
	}
	else if (convSubString == "irreversible") {
		_objMap[iNum].irreversible = (value == "1");			
	}
	else if (convSubString == "state") {
		_objMap[iNum].state = 
			static_cast<Objective::State>(strToInt(value));
	}
	else if (convSubString == "difficulty") {
		_objMap[iNum].difficultyLevels = value;
	}
	else if (convSubString == "enabling_objs") {
		_objMap[iNum].enablingObjs = value;
	}
	else if (convSubString == "script_complete") {
		_objMap[iNum].completionScript = value;
	}
	else if (convSubString == "script_failed") {
		_objMap[iNum].failureScript = value;
	}
	else if (convSubString == "logic_success") {
		_objMap[iNum].logic.successLogic = value;
	}
	else if (convSubString == "logic_failure") {
		_objMap[iNum].logic.failureLogic = value;
	}
	else {
	
		// Use another regex to check for components (obj1_1_blah)
		static const boost::regex reComponent("(\\d+)_(.*)");
		boost::smatch results;
		
		if (!boost::regex_match(convSubString, results, reComponent)) {
			return;
		}
		else {
			
			// Get the component number and key string
			int componentNum = strToInt(results[1]);
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
				comp.setClockInterval(strToFloat(value));
			}
			// Check for the spec_val first
			else if (boost::algorithm::starts_with(componentStr, "spec_val")) {
				// We have a component specifier value, see if the specifier itself is there already
				Specifier::SpecifierNumber specNum = getSpecifierNumber(
					strToInt(componentStr.substr(8), -1)
				);

				if (specNum == Specifier::MAX_SPECIFIERS) {
					globalErrorStream() << 
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
					strToInt(componentStr.substr(4), -1)
				);

				if (specNum == Specifier::MAX_SPECIFIERS) {
					globalErrorStream() << 
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
	}*/
}
	
} // namespace conversation
