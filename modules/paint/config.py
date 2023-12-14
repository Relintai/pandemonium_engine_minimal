

def can_build(env, platform):
  return True


def configure(env):
	pass


def get_doc_classes():
    return [
      "BrightenAction",
      "BrushAction",
      "BucketAction",
      "CutAction",
      "DarkenAction",
      "LineAction",
      "MultiLineAction",
      "PaintAction",
      "PasteCutAction",
      "PencilAction",
      "RainbowAction",
      "RectAction",

      "PaintNode",
      "PaintCanvas",
      "PaintProject", 

      "PaintCanvasBackground",
      "PaintVisualGrid",

      "PaintCustomPropertyInspector",
      "PaintProjectPropertyInspector",
      "PaintProjectToolsPropertyInspector",
      "PaintToolsPropertyInspector",

      "PaintPolygon2D",
      "PaintCurve2D",
    ]

def get_doc_path():
    return "doc_classes"

def get_license_file():
  return "COPYRIGHT.txt"
