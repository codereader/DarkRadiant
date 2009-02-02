#ifndef _FLOATING_LAYOUT_H_
#define _FLOATING_LAYOUT_H_

#include "imainframelayout.h"

namespace ui {

#define FLOATING_LAYOUT_NAME "Floating"

class FloatingLayout;
typedef boost::shared_ptr<FloatingLayout> FloatingLayoutPtr;

class FloatingLayout :
	public IMainFrameLayout
{
public:
	// IMainFrameLayout implementation
	virtual std::string getName();
	virtual void activate();
	virtual void deactivate();

	// The creation function, needed by the mainframe layout manager
	static FloatingLayoutPtr CreateInstance();
};

} // namespace ui

#endif /* _FLOATING_LAYOUT_H_ */
