#include "Component.h"

#include "string/convert.h"
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/replace.hpp>

namespace objectives {

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

		sentence += " (amount: " + getArgument(0) + ")";
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
		sentence += "let the target";

		// Add the Specifier details, if any
		sentence += (sp1) ? (" " + sp1->getSentence(*this)) : "";

		sentence += " be at info_location";
		sentence += (sp2) ? (" " + sp2->getSentence(*this)) : "";
	}
	else if (componentId == ComponentType::COMP_CUSTOM_ASYNC().getId()) {
		sentence += "controlled by external script";
	}
	else if (componentId == ComponentType::COMP_CUSTOM_CLOCKED().getId()) {
		sentence += "call the script function ";
		sentence += getArgument(0);
	}
	else if (componentId == ComponentType::COMP_DISTANCE().getId()) {
		sentence += "let the entities " + getArgument(0);
		sentence += " and " + getArgument(1) + " get closer than ";
		sentence += getArgument(2) + " units";
	}
	else if (componentId == ComponentType::COMP_READABLE_OPENED().getId())
	{
		sentence += "open the readable ";
		sentence += (sp1) ? (" " + sp1->getSentence(*this)) : "";
	}
	else if (componentId == ComponentType::COMP_READABLE_CLOSED().getId())
	{
		sentence += "close the readable ";
		sentence += (sp1) ? (" " + sp1->getSentence(*this)) : "";
	}
	else if (componentId == ComponentType::COMP_READABLE_PAGE_REACHED().getId())
	{
		sentence += "view page " + getArgument(0) + " of readable ";
		sentence += (sp1) ? (" " + sp1->getSentence(*this)) : "";
	}

	if (getClockInterval() > 0) {
		sentence += " (check interval: " + string::to_string(getClockInterval()) + " seconds)";
	}

	// Convert the first character of the sentence to upper case
	if (!sentence.empty()) {
		std::string c(sentence.begin(), sentence.begin()+1);
		sentence[0] = boost::algorithm::to_upper_copy(c)[0];

		// Append a full stop at the end of the sentence
		if (sentence[sentence.length() - 1] != '.') {
			sentence.append(".");
		}
	}

	// Replace all double-space characters with one single space
	boost::algorithm::replace_all(sentence, "  ", " ");

    return sentence;
}

} // namespace objectives
