# See Makefile.delirium
# GCC_EXEC_PREFIX=i686-shikita-delirium-
CC=$(GCC_EXEC_PREFIX)gcc
CFLAGS=-Wall -ffreestanding -Wsystem-headers -mpush-args -nostdlib -g -DENABLE_ASSERTS -nostdinc -isystem../include/ -I../include  -DARCH_i386
