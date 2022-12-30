#include "EntityClass.h"

#include "itextstream.h"
#include "ieclasscolours.h"
#include "string/convert.h"

#include "string/predicate.h"
#include <functional>

namespace eclass
{

namespace
{
    const Vector3 DefaultEntityColour(0.3, 0.3, 1);
    const Vector4 UndefinedColour(-1, -1, -1, -1);
}

EntityClass::EntityClass(const std::string& name)
: DeclarationBase<IEntityClass>(decl::Type::EntityDef, name),
  _visibility([this] { return determineVisibilityFromValues(); }),
  _colour(DefaultEntityColour),
  // greebo: Changed default behaviour when unknown entites are encountered to isFixedSize == FALSE
  // so that brushes of unknown classes don't get lost (issue #240)
  _fixedSize(false)
{}

EntityClass::~EntityClass()
{
    _parentChangedConnection.disconnect();
}

IEntityClass* EntityClass::getParent()
{
    ensureParsed();

    return _parent;
}

vfs::Visibility EntityClass::determineVisibilityFromValues()
{
    // Entity class visibility is NOT inherited -- hiding an abstract base entity from the list
    // does not imply all of its concrete subclasses should also be hidden.
    return getAttributeValue("editor_visibility", false) == "hidden" ? 
        vfs::Visibility::HIDDEN : vfs::Visibility::NORMAL;
}

vfs::Visibility EntityClass::getVisibility()
{
    ensureParsed();

    // File visibility overrides the setting in the entity key/value pairs
    return getBlockSyntax().fileInfo.visibility == vfs::Visibility::HIDDEN ?
        vfs::Visibility::HIDDEN : _visibility.get();
}

sigc::signal<void>& EntityClass::changedSignal()
{
    return _changedSignal;
}

void EntityClass::onSyntaxBlockAssigned(const decl::DeclarationBlockSyntax& block)
{
    DeclarationBase<IEntityClass>::onSyntaxBlockAssigned(block);

    clear();
    emitChangedSignal();
}

bool EntityClass::isFixedSize()
{
    ensureParsed();

    if (_fixedSize) {
        return true;
    }
    else {
        // Check for the existence of editor_mins/maxs attributes, and that
        // they do not contain only a question mark
        return (getAttributeValue("editor_mins").size() > 1
                && getAttributeValue("editor_maxs").size() > 1);
    }
}

AABB EntityClass::getBounds()
{
    ensureParsed();

    if (isFixedSize())
    {
        return AABB::createFromMinMax(
            string::convert<Vector3>(getAttributeValue("editor_mins")),
            string::convert<Vector3>(getAttributeValue("editor_maxs"))
        );
    }

    return AABB(); // null AABB
}

EntityClass::Type EntityClass::getClassType()
{
    ensureParsed();

    if (isLight())
    {
        return Type::Light;
    }

    if (!isFixedSize())
    {
        // Variable size entity
        return Type::StaticGeometry;
    }
    
    if (!getAttributeValue("model").empty())
    {
        // Fixed size, has model path
        return Type::EntityClassModel;
    }
    
    if (getDeclName() == "speaker")
    {
        return Type::Speaker;
    }

    return Type::Generic;
}

bool EntityClass::isLight()
{
    ensureParsed();

    return _isLight;
}

void EntityClass::setIsLight(bool val)
{
    _isLight = val;
    if (_isLight)
        _fixedSize = true;
}

void EntityClass::setColour(const Vector4& colour)
{
    ensureParsed();

    auto origColour = _colour;
    _colour = colour;

    // Set the entity colour to default, if none was specified
    if (_colour == UndefinedColour)
    {
        _colour = DefaultEntityColour;
    }

    // Emit the signal if the colour actually changed
    if (origColour != _colour)
        emitChangedSignal();
}

void EntityClass::resetColour()
{
    ensureParsed();

    // An override colour which matches this exact class is final, and overrides
    // everything else
    if (GlobalEclassColourManager().applyColours(*this))
        return;

    // Look for an editor_color on this class only
    const std::string colStr = getAttributeValue("editor_color", false);
    if (!colStr.empty())
    {
        // Set alpha to 0.5 if editor_transparent is set
        Vector4 colour(string::convert<Vector3>(colStr), _colourTransparent ? 0.5f : 1.0f);
        setColour(colour);
        return;
    }

    // If there is a parent, inherit its getColour() directly, which takes into
    // account any EClassColourManager overrides at the parent level.
    if (_parent)
        return setColour(_parent->getColour());

    // No parent and no attribute, all we can use is the default colour
    setColour(DefaultEntityColour);
}

const Vector4& EntityClass::getColour()
{
    ensureParsed();

    return _colour;
}

/* ATTRIBUTES */

/**
 * Insert an EntityClassAttribute, doesn't overwrite previous values,
 * but will merge useful description information into the existing one.
 */
void EntityClass::emplaceAttribute(EntityClassAttribute&& attribute)
{
    // Try to emplace the class attribute
    auto result = _attributes.try_emplace(attribute.getName(), std::move(attribute));

    if (!result.second)
    {
        auto& existing = result.first->second;

        // greebo: Attribute already existed, check if we have some
        // descriptive properties to be added to the existing one.
        if (!attribute.getDescription().empty() && existing.getDescription().empty())
        {
            existing.setDescription(attribute.getDescription());
        }

        // Check if we have a more descriptive type
        if (!attribute.getType().empty() && existing.getType().empty())
        {
            existing.setType(attribute.getType());
        }
    }
}

void EntityClass::forEachAttributeInternal(InternalAttrVisitor visitor,
                                           bool editorKeys) const
{
    // Visit parent attributes, making sure we set the inherited flag
    if (_parent)
        _parent->forEachAttributeInternal(visitor, editorKeys);

    // Visit our own attributes
    for (const auto& pair: _attributes)
    {
        // Visit if it is a non-editor key or we are visiting all keys
        if (editorKeys || !string::istarts_with(pair.first, "editor_"))
        {
            visitor(pair.second);
        }
    }
}

void EntityClass::forEachAttribute(AttributeVisitor visitor,
                                   bool editorKeys)
{
    ensureParsed();

    // First compile a map of all attributes we need to pass to the visitor,
    // ensuring that there is only one attribute per name (i.e. we don't want to
    // visit the same-named attribute on both a child and one of its ancestors)
    using AttrsByName = std::map<std::string, const EntityClassAttribute*>;
    AttrsByName attrsByName;

    // Internal visit function visits parent first, then child, ensuring that
    // more derived attributes will replace parents in the map
    forEachAttributeInternal(
        [&attrsByName](const EntityClassAttribute& a) {
            attrsByName[a.getName()] = &a;
        },
        editorKeys
    );

    // Pass attributes to the visitor function, setting the inherited flag on
    // any which are not present on this EntityClass
    for (const auto& pair: attrsByName)
    {
        visitor(*pair.second, (_attributes.count(pair.first) == 0));
    }
}

// Resolve inheritance for this class
void EntityClass::resolveInheritance()
{
    // If we have already resolved inheritance, do nothing
    if (_inheritanceResolved)
        return;

    // Lookup the parent name and return if it is not set. Also return if the
    // parent name is the same as our own classname, to avoid infinite
    // recursion.
    std::string parentName = getAttributeValue("inherit");
    if (parentName.empty() || parentName == getDeclName())
    {
        resetColour();
        return;
    }

    // Find the parent entity class
    auto parentClass = GlobalEntityClassManager().findClass(parentName);

    if (parentClass)
    {
        // Set our parent pointer
        _parent = static_cast<EntityClass*>(parentClass.get());
    }
    else
    {
        rWarning() << "[eclassmgr] Entity class " << getDeclName()
            << " specifies unknown parent class " << parentName << std::endl;
    }

    // Set the resolved flag
    _inheritanceResolved = true;

    if (!_fixedSize && !getAttributeValue("model").empty())
    {
        // We have a model path (probably an inherited one), so this is treated as fixed-size class
        _fixedSize = true;
    }

    if (getAttributeValue("editor_light") == "1" || getAttributeValue("spawnclass") == "idLight")
    {
        // We have a light
        setIsLight(true);
    }

    if (getAttributeValue("editor_transparent") == "1")
    {
        _colourTransparent = true;
    }

    // Set up inheritance of entity colours: colours inherit from parent unless
    // there is an explicit editor_color defined at this level
    resetColour();
    if (_parent)
    {
        // resolveInheritance() can be called more than once (e.g. after Reload Defs) so make sure
        // we only have a single connection to the parent's changed signal.
        _parentChangedConnection.disconnect();
        _parentChangedConnection = _parent->changedSignal().connect(
            sigc::mem_fun(this, &EntityClass::resetColour)
        );
    }
}

bool EntityClass::isOfType(const std::string& className)
{
    ensureParsed();

	for (IEntityClass* currentClass = this;
         currentClass != nullptr;
         currentClass = currentClass->getParent())
    {
        if (currentClass->getDeclName() == className)
		{
			return true;
		}
    }

	return false;
}

// Find a single attribute
EntityClassAttribute* EntityClass::getAttribute(const std::string& name, bool includeInherited)
{
    ensureParsed();

    // First look up the attribute on this class; if found, we can simply return it
    auto f = _attributes.find(name);
    if (f != _attributes.end())
        return &f->second;

    // If there is no parent or we have been instructed to ignore inheritance,
    // this is the end of the line: return nothing
    if (!_parent || !includeInherited)
        return nullptr;

    // Otherwise delegate to the parent (which will recurse until an attribute
    // is found or a null parent ends the process)
    return _parent->getAttribute(name);
}

std::string EntityClass::getAttributeValue(const std::string& name, bool includeInherited)
{
    if (auto* attr = getAttribute(name, includeInherited); attr)
        return attr->getValue();
    else
        return "";
}

std::string EntityClass::getAttributeType(const std::string& name)
{
    ensureParsed();

    // Check the attributes on this class
    const auto& attribute = _attributes.find(name);

    if (attribute != _attributes.end())
    {
        const auto& type = attribute->second.getType();

        if (!type.empty())
        {
            return type;
        }
    }

    // Walk up the inheritance tree until we spot a non-empty type
    return _parent ? _parent->getAttributeType(name) : "";
}

std::string EntityClass::getAttributeDescription(const std::string& name) 
{
    ensureParsed();

    // Check the attributes on this class first
    const auto& attribute = _attributes.find(name);

    if (attribute != _attributes.end())
    {
        const auto& description = attribute->second.getDescription();

        if (!description.empty())
        {
            return description;
        }
    }

    // Walk up the inheritance tree until we spot a non-empty description
    return _parent ? _parent->getAttributeDescription(name) : "";
}

void EntityClass::clear()
{
    // Don't clear the name
    _isLight = false;
    _parent = nullptr;

    _colour = UndefinedColour;
    _colourTransparent = false;

    _fixedSize = false;

    _attributes.clear();
    _inheritanceResolved = false;
}

void EntityClass::parseEditorSpawnarg(const std::string& key, const std::string& value)
{
    // "editor_yyy" represents an attribute that may be set on this
    // entity. Construct a value-less EntityClassAttribute to add to
    // the class, so that it will show in the entity inspector.

    // Locate the space in "editor_bool myVariable", starting after "editor_"
    std::size_t spacePos = key.find(' ', 7);

    // Only proceed if we have a space (some keys like "editor_displayFolder"
    // don't have spaces)
    if (spacePos != std::string::npos)
    {
        // The part beyond the space is the name of the attribute
        std::string attName = key.substr(spacePos + 1);

        // Get the type by trimming the string left and right
        std::string type = key.substr(7, key.length() - attName.length() - 8);

        if (!attName.empty() && type != "setKeyValue") // Ignore editor_setKeyValue
        {
            // Transform the type into a better format
            if (type == "var" || type == "string")
            {
                type = "text";
            }

            // Construct an attribute with empty value, but with valid
            // description
            emplaceAttribute(EntityClassAttribute(type, attName, "", value));
        }
    }
}

void EntityClass::onBeginParsing()
{
    // Clear this structure first, we might be "refreshing" ourselves from tokens
    clear();
}

void EntityClass::parseFromTokens(parser::DefTokeniser& tokeniser)
{
    // Loop over all of the keys in this entitydef
    while (tokeniser.hasMoreTokens())
    {
        auto key = tokeniser.nextToken();
        auto value = tokeniser.nextToken();

        // Handle some keys specially
        if (key == "model")
        {
            _fixedSize = true;
        }
        else if (key == "editor_color")
        {
            setColour(string::convert<Vector3>(value));
        }
        else if (key == "editor_light")
        {
            setIsLight(value == "1");
        }
        else if (key == "spawnclass")
        {
            setIsLight(value == "idLight");
        }
        else if (string::istarts_with(key, "editor_"))
        {
            parseEditorSpawnarg(key, value);
        }

        // We're only interested in non-inherited key/values when parsing
        auto* attribute = getAttribute(key, false);

        // Add the EntityClassAttribute for this key/val
        if (!attribute)
        {
            // Attribute does not exist, add it.
            // Following key-specific processing, add the keyvalue to the eclass
            // The type is an empty string, it will be set to a non-type as soon as we encounter it
            emplaceAttribute(EntityClassAttribute("", key, value, ""));
        }
        else if (attribute->getValue().empty())
        {
            // Attribute type is set, but value is empty, set the value.
            attribute->setValue(value);
        }
        else
        {
            // Both type and value are not empty, emit a warning
            rWarning() << "[eclassmgr] attribute " << key
                << " already set on entityclass " << getDeclName() << std::endl;
        }
    }
}

void EntityClass::onParsingFinished()
{
    resolveInheritance();

    // Reset the determined visibility, it might have changed
    _visibility = Lazy<vfs::Visibility>([this] { return determineVisibilityFromValues(); });

    // Notify the observers
    emitChangedSignal();
}

} // namespace eclass
