from setuptools import setup, find_packages
from pybind11.setup_helpers import Pybind11Extension
from pybind11 import get_include
from setuptools.command.build_ext import build_ext

__version__ = "0.0.4"
__engine_name__ = "graphenix_engine2"
__name__ = "graphenix"

__author_data = {"author": "janezsedeljsak",
                 "author_email": "janez.sedeljsak@gmail.com"}

setup(name=__name__,
      version=__version__,
      packages=find_packages(exclude=["*.tests", "*.tests.*", "tests.*", "tests"]),
      install_requires=["pybind11", "graphenix_engine2"],
      description="Graphenix library",
      # exclude_package_data={"": ["*.cpp", "*.h", "*.hpp", "*.pyx", "*.pxd"]},
      **__author_data)

