#include "Setting.h"

#include "iregistry.h"

namespace difficulty {

	namespace {
		const std::string RKEY_APPTYPE_IGNORE("game/difficulty/appTypeIgnore");
	}

void Setting::parseAppType() {
	if (!argument.empty())
	{
		// Check for ignore argument
		if (argument == GlobalRegistry().get(RKEY_APPTYPE_IGNORE))
		{
			appType = EIgnore;
			argument.clear(); // clear the argument
		}
		// Check for special modifiers
		else if (argument[0] == '+')
		{
			appType = EAdd;
			// Remove the first character
			argument = argument.substr(1);
		}
		else if (argument[0] == '*')
		{
			appType = EMultiply;
			// Remove the first character
			argument = argument.substr(1);
		}
		else if (argument[0] == '-')
		{
			appType = EAdd;
			// Leave the "-" sign, it will be the sign of the parsed int
		}
	}
}

} // namespace difficulty
