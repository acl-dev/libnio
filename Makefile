all:
	@(cd c; make)
	@(cd samples; make)
clean cl:
	@(cd c; make clean)
	@(cd samples; make clean)
rebuild rb: cl all
