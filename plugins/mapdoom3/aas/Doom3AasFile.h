#pragma once

#include "iaasfile.h"
#include "parser/DefTokeniser.h"
#include "Doom3AasFileSettings.h"

namespace map
{
    
class Doom3AasFile :
    public IAasFile
{
private:
    Doom3AasFileSettings _settings;

public:
    void parseFromTokens(parser::DefTokeniser& tok);
};
typedef std::shared_ptr<Doom3AasFile> Doom3AasFilePtr;

}
