#include "EntityClassTreePopulator.h"
#include "EntityClassChooser.h"

#include "iregistry.h"
#include "iradiant.h"

namespace ui
{

// Constructor
EntityClassTreePopulator::EntityClassTreePopulator(GtkTreeStore* store)
: _store(store),
  _folderKey(GlobalRegistry().get(FOLDER_KEY_PATH))
{}

GtkTreeIter* 
EntityClassTreePopulator::addRecursive(const std::string& pathName)
{
    // Lookup pathname in map, and return the GtkTreeIter* if it is
    // found
    DirIterMap::iterator iTemp = _dirIterMap.find(pathName);
    if (iTemp != _dirIterMap.end()) { // found in map
        return iTemp->second;
    }
    
    // Split the path into "this directory" and the parent path
    std::size_t slashPos = pathName.rfind("/");
    const std::string parentPath = pathName.substr(0, slashPos);
    const std::string thisDir = pathName.substr(slashPos + 1);

    // Recursively add parent path
    GtkTreeIter* parIter = NULL;
    if (slashPos != std::string::npos)
        parIter = addRecursive(parentPath);

    // Now add "this directory" as a child, saving the iter in the map
    // and returning it.
    GtkTreeIter iter;
    gtk_tree_store_append(_store, &iter, parIter);
    gtk_tree_store_set(_store, &iter, 
                       NAME_COLUMN, thisDir.c_str(),
                       ICON_COLUMN, GlobalRadiant().getLocalPixbuf(FOLDER_ICON),
                       DIR_FLAG_COLUMN, TRUE,
                       -1);
    GtkTreeIter* dynIter = gtk_tree_iter_copy(&iter); // get a heap-allocated iter
    
    // Cache the dynamic iter and return it
    _dirIterMap[pathName] = dynIter;
    return dynIter;
}

// Add the display folder for the given entity class
GtkTreeIter* EntityClassTreePopulator::addDisplayFolder(IEntityClassPtr e) {

    // Get the parent folder from the entity class (which may be blank). We
    // prepend this with the entity class' mod name, to ensure that top-level
    // directories are created for each mod
    std::string folderPath = e->getAttribute(_folderKey).value;
    if (!folderPath.empty())
        folderPath = "/" + folderPath;
    
    std::string parentFolder = e->getModName() + folderPath;
       
    // Call the recursive function to add the folder
    return addRecursive(parentFolder);
}

// Required visit function
void EntityClassTreePopulator::visit(IEntityClassPtr e) {
    // Recursively create the folder to put this EntityClass in,
    // depending on the value of the DISPLAY_FOLDER_KEY. This may return
    // NULL if the key is unset, in which case we add the entity at
    // the top level.
    GtkTreeIter* parIter = addDisplayFolder(e);
    
    // Add the new class under the parent folder
    GtkTreeIter iter;
    gtk_tree_store_append(_store, &iter, parIter);
    gtk_tree_store_set(_store, &iter, 
                       NAME_COLUMN, e->getName().c_str(), 
                       ICON_COLUMN, GlobalRadiant().getLocalPixbuf(ENTITY_ICON),
                       DIR_FLAG_COLUMN, FALSE,
                       -1);
}

} // namespace ui
