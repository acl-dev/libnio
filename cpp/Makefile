.PHONEY: all clean install

all:
	@(mkdir -p build)
	@(cd build; cmake ..; make -j 4)

clean:
	@(rm -rf build)
	@(rm -rf lib)

install:
	@(cd build; make install)
