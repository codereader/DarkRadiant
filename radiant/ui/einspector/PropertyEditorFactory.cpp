#include "PropertyEditorFactory.h"

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
#include "FxPropertyEditor.h"

#include <regex>

#include "wxutil/Bitmap.h"

namespace ui
{

PropertyEditorFactory::PropertyEditorFactory()
{
    registerBuiltinTypes();
}

PropertyEditorFactory::~PropertyEditorFactory()
{
    _customEditors.clear();
    _peMap.clear();
    _dialogs.clear();
}

void PropertyEditorFactory::registerBuiltinTypes()
{
    _peMap["vector3"] = Vector3PropertyEditor::CreateNew;
    _peMap["bool"] = BooleanPropertyEditor::CreateNew;
    _peMap["entity"] = EntityPropertyEditor::CreateNew;
    _peMap["colour"] = ColourPropertyEditor::CreateNew;
    _peMap["color"] = ColourPropertyEditor::CreateNew;
    _peMap["texture"] = TexturePropertyEditor::CreateNew;
    _peMap["mat"] = TexturePropertyEditor::CreateNew;
    _peMap["skin"] = SkinPropertyEditor::CreateNew;
    _peMap["sound"] = SoundPropertyEditor::CreateNew;
    _peMap["float"] = FloatPropertyEditor::CreateNew;
    _peMap["model"] = ModelPropertyEditor::CreateNew;
    _peMap["classname"] = ClassnamePropertyEditor::CreateNew;
    _peMap["angle"] = AnglePropertyEditor::CreateNew;
    _peMap["fx"] = FxPropertyEditor::CreateNew;

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
    auto found = _customEditors.find(key);

    if (found != _customEditors.end())
    {
        _customEditors.erase(found);
        return;
    }

    rWarning() << "Cannot unregister property editor for key " << key << std::endl;
}

IPropertyEditor::Ptr PropertyEditorFactory::create(wxWindow* parent, const std::string& className,
    IEntitySelection& entities, const ITargetKey::Ptr& key)
{
    // greebo: First, search the custom editors for a match
    for (const auto& pair : _customEditors)
    {
        if (pair.first.empty()) continue; // skip empty keys

        // Try to match the entity key against the regex (i->first)
        std::regex expr(pair.first);
        std::smatch matches;

        auto fullKey = key->getFullKey();
        if (!std::regex_match(fullKey, matches, expr)) continue;

        // We have a match, invoke the creation function
        return pair.second(parent, entities, key);
    }

    // No custom editor found, search for the named property editor type
    auto iter = _peMap.find(className);

    // If the type is not found, return NULL otherwise create a new instance of
    // the associated derived type.
    if (iter == _peMap.end())
    {
        return {};
    }

    return iter->second(parent, entities, key);
}

void PropertyEditorFactory::registerPropertyEditorDialog(const std::string& key, const IPropertyEditorDialog::CreationFunc& create)
{
    auto result = _dialogs.emplace(key, create);

    if (!result.second)
    {
        rWarning() << "Could not register property editor dialog for key " << key
            << ", it is already associated." << std::endl;;
    }
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

    auto candidate = wxutil::GetLocalBitmap("icon_" + type + ".png");
    return candidate.IsOk() ? candidate : wxutil::GetLocalBitmap("empty.png");
}

}
