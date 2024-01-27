import dr as darkradiant

class ResizeSelectedBrushesToBounds:
    def execute(self, min: dr.Vector3, max: dr.Vector3, material: str):
        print(min)
        print(max)
        print(material)

import inspect
for name, obj in inspect.getmembers(sys.modules[__name__], inspect.isclass):
    print(name)