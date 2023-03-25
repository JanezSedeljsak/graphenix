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
	sudo rm -rf graphenix.egg-info/

	# install library
	sudo python3 setup.py install --force
	@echo "[graphenix] Finished build!"

_unit_test:
	@echo "[graphenix] Running tests..."
	sudo python3 -m tests.tests_unit
	@echo "[graphenix] Finished tests!"

_perf_test:
	@echo "[graphenix] Running tests..."
	sudo python3 -m tests.tests_perf
	@echo "[graphenix] Finished tests!"