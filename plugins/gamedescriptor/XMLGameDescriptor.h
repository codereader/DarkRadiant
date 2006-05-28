#ifndef XMLGAMEDESCRIPTOR_H_
#define XMLGAMEDESCRIPTOR_H_

#include <libxml/parser.h>

#include "igamedescriptor.h"

/* XMLGameDescriptor
 * 
 * Implementing class for the IGameDescriptor interface. This class provides access
 * to the game description file's XML structure
 */

class XMLGameDescriptor:
    public IGameDescriptor
{
public:
    virtual xmlNodePtr getRootNode();
};

#endif /*XMLGAMEDESCRIPTOR_H_*/
