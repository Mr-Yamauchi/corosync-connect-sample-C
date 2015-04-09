SUBDIRS = lib
bin_PROGRAMS = darkside
darkside_LDFLAGS = -L./lib 
darkside_LDADD = -lcpg -lquorum -ldarkside
darkside_CFLAGS =  -I ./lib 
#test_SOURCES = main.c subs.c message.c
darkside_SOURCES = main.c subs.c 
