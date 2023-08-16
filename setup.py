from setuptools import setup, find_packages

__version__ = "0.0.5"
__engine_name__ = "graphenix_engine2"
__name__ = "graphenix"

__author_data = {"author": "janezsedeljsak",
                 "author_email": "janez.sedeljsak@gmail.com"}

setup(name=__name__,
      version=__version__,
      packages=find_packages(exclude=["*.tests", "*.tests.*", "tests.*", "tests"]),
      description="Graphenix library",
      # exclude_package_data={"": ["*.cpp", "*.h", "*.hpp", "*.pyx", "*.pxd"]},
      **__author_data)

