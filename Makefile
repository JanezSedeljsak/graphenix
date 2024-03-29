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

	# install engine
	sudo python3 graphenix/engine/setup.py install --force

	# install wrapper lib
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

_all_a: _insert_a _iinsert_a _find_make_base _sizes_a _read_a _qread_a _ifind_a _find_a _join_a _agg_a
	@echo "[graphenix] All analysis completed!"

_all_graphs:
	python3 -m analysis.singleinsert.graph
	python3 -m analysis.singleinsert.sizes_analysis 1000000
	python3 -m analysis.indexinsert.graph
	python3 -m analysis.queryread.graph
	python3 -m analysis.singleread.graph
	python3 -m analysis.join.graph
	python3 -m analysis.agg.graph
	python3 -m analysis.find_no_index.graph_size 100000
	python3 -m analysis.find_no_index.graph_size_single 1000000
	@echo "[graphenix] All graphs completed!"

_insert_a:
	@echo "[graphenix] Running single insert analysis..."
	chmod +x analysis_runner.sh
	./analysis_runner.sh singleinsert 5000 10000 50000 100000 150000
	@echo "[graphenix] Finished single insert analysis!"

_iinsert_a:
	@echo "[graphenix] Running indexed insert analysis..."
	chmod +x analysis_runner.sh
	./analysis_runner.sh indexinsert 5000 10000 50000 100000 150000
	@echo "[graphenix] Finished indexed insert analysis!"

_read_a:
	@echo "[graphenix] Running single read analysis..."
	chmod +x analysis_runner.sh
	./analysis_runner.sh singleread 10000 50000 100000 150000
	@echo "[graphenix] Finished single read analysis!"

_qread_a:
	@echo "[graphenix] Running query read analysis..."
	chmod +x analysis_runner.sh
	./analysis_runner.sh queryread 10000 50000 100000 150000
	@echo "[graphenix] Finished query read analysis!"

_find_make_base:
	@echo "[graphenix] Creating large DBs for find analysis..."
 
	# create DBs with 1M rows
	python3 -m analysis.singleinsert.raw_sqlite 1000000
	python3 -m analysis.singleinsert.graphenix_bulk 1000000
	python3 -m analysis.singleinsert.mysql_bulk 1000000

	# create indexed DBs with 1M rows
	python3 -m analysis.indexinsert.raw_sqlite 1000000
	python3 -m analysis.indexinsert.graphenix_bulk 1000000
	python3 -m analysis.indexinsert.mysql_bulk 1000000

	@echo "[graphenix] Finished creating large DBs!"

_find_a:
	@echo "[graphenix] Running find without index analysis..."
	chmod +x analysis_runner.sh
	./analysis_runner.sh find_no_index 100000 1000000
	@echo "[graphenix] Finished find without index analysis!"

_ifind_a:
	@echo "[graphenix] Running find with index analysis..."
	chmod +x analysis_runner.sh
	./analysis_runner.sh find_index 100000 1000000
	python3 -m analysis.find_no_index.graph_size 100000
	python3 -m analysis.find_no_index.graph_size_single 1000000
	@echo "[graphenix] Finished find with index analysis!"

_join_a:
	@echo "[graphenix] Running join analysis..."
	chmod +x ./analysis/join/run_insert.sh
	./analysis/join/run_insert.sh 1000 5000 10000 20000
	chmod +x analysis_runner.sh
	./analysis_runner.sh "join" 1000 5000 10000 20000
	@echo "[graphenix] Finished join analysis!"

_sizes_a:
	@echo "[graphenix] Running size analysis..."
	chmod +x ./analysis/singleinsert/file_sizes.sh
	./analysis/singleinsert/file_sizes.sh 1000000
	./analysis/singleinsert/file_sizes_indexed.sh 1000000
	python3 -m analysis.singleinsert.sizes_analysis 1000000
	@echo "[graphenix] Finished size analysis!"

_agg_a:
	@echo "[graphenix] Running agg analysis..."
	chmod +x analysis_runner.sh
	./analysis_runner.sh "agg" 1000 5000 10000 20000
	@echo "[graphenix] Finished agg analysis!"