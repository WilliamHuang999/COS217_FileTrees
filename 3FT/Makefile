all: ft

ft: dynarray.o path.o nodef.o noded.o ft.o ft_client.o
	gcc217 -g dynarray.o path.o noded.o nodef.o ft.o ft_client.o -o ft

dynarray.o: dynarray.c dynarray.h
	gcc217 -g -c dynarray.c

path.o: path.c path.h
	gcc217 -g -c path.c

ft_client.o: ft_client.c ft.h a4def.h
	gcc217 -g -c ft_client.c

nodef.o: nodef.c nodef.h path.h a4def.h
	gcc217 -g -c nodef.c

noded.o: noded.c dynarray.h nodef.h noded.h path.h a4def.h
	gcc217 -g -c noded.c

ft.o: ft.c dynarray.h noded.h nodef.h ft.h path.h a4def.h
	gcc217 -g -c ft.c