import os
import darkradiant as dr

# Some interface tests

print("bc_rat can be modified" if GlobalMaterialManager.materialCanBeModified('bc_rat') else "bc_rat cannot be modified")

bcRatCopy = GlobalMaterialManager.copyMaterial('bc_rat', 'bc_rat_copy')

print("Got a copy of bc_rat named {0}".format(bcRatCopy.getName()))
print("The copy can be modified" if GlobalMaterialManager.materialCanBeModified(bcRatCopy.getName()) else "The copy cannot be modified?")

try:
    bcRat = GlobalMaterialManager.getMaterial('bc_rat')
    bcRat.setEditorImageExpressionFromString('this is gonna blow')
except RuntimeError as e:
    print('An exception has been thrown: {}'.format(e))

print("Removing the copy again...")
GlobalMaterialManager.removeMaterial(bcRatCopy.getName())

print('Create an empty material named textures/python/blah...')
newMaterial = GlobalMaterialManager.createEmptyMaterial('textures/python/blah')

print("The new material can be modified" if GlobalMaterialManager.materialCanBeModified(newMaterial.getName()) else "The new material cannot be modified?")

renameResult = GlobalMaterialManager.renameMaterial(newMaterial.getName(), newMaterial.getName() + '_renamed')
print("The rename operation was successful" if renameResult else "The rename operation failed")

# Do something with the new material
print('The existing material reference now has the name {0}'.format(newMaterial.getName()))

filename = 'materials/_python_test.mtr'
newMaterial.setShaderFileName(filename)
newMaterial.setEditorImageExpressionFromString('textures/common/caulk')

newMaterial.setSortRequest(45.3)

print('Surface Type: {0}'.format(newMaterial.getSurfaceType()))
print('Cull type: {0}'.format(newMaterial.getCullType()))
print('Clamp type: {0}'.format(newMaterial.getClampType()))
print('Flags: {0}'.format(newMaterial.getMaterialFlags()))
print('Surface Flags: {0}'.format(newMaterial.getSurfaceFlags()))

# There are a couple of pre-defined sort requests, corresponding to the engine code
newMaterial.setSortRequest(dr.Material.SortRequest.NEAREST)
newMaterial.setClampType(dr.Material.ClampType.NOREPEAT) # clamp
newMaterial.setCullType(dr.Material.CullType.NONE) # twosided
newMaterial.setPolygonOffset(0.3)
newMaterial.setMaterialFlag(dr.Material.Flag.NOSHADOWS)
newMaterial.setSurfaceFlag(dr.Material.SurfaceFlag.LADDER)
newMaterial.setSurfaceFlag(dr.Material.SurfaceFlag.NONSOLID)
newMaterial.setSurfaceType(dr.Material.SurfaceType.WOOD)

print('Full Material definition:\n{0}\n{{{1}}}'.format(newMaterial.getName(), newMaterial.getDefinition()))

GlobalMaterialManager.saveMaterial(newMaterial.getName())

newMaterial.clearMaterialFlag(dr.Material.Flag.NOSHADOWS)
newMaterial.clearSurfaceFlag(dr.Material.SurfaceFlag.NONSOLID)
newMaterial.setSurfaceType(dr.Material.SurfaceType.DEFAULT)

fullPath = GlobalFileSystem.findFile(filename) + filename
print(fullPath)
os.remove(fullPath)

print("Removing {0} again...".format(newMaterial.getName()))
GlobalMaterialManager.removeMaterial(newMaterial.getName())

print('--- Done ---')
