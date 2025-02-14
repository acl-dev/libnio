.PHONEY: all clean

all:
	@(mkdir -p build)
	@(cd build; cmake ..; make -j 4)

clean:
	@(rm -rf build)
