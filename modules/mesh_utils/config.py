
def can_build(env, platform):
  return True


def configure(env):
	pass


def get_doc_classes():
    return [
        "MeshMerger",
        "MeshUtils",
        "FastQuadraticMeshSimplifier",
    ]

def get_doc_path():
    return "doc_classes"

def get_license_file():
  return "COPYRIGHT.txt"
