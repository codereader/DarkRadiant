#include "Setting.h"

#include "iregistry.h"

namespace difficulty {

	namespace {
		const std::string RKEY_APPTYPE_IGNORE("game/difficulty/appTypeIgnore");
	}

Setting::Setting() :
	isDefault(false)
{}

std::string Setting::getDescString() const {
	std::string returnValue(argument);

	switch (appType) {
		case EAssign: break;
		case EAdd: 
			returnValue = "+" + returnValue; 
			break;
		case EMultiply: 
			returnValue = "*" + returnValue;
			break;
		case EIgnore:
			returnValue = "IGNORE";
			break;
	};

	returnValue = spawnArg + " = " + returnValue;
	return returnValue;
}

void Setting::parseAppType() {
	appType = EAssign;

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
