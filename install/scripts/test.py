# Some interface tests

# Test the Registry interface
value = GlobalRegistry.get('user/paths/appPath')
print(value)

worldspawn = Radiant.findEntityByClassname("worldspawn")
worldspawn.setKeyValue('test', 'success')
print('Worldspawn edited')

# Test the EClassManager interface
eclass = GlobalEntityClassManager.findClass('atdm:func_shooter')
print(eclass.getAttribute('editor_usage').getValue())

# Try creating a func_shooter entity
if not eclass.isNull():
	shooter = GlobalEntityCreator.createEntity(eclass)

modelDef = GlobalEntityClassManager.findModel('builderforger')
print('ModelDef mesh for builderforger = ' + modelDef.mesh)

# Test iterating over C++ std::map
#for anim in modelDef.anims:
#	print(anim.key())
#	print(' = ')
#	print(anim.data())
#	print('')

# Test implementing a eclass visitor interface
#class TestVisitor(EntityClassVisitor) :
#	def visit(self, eclass):
#		print eclass.getAttribute('editor_usage').getValue()

#eclassVisitor = TestVisitor()
#GlobalEntityClassManager.forEachEntityClass(eclassVisitor)

# Test implementing a model def visitor interface
#class TestModelDefVisitor(ModelDefVisitor) :
#	def visit(self, modelDef):
#		print(modelDef.mesh)
#
#modelDefVisitor = TestModelDefVisitor()
#GlobalEntityClassManager.forEachModelDef(modelDefVisitor)

# Test traversing the scenegraph
class SceneWalker(SceneNodeVisitor) :
	def pre(self, node):
		print(node.getNodeType())

		# Try to get a model from this node
		model = node.getModel()

		if not model.isNull():
			print('Node is a model')
		else:
			print('Node is not a model')

		return 1

walker = SceneWalker()
GlobalSceneGraph.root().traverse(walker)

# Test traversing the current selection
class Walker(SelectionVisitor) :
	def visit(self, node):
		# Try to "cast" the node to a brush
		brush = node.getBrush()

		# If the Brush is not NULL the cast succeeded
		if not brush.isNull():
			print(brush.getNumFaces())
		else:
			print('Node is not a brush')

		# Try to cast the node to a patch
		patch = node.getPatch()

		# If the Patch is not NULL the cast succeeded
		if not patch.isNull():
			print('Node is a patch')
		else:
			print('Node is not a patch')

		# Try to get a model from this node
		model = node.getModel()

		if not model.isNull():
			print('Node is a model')
		else:
			print('Node is not a model')


visitor = Walker()
GlobalSelectionSystem.foreachSelected(visitor)

# Try to find the map's worldspawn
worldspawn = GlobalMap.getWorldSpawn()

if not worldspawn.isNull():
	# Cast the node onto an entity
	worldspawnent = worldspawn.getEntity()
	if not worldspawnent.isNull():
		print('Spawnclass of worldspawn: ' + worldspawnent.getKeyValue('spawnclass'))
else:
	print('There is no worldspawn in this map yet')

# Test the entity visitor interface
class TestEntityVisitor(EntityVisitor) :
	def visit(self, key, value):
		print('Worldspawn has spawnarg: ' + key + ' = ' + value)

if not worldspawn.isNull():
	tev = TestEntityVisitor()

	# Cast the node onto an entity
	worldspawnent = worldspawn.getEntity()

	worldspawnent.forEachKeyValue(tev)

	# Try to retrieve all keyvalues starting with "n"
	keyvalues = worldspawnent.getKeyValuePairs('t')

	for kv in keyvalues:
		print('Keyvalue ' + kv.first + ' = ' + kv.second)

# Test the commandsystem
GlobalCommandSystem.execute('texscale "0 0.1"')

# Test the GameManager interface
print('Mod path = ' + GlobalGameManager.getModPath())

game = GlobalGameManager.currentGame()
print('Current game type: ' + game.getKeyValue('type'))

print('VFS Search paths:')
vfsPaths = GlobalGameManager.getVFSSearchPaths()

