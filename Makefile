.PHONEY: all clean cl rebuild rb install uninstall
all:
	@(cd c; make)
	@(cd cpp; make)
	@(cd samples; make)
	@(cd samples-c++; make)
clean cl:
	@(cd c; make clean)
	@(cd cpp; make clean)
	@(cd samples; make clean)
	@(cd samples-c++; make clean)

rebuild rb: cl all

install:
	@(cd c; make install)
	@(cd cpp; make install)

uninstall:
	@(cd c; make uninstall)
