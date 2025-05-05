VERSION := "0.1.0"

default:
  mkdir -p bin
  gcc src/*.c src/lib/*.c \
    -o bin/stb_stb \
    -Wall -Wextra -Werror -g -Og -Wno-unused-result \
    -DVERSION=\"{{VERSION}}\"
