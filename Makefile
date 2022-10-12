_install_dependencies:
	sudo apt install python3.10-distutils
	sudo apt install libpython3.10-dev

_install_pandoc:
	sudo apt install pandoc texlive-latex-extra

_build:
	@echo "[pybase] Running build..."
	sudo python3 setup.py install
	@echo "[pybase] Finished build!"

_test:
	@echo "[pybase] Running tests..."
	sudo rm -f -r ./_pyb/*
	python3 test.py -b
	@echo "[pybase] Finished tests!"

_test_echo:
	@echo "[pybase] Running tests..."
	sudo rm -f -r ./_pyb/*
	python3 test.py
	@echo "[pybase] Finished tests!"

_docs:
	@echo "[pybase] Generating docs..."
	pandoc -V geometry:margin=.75in -o ./pdfs/IDEA.pdf ./documentation/IDEA.md
	@echo "[pybase] Finished docs!"