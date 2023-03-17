from setuptools import setup, Extension
import pybind11

pybind_include = pybind11.get_include()

extension_mod = Extension("graphenix_engine",
                           sources=["engine/main.cpp"],
                           include_dirs=[pybind_include])


setup(name="graphenix_engine",
      version="0.0.1",
      author="janezsedeljsak",
      author_email="janez.sedeljsak@gmail.com",
      ext_modules=[extension_mod],
      description="Graphenix engine with PyBind11")