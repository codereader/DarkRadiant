#include "XMLFilter.h"

#include "ifilter.h"
#include <boost/regex.hpp>
#include <boost/algorithm/string/erase.hpp>

namespace filters {

XMLFilter::XMLFilter(const std::string& name) : 
	_name(name)
{
	// Construct the eventname out of the filtername (strip the spaces and add "Filter" prefix)
	_eventName = _name;
	boost::algorithm::erase_all(_eventName, " ");
	_eventName = "Filter" + _eventName;
}

// Test visibility of an item against all rules
bool XMLFilter::isVisible(const std::string& item, const std::string& name) const {

	// Iterate over the rules in this filter, checking if each one is a rule for
	// the chosen item. If so, test the match expression and retrieve the visibility
	// flag if there is a match.

	bool visible = true; // default if unmodified by rules
	
	for (RuleList::const_iterator ruleIter = _rules.begin();
		 ruleIter != _rules.end();
		 ++ruleIter)
	{
		// Check the item type.
		if (ruleIter->type != item)
			continue;
			
		// If we have a rule for this item, use boost's regex to match the query name
		// against the "match" parameter
		boost::regex ex(ruleIter->match);
		if (boost::regex_match(name, ex)) {
			// Overwrite the visible flag with the value from the rule.
			visible = ruleIter->show;	
		}
	}
	
	// Pass back the current visibility value
	return visible;
}

// The command target
void XMLFilter::toggle() {
	// Allocate a reference, otherwise the call to GlobalFilterSystem() will crash
	bool currentState = GlobalFilterSystem().getFilterState(_name);
	GlobalFilterSystem().setFilterState(_name, !currentState);
}

std::string XMLFilter::getEventName() const {
	return _eventName;
}
	
} // namespace filters
