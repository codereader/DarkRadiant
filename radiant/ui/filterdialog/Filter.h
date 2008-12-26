#ifndef _UI_FILTER_H_
#define _UI_FILTER_H_

#include "ifilter.h"
#include <vector>
#include <boost/shared_ptr.hpp>

namespace ui {

// This object represents a filter in the filtersystem.
class Filter
{
public:

	// The name
	std::string name;

	// Filter is enabled or disabled
	bool state;

	// Whether this is read-only
	bool readOnly;

	// The rules defining this filter
	FilterRules rules;

	Filter(const std::string& _name, const bool _state, const bool _readOnly) :
		name(_name),
		state(_state),
		readOnly(_readOnly)
	{}
};
typedef boost::shared_ptr<Filter> FilterPtr;

} // namespace ui

#endif /* _UI_FILTER_H_ */
