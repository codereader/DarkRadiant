#include "Setting.h"

#include "iregistry.h"
#include <gtk/gtkliststore.h>

namespace difficulty {

	namespace {
		const std::string RKEY_APPTYPE_IGNORE("game/difficulty/appTypeIgnore");
	}

Setting::Setting() :
	id(++_highestId),
	isDefault(false)
{}

std::string Setting::getArgumentKeyValue() const {
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
			returnValue = GlobalRegistry().get(RKEY_APPTYPE_IGNORE);
			break;
		default:
			break;
	};

	return returnValue;
}

std::string Setting::getDescString() const {
	std::string returnValue(argument);

	switch (appType) {
		case EAssign: 
			returnValue = " = " + returnValue;
			break;
		case EAdd: 
			returnValue = " += " + returnValue; 
			break;
		case EMultiply: 
			returnValue = " *= " + returnValue;
			break;
		case EIgnore:
			returnValue = " = [IGNORE]";
			break;
		default:
			break;
	};

	returnValue = spawnArg + returnValue;
	return returnValue;
}

bool Setting::operator==(const Setting& rhs) const {
	return className == rhs.className && 
			spawnArg == rhs.spawnArg &&
			argument == rhs.argument &&
			appType == rhs.appType;
}

bool Setting::operator!=(const Setting& rhs) const {
	return !operator==(rhs);
}

Setting& Setting::operator=(const Setting& rhs) {
	className = rhs.className;
	spawnArg = rhs.spawnArg;
	argument = rhs.argument;
	appType = rhs.appType;
	isDefault = rhs.isDefault;

	return *this;
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

GtkListStore* Setting::getAppTypeStore() {
	GtkListStore* store = gtk_list_store_new(2, 
		G_TYPE_STRING, // the caption
		G_TYPE_INT     // the enum int
	);

	GtkTreeIter iter;
	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 0, "Assign", 1, EAssign, -1);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 0, "Add", 1, EAdd, -1);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 0, "Multiply", 1, EMultiply, -1);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 0, "Ignore", 1, EIgnore, -1);

	return store;
}

// Initialise the static member
int Setting::_highestId = 0;

} // namespace difficulty
