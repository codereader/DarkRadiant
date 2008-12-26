#ifndef _UI_FILTER_H_
#define _UI_FILTER_H_

#include <string>
#include <boost/shared_ptr.hpp>

namespace ui {

// This object represents a filter in the filtersystem.
class Filter
{
	// The name
	std::string _name;

	// Filter is enabled or disabled
	bool _state;

	// Whether this is read-only
	bool _readonly;

public:
	Filter(const std::string& name, bool state, bool readonly) :
		 _name(name),
		 _state(state),
		 _readonly(readonly)
	{}

	const std::string& getName() const {
		return _name;
	}

	bool getState() const {
		return _state;
	}

	bool isReadOnly() const {
		return _readonly;
	}
};
typedef boost::shared_ptr<Filter> FilterPtr;

} // namespace ui

#endif /* _UI_FILTER_H_ */
