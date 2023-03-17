_install_dependencies:
	sudo apt install python3.10-distutils
	sudo apt install libpython3.10-dev

_build:
	@echo "[graphenix] Running build..."
	sudo python3 setup.py install
	@echo "[graphenix] Finished build!"

_test:
	@echo "[graphenix] Running tests..."
	python3 tests.py
	@echo "[graphenix] Finished tests!"