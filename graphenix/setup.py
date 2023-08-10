from setuptools import setup, find_packages
from pybind11.setup_helpers import Pybind11Extension
from pybind11 import get_include
from setuptools.command.build_ext import build_ext
import os

__version__ = "0.0.4"
__engine_name__ = "graphenix_engine2"
__name__ = "graphenix"

__author_data = {"author": "janezsedeljsak",
                 "author_email": "janez.sedeljsak@gmail.com"}

setup_dir = os.path.dirname(os.path.abspath(__file__))
cpp_file_path = os.path.join(setup_dir, "engine", "main.cpp")

class BuildExt(build_ext):
    def build_extensions(self):
        for ext in self.extensions:
            if ext.name == __engine_name__:
                ext.define_macros.append(("WITH_OPENMP", None))
                ext.extra_compile_args.extend(["-std=c++20", "-fopenmp", "-O3"])
                ext.extra_link_args.append("-fopenmp")
                ext.include_dirs.append(get_include())
                ext.libraries.append("gomp")
        super().build_extensions()

ext_modules = [
    Pybind11Extension(
        __engine_name__,
        [cpp_file_path],
        define_macros=[('VERSION_INFO', __version__)],
    ),
]

setup(name=__engine_name__,
      version=__version__,
      ext_modules=ext_modules,
      python_requires=">=3.10", # because of match stmts
      description="Graphenix engine written with PyBind11 - C++",
      cmdclass={"build_ext": BuildExt},
      **__author_data)