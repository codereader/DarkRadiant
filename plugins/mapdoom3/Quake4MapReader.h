#pragma once

#include "i18n.h"
#include "itextstream.h"
#include "Doom3MapReader.h"
#include "Quake4MapFormat.h"

#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>

namespace map
{

class Quake4MapReader :
	public Doom3MapReader
{
public:
	Quake4MapReader(IMapImportFilter& importFilter) :
		 Doom3MapReader(importFilter)
	{}

protected:
	// Parse the version tag at the beginning, throws on failure
	virtual void parseMapVersion(parser::DefTokeniser& tok)
	{
		// Parse the map version
		float version = 0;

		try
		{
			tok.assertNextToken("Version");
			version = boost::lexical_cast<float>(tok.nextToken());
		}
		catch (parser::ParseException& e)
		{
			// failed => quit
			globalErrorStream()
				<< "[mapdoom3] Unable to parse map version: "
				<< e.what() << std::endl;

			throw FailureException(_("Unable to parse map version (parse exception)."));
		}
		catch (boost::bad_lexical_cast& e)
		{
			globalErrorStream()
				<< "[mapdoom3] Unable to parse map version: "
				<< e.what() << std::endl;

			throw FailureException(_("Could not recognise map version number format."));
		}

		float requiredVersion = MAP_VERSION_Q4;

		// Check we have the correct version for this module
		if (version != requiredVersion)
		{
			std::string errMsg = (boost::format(_("Incorrect map version: required %f, found %f")) % requiredVersion % version).str();

			globalErrorStream() << errMsg << std::endl;

			throw FailureException(errMsg);
		}
	}
};

} // namespace
