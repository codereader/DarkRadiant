#ifndef XMLFILTER_H_
#define XMLFILTER_H_

#include "XMLFilterRule.h"

#include <string>
#include <vector>

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
	
	// Ordered list of rule objects
	typedef std::vector<XMLFilterRule> RuleList;
	RuleList _rules;

public:

	/** Construct an XMLFilter with the given name.
	 */
	XMLFilter(const std::string n)
	: _name(n) {}
	
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
		_rules.push_back(XMLFilterRule(type, match, show));	
	}
	
	/** Test a given texture for visibility against all of the rules
	 * in this XMLFilter.
	 * 
	 * @param texture
	 * String name of the texture to test.
	 */
	bool isTextureVisible(const std::string& texture) const;
};


}

#endif /*XMLFILTER_H_*/
