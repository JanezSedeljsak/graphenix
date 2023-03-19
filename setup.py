from setuptools import setup
from pybind11.setup_helpers import Pybind11Extension

__version__ = "0.0.2"
__name__ = "graphenix_engine2"

ext_modules = [
    Pybind11Extension(
        __name__,
        ["engine/main.cpp"],
        define_macros = [('VERSION_INFO', __version__)],
    ),
]

setup(name=__name__,
      version=__version__,
      author="janezsedeljsak",
      author_email="janez.sedeljsak@gmail.com",
      ext_modules=ext_modules,
      python_requires=">=3.9",
      description="Graphenix engine with PyBind11")