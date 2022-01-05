#include "Light.h"

#include "iradiant.h"
#include "itextstream.h"
#include "igrid.h"
#include "Doom3LightRadius.h"
#include "LightShader.h"
#include <functional>
#include "../EntitySettings.h"

#include "LightNode.h"

namespace entity
{

// Initialise the static default shader string
std::string LightShader::m_defaultShader = "";

// ----- Light Class Implementation -------------------------------------------------

} // namespace entity
