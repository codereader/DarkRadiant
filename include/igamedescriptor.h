#ifndef IGAMEDESCRIPTOR_H_
#define IGAMEDESCRIPTOR_H_

#include "generic/constant.h"

#include <libxml/parser.h>

/* IGameDescriptor interface class.
 * 
 * Interface functions for the game descriptor Global module.
 */

class IGameDescriptor {
public:   
    
    // Required unique name and version
    STRING_CONSTANT(Name, "IGameDescriptor");
    INTEGER_CONSTANT(Version, 1);
   
   // Return the root node of the XML game description file
   virtual xmlNodePtr getRootNode() = 0;
    
};

#endif /*IGAMEDESCRIPTOR_H_*/