for path in vfsPaths:
	print(path)

# Test FileSystem (VFS)
#class TestFileVisitor(FileVisitor) :
#	def visit(self, filename):
#		print('Found file: ' + filename)
#
#filevisitor = TestFileVisitor()
#GlobalFileSystem.forEachFile('skins/', 'skin', filevisitor, 99)

filecontents = GlobalFileSystem.readTextFile('skins/tdm_ai_elemental_fire.skin');
print(filecontents)

# Test the Grid Interface
print('Current grid size = ' + str(GlobalGrid.getGridSize()))

# Test the ShaderSystem interface
class TestShaderVisitor(ShaderVisitor) :
	def visit(self, shader):
		if not shader.isNull():
			print('Found shader: ' + shader.getName() + ' defined in ' + shader.getShaderFileName())

shadervisitor = TestShaderVisitor()
GlobalMaterialManager.foreachShader(shadervisitor)

shader = GlobalMaterialManager.getMaterialForName('bc_rat')

if not shader.isNull():
	print('Shader ' + shader.getName() + ' is defined in ' + shader.getShaderFileName())

# Test finding a model
class ModelFinder(SceneNodeVisitor) :
	def pre(self, node):
		# Try to get a model from this node
		model = node.getModel()

		if not model.isNull():
			print('Model information:')
			print('Filename: ' + model.getFilename())
			print('Model path: ' + model.getModelPath())
			print('Surface count: ' + str(model.getSurfaceCount()))
			print('Vertex count: ' + str(model.getVertexCount()))
			print('Poly count: ' + str(model.getPolyCount()))

			materials = model.getActiveMaterials()

			print('Active Materials:')
			for material in materials:
				print(material)

			for i in range(0, model.getSurfaceCount()):
				surface = model.getSurface(i)
				print('Surface: ' + str(i))
				print('  Default Shader: ' + surface.getDefaultMaterial())
				print('  Active Shader: ' + surface.getActiveMaterial())
				print('  PolyCount: ' + str(surface.getNumTriangles()))
				print('  Vertex Count: ' + str(surface.getNumVertices()))

				s = Vector3(0,0,0)
				numverts = surface.getNumVertices()
				for v in range(0, numverts):
					meshvertex = surface.getVertex(v)
					s += meshvertex.vertex

				print('  Sum of all vertices: ' + str(s.x()) + ',' + str(s.y()) + ',' + str(s.z()))

		return 1

walker = ModelFinder()
GlobalSceneGraph.root().traverse(walker)

# Test the ModelSkinCache interface
#allSkins = GlobalModelSkinCache.getAllSkins()
#
#for skin in allSkins:
#	modelskin = GlobalModelSkinCache.capture(skin)
#	print('Skin found: ' + modelskin.getName())

# Test patch manipulation
class PatchManipulator(SceneNodeVisitor) :
	def pre(self, node):
		# Try to get a patch from this node
		patch = node.getPatch()

		if not patch.isNull():
			print('Patch information:')
			print('Dimensions: ' + str(patch.getWidth()) + 'x' + str(patch.getHeight()))

			w = 0
			while w < patch.getWidth():
				h = 0
				while h < patch.getHeight():
					# Push the vertex around a bit
					ctrl = patch.ctrlAt(h,w)
					ctrl.vertex += Vector3(0,0,10*(h-w))
					h += 1
				w += 1

			patch.controlPointsChanged()

		return 1

walker = PatchManipulator()
GlobalSceneGraph.root().traverse(walker)

# Test the SelectionSetManager interface
class SelectionSetWalker(SelectionSetVisitor) :
	def visit(self, selectionset):
		print(selectionset.getName())

walker = SelectionSetWalker()
GlobalSelectionSetManager.foreachSelectionSet(walker)

selSet = GlobalSelectionSetManager.createSelectionSet("TestSelectionSet")
selSet.assignFromCurrentScene()

selSet.deselect()
selSet.select()

selSet.clear()

if selSet.empty():
	GlobalSelectionSetManager.deleteSelectionSet("TestSelectionSet")

print('')

