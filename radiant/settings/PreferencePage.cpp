#include "PreferencePage.h"

#include <stdexcept>
#include "i18n.h"
#include "itextstream.h"
#include "string/split.h"
#include "string/join.h"
#include <fmt/format.h>

#include "PreferenceItems.h"

namespace settings
{

PreferencePage::PreferencePage(const std::string& name, const PreferencePagePtr& parentPage) :
	_name(name)
{
	_title = fmt::format(_("{0} Settings"), _name);

	// Construct the _path member value
	if (parentPage && !parentPage->getPath().empty())
	{
		_path = parentPage->getPath() + "/" + _name;
	}
	else
	{
		_path = _name;
	}
}

const std::string& PreferencePage::getTitle() const
{
	return _title;
}

void PreferencePage::setTitle(const std::string& title)
{
	_title = title;
}

const std::string& PreferencePage::getPath() const
{
	return _path;
}

const std::string& PreferencePage::getName() const
{
	return _name;
}

PreferencePage& PreferencePage::createOrFindPage(const std::string& path)
{
	// Split the path into parts
	std::list<std::string> parts;
	string::split(parts, path, "/");

	if (parts.empty())
	{
		rConsole() << "Cannot resolve empty preference path: " << path << std::endl;
		throw std::logic_error("Cannot resolve empty preference path.");
	}

	PreferencePagePtr child;

	// Try to lookup the page in the child list
	for (const PreferencePagePtr& candidate : _children)
	{
		if (candidate->getName() == parts.front())
		{
			child = candidate;
			break;
		}
	}

	if (!child)
	{
		// No child found, create a new page and add it to the list
		child = std::make_shared<PreferencePage>(parts.front(), shared_from_this());
		_children.push_back(child);
	}

	// We now have a child with this name, do we have a leaf?
	if (parts.size() > 1) 
	{
		// We have still more parts, split off the first part
		parts.pop_front();
		std::string subPath = string::join(parts, "/");

		// Pass the call to the child
		return child->createOrFindPage(subPath);
	}
	else 
	{
		// We have found a leaf, return the child page
		return *child;
	}
}

void PreferencePage::foreachChildPage(const std::function<void(PreferencePage&)>& functor)
{
	for (const PreferencePagePtr& child : _children)
	{
		// Visit this instance
		functor(*child);

		// Pass the visitor recursively
		child->foreachChildPage(functor);
	}
}

void PreferencePage::foreachItem(const std::function<void(const PreferenceItemBasePtr&)>& functor) const
{
	for (const PreferenceItemBasePtr& item : _items)
	{
		functor(item);
	}
}

bool PreferencePage::isEmpty() const
{
	return _items.empty();
}

void PreferencePage::appendCheckBox(const std::string& label, const std::string& registryKey)
{
	_items.push_back(std::make_shared<PreferenceCheckbox>(label, registryKey));
}

void PreferencePage::appendSlider(const std::string& name, const std::string& registryKey,
	double lower, double upper, double stepIncrement, double pageIncrement)
{
	_items.push_back(std::make_shared<PreferenceSlider>(name, registryKey, 
		lower, upper, stepIncrement, pageIncrement));
}

void PreferencePage::appendCombo(const std::string& name, const std::string& registryKey,
	const ComboBoxValueList& valueList, bool storeValueNotIndex)
{
	_items.push_back(std::make_shared<PreferenceCombobox>(name, registryKey, valueList, storeValueNotIndex));
}

void PreferencePage::appendEntry(const std::string& name, const std::string& registryKey)
{
	_items.push_back(std::make_shared<PreferenceEntry>(name, registryKey));
}

void PreferencePage::appendLabel(const std::string& caption)
{
	_items.push_back(std::make_shared<PreferenceLabel>(caption));
}

void PreferencePage::appendPathEntry(const std::string& name, const std::string& registryKey, bool browseDirectories)
{
	_items.push_back(std::make_shared<PreferencePathEntry>(name, registryKey, browseDirectories));
}

void PreferencePage::appendSpinner(const std::string& name, const std::string& registryKey,
	double lower, double upper, int fraction)
{
	_items.push_back(std::make_shared<PreferenceSpinner>(name, registryKey, lower, upper, fraction));
}

} // namespace
