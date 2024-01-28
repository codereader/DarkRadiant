import darkradiant as dr
import inspect

class CommandUtil:
    @staticmethod
    def registerModule(moduleName):
        for name, obj in inspect.getmembers(sys.modules[moduleName], inspect.isclass):
            print(name + ' ' + str(obj))
            registerClass(name, obj)

    @staticmethod
    def registerClass(name, obj):
        sig = inspect.signature(obj.execute)
        print(sig)

        for paramName in sig.parameters:
            print("Parameter {0} has type {1}".format(paramName, sig.parameters[paramName].annotation))
            # Prove that we can identify DR types in the signature
            if sig.parameters[paramName].annotation == dr.Vector3:
                print('This parametr is a Vec3')
        #dr.ScriptingSystem.registerBuiltinScriptCommand(...)