default: clean build view

clean:
	rm output/* || true
	rm -r doxyreport/* || true

build:
	python3 emitter.py --templates templates/c
	clang-format --style=file:../clang-format-h -i output/*.h

view:
	less output/libxnvme_spec.h

doxy:
	doxygen doxy.cfg
