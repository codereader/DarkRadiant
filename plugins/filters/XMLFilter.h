#ifndef XMLFILTER_H_
#define XMLFILTER_H_

#include <string>
#include <vector>
#include "ifilter.h"

namespace filters
{

/** Class encapsulting a single filter. This consists of a name, and a list of
 * filter rules, and methods to query textures, entityclasses and objects against
 * these rules.
 */
 
class XMLFilter {
private:

	// Text name of filter (from game.xml)
	std::string _name;
	
	// The name of the toggle event
	std::string _eventName;
	
	// Ordered list of rule objects
	FilterRules _rules;

	// True if this filter can't be changed
	bool _readonly;

public:

	/** Construct an XMLFilter with the given name.
	 * Pass the read-only flag to indicate whether this filter is
	 * custom or coming from the "stock" filters in the .game files.
	 */
	XMLFilter(const std::string& name, bool readOnly);
	
	/** Add a rule to this filter.
	 * 
	 * @param type
	 * The type of rule - "texture", "entityclass" or "object".
	 * 
	 * @param match
	 * The regex match expression to use for this rule.
	 * 
	 * @param show
	 * true if this filter should show its matches, false if it should
	 * hide them.
	 */
	void addRule(const std::string& type, const std::string& match, bool show) {
		_rules.push_back(FilterRule(type, match, show));
	}
	
	/** Test a given item for visibility against all of the rules
	 * in this XMLFilter.
	 * 
	 * @param itemClass
	 * Class of the item to test - "texture", "entityclass" etc
	 * 
	 * @param name
	 * String name of the item to test.
	 */
	bool isVisible(const std::string& itemClass, const std::string& texture) const;
	
	/** greebo: Returns the name of the toggle event associated to this filter
	 */
	std::string getEventName() const;

	/**
	 * greebo: Renames the event to <newName>. This also updates the event name.
	 */
	void setName(const std::string& newName);
	
	/** greebo: Gets called when the associated Event is fired. 
	 */
	void toggle();

	// Whether this filter is read-only
	bool isReadOnly() const;

	// Returns the ruleset
	FilterRules getRuleSet();

	// Applies the given ruleset, replacing the existing one.
	void setRules(const FilterRules& rules);

private:
	void updateEventName();
};


}

#endif /*XMLFILTER_H_*/
