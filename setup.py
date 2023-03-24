from setuptools import setup
from pybind11.setup_helpers import Pybind11Extension
from pybind11 import get_include
from setuptools.command.build_ext import build_ext

__version__ = "0.0.2"
__name__ = "graphenix_engine2"

class BuildExt(build_ext):
    def build_extensions(self):
        for ext in self.extensions:
            if ext.name == __name__:
                ext.define_macros.append(("WITH_OPENMP", None))
                ext.extra_compile_args.extend(["-fopenmp", "-O3"])
                ext.extra_link_args.append("-fopenmp")
                ext.include_dirs.append(get_include())
        super().build_extensions()


ext_modules = [
    Pybind11Extension(
        __name__,
        ["engine/main.cpp"],
        define_macros=[('VERSION_INFO', __version__)],
    ),
]

setup(name=__name__,
      version=__version__,
      author="janezsedeljsak",
      author_email="janez.sedeljsak@gmail.com",
      ext_modules=ext_modules,
      python_requires=">=3.10", # because of match stmts
      description="Graphenix engine with PyBind11",
      cmdclass={"build_ext": BuildExt})