_install_dependencies:
	sudo apt install python3.10-distutils
	sudo apt install libpython3.10-dev
	sudo pip3 install pybind11

_build:
	@echo "[graphenix] Running build..."
	# remove build files
	sudo rm -rf dist/
	sudo rm -rf build/
	sudo rm -rf graphenix_engine2.egg-info/

	# install library
	sudo python3 setup.py build --force
	sudo python3 setup.py install --force
	@echo "[graphenix] Finished build!"

_test:
	@echo "[graphenix] Running tests..."
	sudo python3 tests.py
	@echo "[graphenix] Finished tests!"