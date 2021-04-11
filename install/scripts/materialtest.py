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
print('Deform Type: {0}'.format(newMaterial.getDeformType()))
print('Deform Expression #1: {0}'.format(newMaterial.getDeformExpressionString(0)))
print('Deform Decl Name: {0}'.format(newMaterial.getDeformDeclName()))
print('Spectrum: {0}'.format(newMaterial.getSpectrum()))
print('DecalInfo.stayMilliSeconds: {0}'.format(newMaterial.getDecalInfo().stayMilliSeconds))
print('DecalInfo.fadeMilliSeconds: {0}'.format(newMaterial.getDecalInfo().fadeMilliSeconds))
print('DecalInfo.startColour: {0}'.format(newMaterial.getDecalInfo().startColour))
print('DecalInfo.endColour: {0}'.format(newMaterial.getDecalInfo().endColour))
print('Coverage: {0}'.format(newMaterial.getCoverage()))
print('Light Falloff Map Type: {0}'.format(newMaterial.getLightFalloffCubeMapType()))
print('Renderbump Arguments: {0}'.format(newMaterial.getRenderBumpArguments()))

# There are a couple of pre-defined sort requests, corresponding to the engine code
newMaterial.setSortRequest(dr.Material.SortRequest.NEAREST)
newMaterial.setClampType(dr.Material.ClampType.NOREPEAT) # clamp
newMaterial.setCullType(dr.Material.CullType.NONE) # twosided
newMaterial.setPolygonOffset(0.3)
newMaterial.setMaterialFlag(dr.Material.Flag.NOSHADOWS)
newMaterial.setSurfaceFlag(dr.Material.SurfaceFlag.LADDER)
newMaterial.setSurfaceFlag(dr.Material.SurfaceFlag.NONSOLID)
newMaterial.setSurfaceType(dr.Material.SurfaceType.WOOD)
newMaterial.setSpectrum(5)
newMaterial.setIsFogLight(1)
newMaterial.setIsBlendLight(0)
newMaterial.setDescription("New Material")

stageIndex = newMaterial.addStage(dr.MaterialStage.Type.BLEND)

print('Material has now {0} stages'.format(newMaterial.getNumStages()))

newMaterial.removeStage(stageIndex)

diffuseStageIndex = newMaterial.addStage(dr.MaterialStage.Type.DIFFUSE)
diffuseStage = newMaterial.getStage(diffuseStageIndex)

bumpStageIndex = newMaterial.duplicateStage(diffuseStageIndex)
bumpStage = newMaterial.getStage(bumpStageIndex)

print('Material has now {0} stages'.format(newMaterial.getNumStages()))

for stage in newMaterial.getAllStages():
    print('Stage type: {0}'.format(stage.getType()))
    print('Stage flags: {0}'.format(stage.getStageFlags()))
    print('Stage clamp type: {0}'.format(stage.getClampType()))
    print('Stage texgen type: {0}'.format(stage.getTexGenType()))
    print('Stage texgen expression #1: {0}'.format(stage.getTexGenExpressionString(0)))
    print('Stage blend func strings: {0},{1}'.format(stage.getBlendFuncStrings()[0], stage.getBlendFuncStrings()[1]))
    print('Stage colour expression RED: {0}'.format(stage.getColourExpressionString(dr.MaterialStage.ColourComponent.RED)))
    print('Stage colour expression GREEN: {0}'.format(stage.getColourExpressionString(dr.MaterialStage.ColourComponent.GREEN)))
    print('Stage colour expression BLUE: {0}'.format(stage.getColourExpressionString(dr.MaterialStage.ColourComponent.BLUE)))
    print('Stage colour expression ALPHA: {0}'.format(stage.getColourExpressionString(dr.MaterialStage.ColourComponent.ALPHA)))
    print('Stage colour expression RGB: {0}'.format(stage.getColourExpressionString(dr.MaterialStage.ColourComponent.RGB)))
    print('Stage colour expression RGBA: {0}'.format(stage.getColourExpressionString(dr.MaterialStage.ColourComponent.RGBA)))

newMaterial.swapStagePosition(diffuseStageIndex, bumpStageIndex)

print("The new material has been modified" if newMaterial.isModified() else "The new material is not modified?")

print('\n\nFull Material definition:\n{0}\n{{{1}}}\n\n'.format(newMaterial.getName(), newMaterial.getDefinition()))

GlobalMaterialManager.saveMaterial(newMaterial.getName())

print("After save, the material is not modified" if not newMaterial.isModified() else "After save, the material is still modified?")

newMaterial.clearMaterialFlag(dr.Material.Flag.NOSHADOWS)
newMaterial.clearSurfaceFlag(dr.Material.SurfaceFlag.NONSOLID)
newMaterial.setSurfaceType(dr.Material.SurfaceType.DEFAULT)

fullPath = GlobalFileSystem.findFile(filename) + filename
print(fullPath)
os.remove(fullPath)

print("Removing {0} again...".format(newMaterial.getName()))
GlobalMaterialManager.removeMaterial(newMaterial.getName())

print('--- Done ---')

