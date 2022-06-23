_build:
	@echo "[pybase] Running build..."
	sudo python3 setup.py install
	@echo "[pybase] Finished build!"

_test:
	@echo "[pybase] Running tests..."
	python3 test.py -b
	@echo "[pybase] Finished tests!"