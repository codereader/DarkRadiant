#pragma once

#include "iaasfile.h"

namespace parser { class DefTokeniser; }

namespace map
{

/**
 * A loader class designed to parse Doom 3 AAS Files.
 */
class Doom3AasFileLoader :
    public IAasFileLoader
{
public:
    virtual const std::string& getAasFormatName() const override;
	virtual const std::string& getGameType() const override;

	virtual bool canLoad(std::istream& stream) const override;
    virtual IAasFilePtr loadFromStream(std::istream& stream) override;

private:
    // Parses the file header, throws exception on failure
    void parseVersion(parser::DefTokeniser& tok) const;
};

}