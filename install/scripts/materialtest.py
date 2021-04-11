import os

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

GlobalMaterialManager.saveMaterial(newMaterial.getName())

fullPath = GlobalFileSystem.findFile(filename) + filename
print(fullPath)
os.remove(fullPath)

print("Removing {0} again...".format(newMaterial.getName()))
GlobalMaterialManager.removeMaterial(newMaterial.getName())

print('--- Done ---')

