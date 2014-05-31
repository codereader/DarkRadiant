"""Find any entities with "target" spawnargs that don't point to a valid entity in the map.
Print results to the console, plus show a dialog and offer to select any entities that have 
missing targets.

# 3718
Proposal: RJFerret
Script: SteveL 
"""

__commandName__ = 'test_targets' # should not contain spaces
__commandDisplayName__ = 'Test for Missing Targets' # should not contain spaces


def execute():
    g_targets = {} # entityname : [targets]
    g_missing = {} # entityname : [missingtargets]
    class TargetFinder(SceneNodeVisitor):
        def pre(self, node):
            if node.isEntity():
                n = node.getEntity()
                name = n.getKeyValue('name')
                targs = [t.second for t in n.getKeyValuePairs('target')]
                g_targets[name] = targs
            return 1
    # Instantiate a new walker object and get list of entities/targets
    walker = TargetFinder()
    GlobalSceneGraph.root().traverse(walker)
    # Find any targets that don't exist, and count all targets
    entities = g_targets.keys()
    targetcount = 0
    for ent in entities:
        targetcount += len(g_targets[ent])
        missing = []
        for targ in g_targets[ent]:
            if targ not in entities:
                missing.append(targ)
        if missing:
            g_missing[ent] = missing
    # generate report
    msg = '%d entities found with %d targets' % (len(entities), targetcount) + '\n\n'
    if not g_missing:
        msg += 'No missing targets found'
        GlobalDialogManager.createMessageBox('Missing targets', msg, Dialog.CONFIRM).run()
    else:
        msg += 'Missing targets:\n'
        for ent in g_missing.keys():
            for targ in g_missing[ent]:
                msg += '%s -> %s\n' % (ent, targ)
        print(msg) # output to console
        msg += "\nThe list of missing targets has been printed to the console."
        msg += "\n\nDo you want to select all entities with missing targets?"
        response = GlobalDialogManager.createMessageBox('Missing targets', msg, Dialog.ASK).run()
        if response == Dialog.YES:
            class Selector(SceneNodeVisitor):
                def pre(self, node):
                    if node.isEntity() and node.getEntity().getKeyValue('name') in g_missing.keys():
                        node.setSelected(True)
                    return 1
            GlobalSceneGraph.root().traverse(Selector())

if __executeCommand__:
    execute()
