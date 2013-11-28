#pragma once

#include <gtkmm/checkbutton.h>

namespace ui
{

/**
 * An enhanced checkbox that is updating the named
 * entity property (spawnarg) when toggled.
 *
 * The logic toggled = "1" can be optionally inversed such that 
 * an unchecked box reflects a property value of "1".
 */
class SpawnargLinkedCheckbox : 
	public Gtk::CheckButton
{
private:
	bool _inverseLogic;

	std::string _propertyName;

public:
	SpawnargLinkedCheckbox(const std::string& label, 
						   const std::string& propertyName, 
						   bool inverseLogic = false) :
		Gtk::CheckButton(label),
		_inverseLogic(inverseLogic)
	{}

	void loadFrom(Entity* entity)
	{
		bool value = entity->getKeyValue(_propertyName) == "1";

		set_active(_inverseLogic ? !value : value);
	}

	void saveTo(Entity* entity)
	{
		if (_inverseLogic)
		{
			entity->setKeyValue(_propertyName, get_active() ? "0" : "1"); // Active => "0"
		}
		else
		{
			entity->setKeyValue(_propertyName, get_active() ? "1" : "0");
		}
	}
};

} // namespace
