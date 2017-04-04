file dskyline.bin dskyline.syms
load mram bin runtime_data/dsky.bin
boot
print mram 0x370F000 32
quit