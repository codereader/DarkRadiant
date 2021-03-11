__commandName__ = 'findDuplicateEntities'
__commandDisplayName__ = 'Find Duplicate Entities'

if __executeCommand__:
    import darkradiant as dr

    class EntityCache(dr.SceneNodeVisitor):
        entities = []
        def pre(self, node):
            # Try to "cast" the node to an entity
            entity = node.getEntity()
            if not entity.isNull():
                self.entities.append({
                    'node': node,
                    'origin': entity.getKeyValue('origin'),
                    'model': entity.getKeyValue('model'),
                    'classname': entity.getKeyValue('classname'),
                    'rotation': entity.getKeyValue('rotation')
                })
                return 0 # don't traverse this entity's children
            return 1 # not an entity, so traverse children

    GlobalSelectionSystem.setSelectedAll(False)
    
    entity_cache = EntityCache()
    GlobalSceneGraph.root().traverse(entity_cache)

    duplicates = 0
    num_entities = len(entity_cache.entities)
    for i in range(0, num_entities - 1):
        current_entity = entity_cache.entities[i]
        for j in range(i + 1 , num_entities):
            compare_entity = entity_cache.entities[j]
            if (current_entity['origin'] == compare_entity['origin']
                and current_entity['model'] == compare_entity['model']
                and current_entity['classname'] == compare_entity['classname']
                and current_entity['rotation'] == compare_entity['rotation']
            ):
                compare_entity['node'].setSelected(True)
                duplicates += 1

    if duplicates > 0:
        result = 'Found and selected %d duplicate entities.' % duplicates
    else:
        result = 'No duplicate entities found.'
        
    GlobalDialogManager.createMessageBox('Find Duplicate Entities Results', result, dr.Dialog.CONFIRM).run()
