from distutils.core import setup, Extension

def main():
  setup(
    name="pybase",
    version="1.0.0",
    description="py-base module",
    author="janezsedeljsak",
    author_email="janez.sedeljsak@gmail.com",
    ext_modules=[Extension("pybase", ["./core/pybase.c"])]
  )

if (__name__ == "__main__"):
  main()