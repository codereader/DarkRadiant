#include "XMLFilter.h"

#include <boost/regex.hpp>

namespace filters {

// Test visibility of a texture against all rules

bool XMLFilter::isTextureVisible(const std::string& texture) const {

	// Iterate over the rules in this filter, checking if each one is a "texture"
	// rule. If it is, test the query texture against the match expression using
	// a regex.

	bool visible = true; // default if unmodified by rules
	
	for (RuleList::const_iterator ruleIter = _rules.begin();
		 ruleIter != _rules.end();
		 ++ruleIter)
	{
		// We are only interested in texture rules
		if (ruleIter->type != "texture")
			continue;
			
		// If we have a texture rule, use boost's regex to match the query texture
		// against the "match" parameter
		boost::regex ex(ruleIter->match);
		if (boost::regex_match(texture, ex)) {
			// Overwrite the visible flag with the value from the rule.
			visible = ruleIter->show;	
		}
	}
	
	// Pass back the current visibility value
	return visible;
}	
	

	
} // namespace filters
