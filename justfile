exe := if os_family() == "windows" { ".exe" } else { "" }
bin := "bin/stb_stb" + exe
flags := "-std=gnu17 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wstrict-prototypes -Wmissing-prototypes -Werror -g -Og"

default: build

build:
    mkdir -p bin
    clang {{flags}} src/*.c -o {{bin}}

# Packages stb_stb's own sources into stb_stb.h, then rebuilds the tool from that header and checks it reproduces the same header.
stb: build
    {{bin}} stb src/*.c src/lib/*.h --overwrite -o stb_stb.h
    clang {{flags}} -DSTB_STB_IMPLEMENTATION -x c stb_stb.h -o bin/stb_stb_packaged{{exe}}
    bin/stb_stb_packaged{{exe}} stb src/*.c src/lib/*.h --overwrite -o bin/stb_stb_check.h
    diff stb_stb.h bin/stb_stb_check.h

clean:
    rm -rf bin

# Runs the unit tests (test/unit.c) and the command-line tests (test/cli.sh).
test: build
    clang {{flags}} -Isrc test/unit.c src/c.c src/src_type.c -o bin/unit{{exe}}
    bin/unit{{exe}}
    sh test/cli.sh {{bin}}
