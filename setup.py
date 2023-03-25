from setuptools import setup, find_packages
from pybind11.setup_helpers import Pybind11Extension
from pybind11 import get_include
from setuptools.command.build_ext import build_ext

__version__ = "0.0.2"
__engine_name__ = "graphenix_engine2"
__name__ = "graphenix"

__author_data = {"author": "janezsedeljsak",
                 "author_email": "janez.sedeljsak@gmail.com"}

class BuildExt(build_ext):
    def build_extensions(self):
        for ext in self.extensions:
            if ext.name == __engine_name__:
                ext.define_macros.append(("WITH_OPENMP", None))
                ext.extra_compile_args.extend(["-fopenmp", "-O3"])
                ext.extra_link_args.append("-fopenmp")
                ext.include_dirs.append(get_include())
        super().build_extensions()


ext_modules = [
    Pybind11Extension(
        __engine_name__,
        ["graphenix/engine/main.cpp"],
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

setup(name=__name__,
      version=__version__,
      packages=find_packages(exclude=["*.tests", "*.tests.*", "tests.*", "tests"]),
      install_requires=["pybind11", "graphenix_engine2"],
      description="Graphenix library",
      exclude_package_data={"": ["*.cpp", "*.h", "*.hpp", "*.pyx", "*.pxd"]},
      **__author_data)

