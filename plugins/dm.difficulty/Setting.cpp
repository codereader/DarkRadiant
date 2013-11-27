#include "Setting.h"

#include "i18n.h"
#include "iregistry.h"
#include "gamelib.h"
#include <gtkmm/liststore.h>

namespace difficulty {

	namespace {
		const std::string GKEY_APPTYPE_IGNORE("/difficulty/appTypeIgnore");
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
			returnValue = game::current::getValue<std::string>(GKEY_APPTYPE_IGNORE);
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
		if (argument == game::current::getValue<std::string>(GKEY_APPTYPE_IGNORE))
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

const Setting::ListStoreColumns& Setting::getTreeModelColumns()
{
	static ListStoreColumns cols;
	return cols;
}

Glib::RefPtr<Gtk::TreeModel> Setting::getAppTypeStore()
{
	const ListStoreColumns& columns = getTreeModelColumns();
	Glib::RefPtr<Gtk::ListStore> store = Gtk::ListStore::create(columns);

	Gtk::TreeModel::Row row = *store->append();
	row[columns.name] = _("Assign");
	row[columns.type] = EAssign;

	row = *store->append();
	row[columns.name] = _("Add");
	row[columns.type] = EAdd;

	row = *store->append();
	row[columns.name] = _("Multiply");
	row[columns.type] = EMultiply;

	row = *store->append();
	row[columns.name] = _("Ignore");
	row[columns.type] = EIgnore;

	return store;
}

// Initialise the static member
int Setting::_highestId = 0;

} // namespace difficulty
