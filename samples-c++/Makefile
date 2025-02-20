all:
	@(cd timer; mkdir -p build; cd build; cmake ..; make)
	@(cd server; mkdir -p build; cd build; cmake ..; make)
	@(cd client; mkdir -p build; cd build; cmake ..; make)
	@(cd server2; mkdir -p build; cd build; cmake ..; make)
	@(cd client2; mkdir -p build; cd build; cmake ..; make)
clean cl:
	@(cd timer; rm -rf build timer)
	@(cd server; rm -rf build server)
	@(cd client; rm -rf build client)
	@(cd server2; rm -rf build server2)
	@(cd client2; rm -rf build client2)

rebuild rb: cl all

