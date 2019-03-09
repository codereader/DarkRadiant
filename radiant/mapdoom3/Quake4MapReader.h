#pragma once

#include "Doom3MapReader.h"
#include "Quake4MapFormat.h"

namespace map
{

class Quake4MapReader :
	public Doom3MapReader
{
public:
	Quake4MapReader(IMapImportFilter& importFilter);

protected:
	virtual void initPrimitiveParsers();

	// Parse the version tag at the beginning, throws on failure
	virtual void parseMapVersion(parser::DefTokeniser& tok);
};

} // namespace
