# Test patch manipulation
class PatchManipulator(SceneNodeVisitor) :
	def pre(self, node):
		# Try to get a patch from this node
		patch = node.getPatch()

		if not patch.isNull():
			print('Patch information:')
			print('Dimensions: ' + str(patch.getWidth()) + 'x' + str(patch.getHeight()))

			mesh = patch.getTesselatedPatchMesh()

			print('Mesh dimension: w=' + str(mesh.width) + ', h=' + str(mesh.height))

			for v in mesh.vertices:
				print('Mesh vertex: ' + str(v.vertex.x()) + ',' + str(v.vertex.y()) + ',' + str(v.vertex.z()))

		return 1

walker = PatchManipulator()
GlobalSceneGraph.root().traverse(walker)