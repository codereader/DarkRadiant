#pragma once

namespace ui
{

/**
 * A class managing the various script commands in the main menu.
 */
class ScriptMenu
{
public:
	ScriptMenu();

	~ScriptMenu();
};
typedef std::shared_ptr<ScriptMenu> ScriptMenuPtr;

} // namespace ui
