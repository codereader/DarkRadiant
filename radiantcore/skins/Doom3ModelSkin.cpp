#include "Doom3ModelSkin.h"

#include <sstream>

namespace skins
{

Skin::Skin(const std::string& name) :
    EditableDeclaration<decl::ISkin>(decl::Type::Skin, name),
    _original(std::make_shared<SkinData>()),
    _current(_original)
{}

std::string Skin::getName() const
{
	return getDeclName();
}

std::string Skin::getSkinFileName() const
{
	return getDeclFilePath();
}

const std::set<std::string>& Skin::getModels()
{
    ensureParsed();

    return _current->matchingModels;
}

void Skin::addModel(const std::string& model)
{
    if (_current->matchingModels.count(model) == 0)
    {
        ensureSkinDataBackup();

        _current->matchingModels.insert(model);
        onParsedContentsChanged();
    }
}

void Skin::removeModel(const std::string& model)
{
    if (_current->matchingModels.count(model) > 0)
    {
        ensureSkinDataBackup();

        _current->matchingModels.erase(model);
        onParsedContentsChanged();
    }
}

std::string Skin::getRemap(const std::string& name)
{
    ensureParsed();

    // The remaps are applied in the order they appear in the decl
    for (const auto& remapping : _current->remaps)
    {
        if (remapping.Original == "*" || remapping.Original == name)
        {
            return remapping.Replacement;
        }
    }

    return {};
}

void Skin::addRemap(const std::string& src, const std::string& dst)
{
    ensureSkinDataBackup();

    _current->remaps.emplace_back(Remapping{ src, dst });
}

void Skin::foreachMatchingModel(const std::function<void(const std::string&)>& functor)
{
    ensureParsed();

    for (const auto& model : _current->matchingModels)
    {
        functor(model);
    }
}

const std::vector<decl::ISkin::Remapping>& Skin::getAllRemappings()
{
    return _current->remaps;
}

void Skin::addRemapping(const Remapping& remapping)
{
    ensureParsed();

    for (const auto& existing : _current->remaps)
    {
        if (existing.Original == remapping.Original && existing.Replacement == remapping.Replacement)
        {
            return; // duplicate found, do nothing
        }
    }

    ensureSkinDataBackup();
    _current->remaps.push_back(remapping);
    onParsedContentsChanged();
}

void Skin::removeRemapping(const std::string& material)
{
    ensureParsed();

    // Check if there's any remap (otherwise avoid marking this skin as modified)
    bool hasRemap = false;
    for (auto it = _current->remaps.begin(); it != _current->remaps.end(); ++it)
    {
        if (it->Original != material) continue;

        hasRemap = true;
        break;
    }

    if (!hasRemap) return;

    ensureSkinDataBackup();

    // Remove that first matching remap
    for (auto it = _current->remaps.begin(); it != _current->remaps.end(); ++it)
    {
        if (it->Original != material) continue;

        _current->remaps.erase(it);
        onParsedContentsChanged();
        break;
    }
}

void Skin::clearRemappings()
{
    ensureParsed();

    if (_current->remaps.empty()) return; // nothing to do

    ensureSkinDataBackup();
    _current->remaps.clear();
    onParsedContentsChanged();
}

bool Skin::isModified()
{
    return _current != _original;
}

void Skin::setIsModified()
{
    if (!isModified())
    {
        ensureSkinDataBackup();
        signal_DeclarationChanged().emit();
    }
}

void Skin::commitModifications()
{
    _original = _current;
    onParsedContentsChanged();
}

void Skin::revertModifications()
{
    auto currentName = getDeclName();

    // Reverting the name to its original should notify the decl manager
    if (currentName != getOriginalDeclName())
    {
        // This will in turn call getDeclName()
        GlobalDeclarationManager().renameDeclaration(decl::Type::Skin, currentName, getOriginalDeclName());
    }

    _current = _original;
    onParsedContentsChanged();
}

void Skin::ensureSkinDataBackup()
{
    ensureParsed();

    if (_current != _original)
    {
        return; // copy is already in place
    }

    // Create a copy of the original struct
    _current = std::make_shared<SkinData>(*_original);
}

void Skin::onBeginParsing()
{
    _current->remaps.clear();
    _current->matchingModels.clear();
}

void Skin::parseFromTokens(parser::DefTokeniser& tokeniser)
{
    // [ "skin" ] <name>
    // "{"
    //      [ "model" <modelname> ]
    //      ( <sourceTex> <destTex> )*
    //      [ * <destTex> ]
    // "}"
    while (tokeniser.hasMoreTokens())
    {
        // Read key/value pairs until end of decl
        auto key = tokeniser.nextToken();
        auto value = tokeniser.nextToken();

        // If this is a model key, add to the model->skin map, otherwise assume
        // this is a remap declaration
        if (key == "model")
        {
            _current->matchingModels.insert(value);
        }
        else
        {
            // Add the pair, preserving any wildcards "*"
            _current->remaps.emplace_back(Remapping{ std::move(key), std::move(value) });
        }
    }
}

std::string Skin::generateSyntax()
{
    std::stringstream output;

    output << std::endl;

    // Export models (indentation one tab)
    for (const auto& model : _current->matchingModels)
    {
        output << "\tmodel\t\"" << model << "\"" << std::endl;
    }

    if (!_current->matchingModels.empty() && !_current->remaps.empty())
    {
        output << std::endl;
    }

    // Export remaps
    for (const auto& remap : _current->remaps)
    {
        output << "\t\"" << remap.Original << "\"\t\"" << remap.Replacement << "\"" << std::endl;
    }

    return output.str();
}

}
