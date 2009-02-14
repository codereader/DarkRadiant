# Called at DarkRadiant startup

t = Temp()
t.printToConsole('Boost::Python is working.')

value = GlobalRegistry.get('user/paths/appPath')

t.printToConsole(value)
