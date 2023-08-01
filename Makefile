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

# --------------------------------
# tests section
# --------------------------------

_unit_test:
	@echo "[graphenix] Running tests..."
	sudo python3 -m tests.tests_unit
	@echo "[graphenix] Finished tests!"

_cpp_test:
	@echo "[graphenix] Running low level tests..."
	chmod +x cpp_tests.sh
	./cpp_tests.sh
	@echo "[graphenix] Finished low level tests!"

_perf_test:
	@echo "[graphenix] Running tests..."
	sudo python3 -m tests.tests_perf
	@echo "[graphenix] Finished tests!"

# --------------------------------
# analysis section
# --------------------------------

_insert_a:
	@echo "[graphenix] Running single insert analysis..."
	chmod +x analysis_runner.sh
	./analysis_runner.sh singleinsert 5000 10000 50000 100000 150000 200000
	@echo "[graphenix] Finished single insert analysis!"

_read_a:
	@echo "[graphenix] Running single read analysis..."
	chmod +x analysis_runner.sh
	./analysis_runner.sh singleread 10000 50000 100000 150000 200000
	@echo "[graphenix] Finished single read analysis!"

_find_a:
	@echo "[graphenix] Running find without index analysis..."
	chmod +x analysis_runner.sh
	./analysis_runner.sh find_no_index 100000 1000000
	@echo "[graphenix] Finished find without index analysis!"

_findi_a:
	@echo "[graphenix] Running find with index analysis..."
	chmod +x analysis_runner.sh
	./analysis_runner.sh find_index 100000 1000000
	@echo "[graphenix] Finished find with index analysis!"

_join_a:
	@echo "[graphenix] Running find with index analysis..."
	chmod +x analysis_runner.sh
	./analysis_runner.sh join 1000 10000 100000 200000
	@echo "[graphenix] Finished find with index analysis!"