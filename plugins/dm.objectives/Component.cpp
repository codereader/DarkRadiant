#include "Component.h"

#include "string/string.h"
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/replace.hpp>

namespace objectives {

std::string Component::getSpecifierSentence() {
	return "";
}

std::string Component::getString() {
	// This will hold our return value
	std::string sentence;

	int componentId = _type.getId();

	if (isInverted()) {
		sentence += "do NOT ";
	}

	SpecifierPtr sp1 = getSpecifier(Specifier::FIRST_SPECIFIER);
	SpecifierPtr sp2 = getSpecifier(Specifier::SECOND_SPECIFIER);

	if (componentId == ComponentType::COMP_KILL().getId()) {
		// First add the verb
		sentence += "kill";
		
		// Add the Specifier details, if any
		sentence += (sp1) ? (" " + sp1->getSentence(*this)) : "";
	}
	else if (componentId == ComponentType::COMP_KO().getId()) {
		// First add the verb
		sentence += "knockout";
				
		// Add the Specifier details, if any
		sentence += (sp1) ? (" " + sp1->getSentence(*this)) : "";
	}
	else if (componentId == ComponentType::COMP_AI_FIND_ITEM().getId()) {
		// First add the verb
		sentence += "let AI find item:";
		
		// Add the Specifier details, if any
		sentence += (sp1) ? (" " + sp1->getSentence(*this)) : "";
	}
	else if (componentId == ComponentType::COMP_AI_FIND_BODY().getId()) {
		sentence += "let AI find body:";

		// Add the Specifier details, if any
		sentence += (sp1) ? (" " + sp1->getSentence(*this)) : "";
	}
	else if (componentId == ComponentType::COMP_ALERT().getId()) {
		sentence += "alert";

		// Add the Specifier details, if any
		sentence += (sp1) ? (" " + sp1->getSentence(*this)) : "";

		// Add the alert level info
		sentence += " " + getArgument(0) + " times to a minimum alert level of " + getArgument(1);
	}
	else if (componentId == ComponentType::COMP_DESTROY().getId()) {
		sentence += "destroy";

		// Add the Specifier details, if any
		sentence += (sp1) ? (" " + sp1->getSentence(*this)) : "";
	}
	else if (componentId == ComponentType::COMP_ITEM().getId()) {
		sentence += "acquire";

		// Add the Specifier details, if any
		sentence += (sp1) ? (" " + sp1->getSentence(*this)) : "";
	}
	else if (componentId == ComponentType::COMP_PICKPOCKET().getId()) {
		sentence += "pickpocket";

		// Add the Specifier details, if any
		sentence += (sp1) ? (" " + sp1->getSentence(*this)) : "";
	}
	else if (componentId == ComponentType::COMP_LOCATION().getId()) {
		sentence += "let the target";
		
		// Add the Specifier details, if any
		sentence += (sp1) ? (" " + sp1->getSentence(*this)) : "";

		sentence += " be at location";
		sentence += (sp2) ? (" " + sp2->getSentence(*this)) : "";
	}
	else if (componentId == ComponentType::COMP_INFO_LOCATION().getId()) {

	}
	else if (componentId == ComponentType::COMP_CUSTOM_ASYNC().getId()) {
	}
	else if (componentId == ComponentType::COMP_CUSTOM_CLOCKED().getId()) {
		sentence += "Call the script function ";
		sentence += getArgument(0);

		if (getClockInterval() > 0) {
			sentence += " (time interval: " + floatToStr(getClockInterval()) + " seconds)";
		}
	}
	else if (componentId == ComponentType::COMP_DISTANCE().getId()) {
	}

	// Convert the first character of the sentence to upper case
	if (!sentence.empty()) {
		std::string c(sentence.begin(), sentence.begin()+1);
		sentence[0] = boost::algorithm::to_upper_copy(c)[0];
	}

	// Replace all double-space characters with one single space
	boost::algorithm::replace_all(sentence, "  ", " ");
    
    /*if (sp2) {
        if (sp1)
            retStr += ", ";
        retStr += sp2->getType().getDisplayName();
        retStr += " = " + sp2->getValue();
    }

    if (sp1 || sp2) {
        retStr += " ]";
    }*/

    return sentence;
}

} // namespace objectives
