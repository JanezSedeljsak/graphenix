from distutils.core import setup, Extension

def main():
  setup(
    name="graphenix_engine",
    version="1.0.0",
    description="py-base-core module",
    author="janezsedeljsak",
    author_email="janez.sedeljsak@gmail.com",
    ext_modules=[Extension("graphenix_engine", ["./engine/main.cpp"])]
  )

if (__name__ == "__main__"):
  main()