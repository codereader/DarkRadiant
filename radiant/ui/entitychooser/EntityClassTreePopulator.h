#ifndef ENTITYCLASSTREEPOPULATOR_H_
#define ENTITYCLASSTREEPOPULATOR_H_

#include "ieclass.h"

#include <gtk/gtktreestore.h>

#include <map>
#include <string>

#include <boost/functional/hash/hash.hpp>

namespace ui
{

/**
 * EntityClassVisitor which populates a GtkTreeStore with entity classnames
 * taking account of display folders and mod names.
 */
class EntityClassTreePopulator
: public EntityClassVisitor 
{
    // Map between string directory names and their corresponding Iters
	typedef std::map<std::string, GtkTreeIter*> DirIterMap;
    DirIterMap _dirIterMap;

    // TreeStore to populate
    GtkTreeStore* _store;
    
    // Key that specifies the display folder
    std::string _folderKey;

private:
    
    // Recursive folder add function
    GtkTreeIter* addRecursive(const std::string& pathName);

    // Add parent folder
    GtkTreeIter* addDisplayFolder(IEntityClassPtr e);
    
public:

    // Constructor
    EntityClassTreePopulator(GtkTreeStore* store);

    // Required visit function
    virtual void visit(IEntityClassPtr e);
};

}

#endif /*ENTITYCLASSTREEPOPULATOR_H_*/
