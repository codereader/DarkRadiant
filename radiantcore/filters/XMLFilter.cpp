#include "XMLFilter.h"

#include "ientity.h"
#include "ieclass.h"
#include "ifilter.h"
#include <regex>
#include <algorithm>

namespace filters
{

XMLFilter::XMLFilter(const std::string& name, bool readOnly) :
	_name(name),
	_readonly(readOnly)
{
	updateEventName();
}

// Test visibility of an item against all rules
bool XMLFilter::isVisible(const FilterRule::Type type, const std::string& name) const
{
	// Iterate over the rules in this filter, checking if each one is a rule for
	// the chosen item. If so, test the match expression and retrieve the visibility
	// flag if there is a match.

	bool visible = true; // default if unmodified by rules

	for (FilterRules::const_iterator ruleIter = _rules.begin();
		 ruleIter != _rules.end();
		 ++ruleIter)
	{
		// Check the item type.
		if (ruleIter->type != type)
		{
			continue;
		}

		// If we have a rule for this item, use a regex to match the query name
		// against the "match" parameter
		std::regex ex(ruleIter->match);

		if (std::regex_match(name, ex))
		{
			// Overwrite the visible flag with the value from the rule.
			visible = ruleIter->show;
		}
	}

	// Pass back the current visibility value
	return visible;
}

bool XMLFilter::isEntityVisible(const FilterRule::Type type, const Entity& entity) const
{
	bool visible = true; // default if unmodified by rules

	IEntityClassConstPtr eclass = entity.getEntityClass();
	
	for (FilterRules::const_iterator ruleIter = _rules.begin();
		 ruleIter != _rules.end();
		 ++ruleIter)
	{
		if (ruleIter->type != type)
		{
			continue;
		}

		if (type == FilterRule::TYPE_ENTITYCLASS)
		{
			std::regex ex(ruleIter->match);

			if (std::regex_match(eclass->getDeclName(), ex))
			{
				visible = ruleIter->show;
			}
		}
		else if (type == FilterRule::TYPE_ENTITYKEYVALUE)
		{
			std::regex ex(ruleIter->match);

			if (std::regex_match(entity.getKeyValue(ruleIter->entityKey), ex))
			{
				visible = ruleIter->show;
			}
		}
	}

	return visible;
}

const std::string& XMLFilter::getEventName() const {
	return _eventName;
}

const std::string& XMLFilter::getName() const
{
	return _name;
}

void XMLFilter::setName(const std::string& newName) {
	// Set the name ...
	_name = newName;

	// ...and update the event name
	updateEventName();
}

bool XMLFilter::isReadOnly() const {
	return _readonly;
}

const FilterRules& XMLFilter::getRuleSet() const
{
	return _rules;
}

void XMLFilter::setRules(const FilterRules& rules) {
	_rules = rules;
}

void XMLFilter::updateEventName() {
	// Construct the eventname out of the filtername (strip the spaces and add "Filter" prefix)
	_eventName = _name;

	// Strip all spaces from the string
	_eventName.erase(std::remove(_eventName.begin(), _eventName.end(), ' '), _eventName.end());

	_eventName = "Filter" + _eventName;
}

} // namespace filters
