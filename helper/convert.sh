#!/bin/zsh

# https://lvgl.io/tools/imageconverter

sed -i '' '/#else/d' "$1.c"
sed -i '' '/#include "lvgl\/lvgl.h"/d' "$1.c"

sed -i '' 's/.header.cf = /{/g' "$1.c"
sed -i '' 's/.header.always_zero = //g' "$1.c"
sed -i '' 's/.header.reserved = //g' "$1.c"
sed -i '' 's/.header.w = //g' "$1.c"
sed -i '' 's/.header.h = //g' "$1.c"
sed -i '' 's/,*.data_size = /},/g' "$1.c"
sed -i '' 's/.data = //g' "$1.c"

mv "$1.c" "../src/$1.h"