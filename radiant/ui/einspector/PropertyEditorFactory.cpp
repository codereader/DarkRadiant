#include "PropertyEditorFactory.h"

#include "isound.h"
#include "itextstream.h"
#include "Vector3PropertyEditor.h"
#include "BooleanPropertyEditor.h"
#include "EntityPropertyEditor.h"
#include "ColourPropertyEditor.h"
#include "TexturePropertyEditor.h"
#include "SkinPropertyEditor.h"
#include "SoundPropertyEditor.h"
#include "FloatPropertyEditor.h"
#include "ModelPropertyEditor.h"
#include "ClassnamePropertyEditor.h"
#include "AnglePropertyEditor.h"

#include <regex>

#include "wxutil/Bitmap.h"

namespace ui
{

// Initialisation
PropertyEditorFactory::PropertyEditorMap PropertyEditorFactory::_peMap;
PropertyEditorFactory::PropertyEditorMap PropertyEditorFactory::_customEditors;
std::map<std::string, IPropertyEditorDialog::CreationFunc> PropertyEditorFactory::_dialogs;

// Register the classes
void PropertyEditorFactory::registerClasses()
{
    _peMap["vector3"] = Vector3PropertyEditor::CreateNew;
    _peMap["bool"] = BooleanPropertyEditor::CreateNew;
    _peMap["entity"] = EntityPropertyEditor::CreateNew;
	_peMap["colour"] = ColourPropertyEditor::CreateNew;
	_peMap["color"] = ColourPropertyEditor::CreateNew;
	_peMap["texture"] = TexturePropertyEditor::CreateNew;
	_peMap["mat"] = TexturePropertyEditor::CreateNew;
	_peMap["skin"] = SkinPropertyEditor::CreateNew;

	if (module::GlobalModuleRegistry().moduleExists(MODULE_SOUNDMANAGER))
	{
		_peMap["sound"] = SoundPropertyEditor::CreateNew;
	}

	_peMap["float"] = FloatPropertyEditor::CreateNew;
	_peMap["model"] = ModelPropertyEditor::CreateNew;
	_peMap["classname"] = ClassnamePropertyEditor::CreateNew;
    _peMap["angle"] = AnglePropertyEditor::CreateNew;

    _dialogs["skin"] = []() { return std::make_shared<SkinChooserDialogWrapper>(); };
}

void PropertyEditorFactory::registerPropertyEditor(const std::string& key, const IPropertyEditor::CreationFunc& creator)
{
    auto result = _customEditors.emplace(key, creator);

    if (!result.second)
    {
        rWarning() << "Could not register property editor for key " << key
            << ", it is already associated." << std::endl;;
    }
}

void PropertyEditorFactory::unregisterPropertyEditor(const std::string& key)
{
	PropertyEditorMap::iterator found = _customEditors.find(key);

	if (found != _customEditors.end())
	{
		_customEditors.erase(found);
	}
	else
	{
		rWarning() << "Cannot unregister property editor for key " << key << std::endl;
	}
}

// Create a PropertyEditor from the given name.
IPropertyEditor::Ptr PropertyEditorFactory::create(wxWindow* parent, const std::string& className,
    IEntitySelection& entities, const std::string& key, const std::string& options)
{
    // Register the PropertyEditors if the map is empty
    if (_peMap.empty()) {
        registerClasses();
    }

	// greebo: First, search the custom editors for a match
	for (PropertyEditorMap::const_iterator i = _customEditors.begin();
		 i != _customEditors.end(); ++i)
	{
		if (i->first.empty()) continue; // skip empty keys

		// Try to match the entity key against the regex (i->first)
		std::regex expr(i->first);
		std::smatch matches;

		if (!std::regex_match(key, matches, expr)) continue;

		// We have a match
		return i->second(parent, entities, key, options);
	}

	// No custom editor found, search for the named property editor type
    auto iter = _peMap.find(className);

    // If the type is not found, return NULL otherwise create a new instance of
    // the associated derived type.
    if (iter == _peMap.end()) 
    {
        return {};
    } 
    
    return iter->second(parent, entities, key, options);
}

void PropertyEditorFactory::registerPropertyEditorDialog(const std::string& key, const IPropertyEditorDialog::CreationFunc& create)
{
    _dialogs.emplace(key, create);
}

IPropertyEditorDialog::Ptr PropertyEditorFactory::createDialog(const std::string& key)
{
    auto dialog = _dialogs.find(key);

    return dialog != _dialogs.end() ? dialog->second() : IPropertyEditorDialog::Ptr();
}

void PropertyEditorFactory::unregisterPropertyEditorDialog(const std::string& key)
{
    _dialogs.erase(key);
}

wxBitmap PropertyEditorFactory::getBitmapFor(const std::string& type)
{
	// Sanity check
	if (type.empty()) return wxNullBitmap;

	std::string iconName = "icon_" + type + ".png";

	wxBitmap candidate = wxutil::GetLocalBitmap(iconName);

	if (!candidate.IsOk())
	{
		candidate = wxutil::GetLocalBitmap("empty.png");
	}

	return candidate;
}

}
