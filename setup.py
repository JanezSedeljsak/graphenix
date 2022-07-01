from distutils.core import setup, Extension

def main():
  setup(
    name="pybase_core",
    version="1.0.0",
    description="py-base-core module",
    author="janezsedeljsak",
    author_email="janez.sedeljsak@gmail.com",
    ext_modules=[Extension("pybase_core", ["./core/main.c"])]
  )

if (__name__ == "__main__"):
  main()