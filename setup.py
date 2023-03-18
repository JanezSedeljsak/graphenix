from setuptools import setup
from pybind11.setup_helpers import Pybind11Extension

__version__ = "0.0.1"

ext_modules = [
    Pybind11Extension(
        "graphenix_engine",
        ["engine/main.cpp"],
        define_macros = [('VERSION_INFO', __version__)],
    ),
]

setup(name="graphenix_engine",
      version=__version__,
      author="janezsedeljsak",
      author_email="janez.sedeljsak@gmail.com",
      ext_modules=ext_modules,
      python_requires=">=3.9",
      description="Graphenix engine with PyBind11")