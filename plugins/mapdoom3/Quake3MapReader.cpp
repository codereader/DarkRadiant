#include "Quake3MapReader.h"

#include "itextstream.h"
#include "ieclass.h"
#include "igame.h"
#include "ientity.h"
#include "string/string.h"

#include "i18n.h"
#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>

#include "primitiveparsers/BrushDef.h"
#include "primitiveparsers/BrushDef3.h"
#include "primitiveparsers/PatchDef2.h"
#include "primitiveparsers/PatchDef3.h"

namespace map {

Quake3MapReader::Quake3MapReader(IMapImportFilter& importFilter) : 
	_importFilter(importFilter),
	_entityCount(0),
	_primitiveCount(0)
{}

void Quake3MapReader::readFromStream(std::istream& stream)
{
	// Call the virtual method to initialise the primitve parser map (if not done yet)
	initPrimitiveParsers();

	// The tokeniser used to split the stream into pieces
	parser::BasicDefTokeniser<std::istream> tok(stream);

	// Read each entity in the map, until EOF is reached
	while (tok.hasMoreTokens())
	{
		// Create an entity node by parsing from the stream. If there is an
		// exception, display it and return
		try
		{
			parseEntity(tok);
		}
		catch (FailureException& e)
		{
			std::string text = (boost::format(_("Failed parsing entity %d:\n%s")) % _entityCount % e.what()).str();

			// Re-throw with more text
			throw FailureException(text);
		}

		_entityCount++;
	}

	// EOF reached, success
}

void Quake3MapReader::initPrimitiveParsers()
{
	if (_primitiveParsers.empty())
	{
		addPrimitiveParser(PrimitiveParserPtr(new BrushDefParser));
		addPrimitiveParser(PrimitiveParserPtr(new PatchDef2ParserQ3));
	}
}

void Quake3MapReader::addPrimitiveParser(const PrimitiveParserPtr& parser)
{
	_primitiveParsers.insert(PrimitiveParsers::value_type(parser->getKeyword(), parser));
}

void Quake3MapReader::parsePrimitive(parser::DefTokeniser& tok, const scene::INodePtr& parentEntity)
{
    _primitiveCount++;

	std::string primitiveKeyword = tok.nextToken();

	// Get a parser for this keyword
	PrimitiveParsers::const_iterator p = _primitiveParsers.find(primitiveKeyword);

	if (p == _primitiveParsers.end())
	{
		throw FailureException("Unknown primitive type: " + primitiveKeyword);
	}

	const PrimitiveParserPtr& parser = p->second;

	// Try to parse the primitive, throwing exception if failed
	try
	{
		scene::INodePtr primitive = parser->parse(tok);

		if (!primitive)
		{
			std::string text = (boost::format(_("Primitive #%d: parse error")) % _primitiveCount).str();
			throw FailureException(text);
		}

		// Now add the primitive as a child of the entity
		_importFilter.addPrimitiveToEntity(primitive, parentEntity); 
	}
	catch (parser::ParseException& e)
	{
		// Translate ParseExceptions to FailureExceptions
		std::string text = (boost::format(_("Primitive #%d: parse exception %s")) % _primitiveCount % e.what()).str();
		throw FailureException(text);
	}
}

scene::INodePtr Quake3MapReader::createEntity(const EntityKeyValues& keyValues)
{
    // Get the classname from the EntityKeyValues
    EntityKeyValues::const_iterator found = keyValues.find("classname");

    if (found == keyValues.end())
	{
		throw FailureException("Quake3MapReader::createEntity(): could not find classname.");
    }

    // Otherwise create the entity and add all of the properties
    std::string className = found->second;
	IEntityClassPtr classPtr = GlobalEntityClassManager().findClass(className);

	if (classPtr == NULL)
	{
		rError() << "[mapdoom3]: Could not find entity class: "
			<< className << std::endl;

		// greebo: EntityClass not found, insert a brush-based one
		classPtr = GlobalEntityClassManager().findOrInsert(className, true);
	}

	// Create the actual entity node
    IEntityNodePtr node(GlobalEntityCreator().createEntity(classPtr));

    for (EntityKeyValues::const_iterator i = keyValues.begin();
         i != keyValues.end();
         ++i)
    {
        node->getEntity().setKeyValue(i->first, i->second);
    }

    return node;
}

void Quake3MapReader::parseEntity(parser::DefTokeniser& tok)
{
    // Map of keyvalues for this entity
    EntityKeyValues keyValues;

    // The actual entity. This is initially null, and will be created when
    // primitives start or the end of the entity is reached
    scene::INodePtr entity;

	// Start parsing, first token must be an open brace
	tok.assertNextToken("{");

	std::string token = tok.nextToken();

	// Reset the primitive counter, we're starting a new entity
	_primitiveCount = 0;

	while (true)
	{
	    // Token must be either a key, a "{" to indicate the start of a
	    // primitive, or a "}" to indicate the end of the entity

	    if (token == "{") // PRIMITIVE
		{ 
			// Create the entity right now, if not yet done
			if (entity == NULL)
			{
				entity = createEntity(keyValues);
			}

			// Parse the primitive block, and pass the parent entity
			parsePrimitive(tok, entity);
	    }
	    else if (token == "}") // END OF ENTITY
		{
            // Create the entity if necessary and return it
	        if (entity == NULL)
			{
	            entity = createEntity(keyValues);
	        }

			break;
	    }
	    else // KEY
		{ 
	        std::string value = tok.nextToken();

	        // Sanity check (invalid number of tokens will get us out of sync)
	        if (value == "{" || value == "}")
			{
				std::string text = (boost::format(_("Parsed invalid value '%s' for key '%s'")) % value % token).str();
	            throw FailureException(text);
	        }

	        // Otherwise add the keyvalue pair to our map
	        keyValues.insert(EntityKeyValues::value_type(token, value));
	    }

	    // Get the next token
	    token = tok.nextToken();
	}

	// Insert the entity
	_importFilter.addEntity(entity);
}

} // namespace map
