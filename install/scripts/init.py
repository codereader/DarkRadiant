# Called at DarkRadiant startup

value = GlobalRegistry.get('user/paths/appPath')

print(value)

worldspawn = Radiant.findEntityByClassname("worldspawn")

worldspawn.setKeyValue('test', 'success')

print('Worldspawn edited')