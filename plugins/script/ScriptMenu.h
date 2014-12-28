#ifndef _SCRIPT_MENU_H_
#define _SCRIPT_MENU_H_

#include "ScriptCommand.h"

namespace ui
{

/**
 * A class managing the various script commands in the main menu.
 */
class ScriptMenu
{
public:
	ScriptMenu(const script::ScriptCommandMap& commands);

	~ScriptMenu();
};
typedef std::shared_ptr<ScriptMenu> ScriptMenuPtr;

} // namespace ui

#endif /* _SCRIPT_MENU_H_ */
