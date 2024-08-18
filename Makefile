build: src/main.c
	mkdir -p ./build
	gcc -O3 -Wall -o ./build/game src/main.c -I./src/raylib-5.0_linux_amd64/include -L./src/raylib-5.0_linux_amd64/lib ./src/raylib-5.0_linux_amd64/lib/libraylib.a -lraylib -lm -lpthread -ldl

build-win: src/main.c
	mkdir -p ./build
	x86_64-w64-mingw32-gcc -O3 -Wall -o ./build/game.exe src/main.c -I./src/raylib-5.0_win64_mingw-w64/include -L./src/raylib-5.0_win64_mingw-w64/lib ./src/raylib-5.0_win64_mingw-w64/lib/libraylib.a -lraylib -lm -lwinmm -lgdi32

run: build
	./build/game

clean:
	rm -rf ./build