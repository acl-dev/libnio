.PHONEY: all clean cl rebuild rb install uninstall
all:
	@(cd c; make)
	@(cd samples; make)
clean cl:
	@(cd c; make clean)
	@(cd samples; make clean)
rebuild rb: cl all

install:
	@(cd c; make install)

uninstall:
	@(cd c; make uninstall)
