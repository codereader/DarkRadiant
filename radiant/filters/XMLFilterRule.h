#ifndef XMLFILTERRULE_H_
#define XMLFILTERRULE_H_

#include <string>

namespace filters
{

/** Data class encapsulating a single filter rule. Several of these rules may make
 * up a single XMLFilter.
 */
 
struct XMLFilterRule {

	std::string type; 	// "texture", "entityclass" or "object"
	std::string match; 	// the match expression regex
	bool show;			// true for action="show", false for action="hide"
	
	// Constructor
	XMLFilterRule(const std::string t, const std::string m, bool s)
	: type(t), match(m), show(s) 
	{
	}
};

}

#endif /*XMLFILTERRULE_H_*/
