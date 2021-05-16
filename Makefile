# all: clean libsimplefs.a create_format app

# libsimplefs.a: 	simplefs.c
# 	gcc -Wall -c simplefs.c
# 	ar -cvq libsimplefs.a simplefs.o
# 	ranlib libsimplefs.a

# create_format: create_format.c
# 	gcc -Wall -o create_format  create_format.c   -L. -lsimplefs -lm

# app: 	app.c
# 	gcc -Wall -o app app.c  -L. -lsimplefs -lm

# clean: 
# 	rm -fr *.o *.a *~ a.out app  vdisk create_format


all: clean libsimplefs.a create_format app1 app2 app3 app4 app5 app6 app7 app8 app9

libsimplefs.a: 	simplefs.c
	gcc -Wall -c simplefs.c
	ar -cvq libsimplefs.a simplefs.o
	ranlib libsimplefs.a

create_format: create_format.c
	gcc -Wall -o create_format  create_format.c   -L. -lsimplefs -lm

app1: 	app1.c
	gcc -Wall -o app1 app1.c  -L. -lsimplefs -lm

app2: 	app2.c
	gcc -Wall -o app2 app2.c  -L. -lsimplefs -lm

app3: 	app3.c
	gcc -Wall -o app3 app3.c  -L. -lsimplefs -lm

app4: 	app4.c
	gcc -Wall -o app4 app4.c  -L. -lsimplefs -lm

app5: 	app5.c
	gcc -Wall -o app5 app5.c  -L. -lsimplefs -lm

app6: 	app6.c
	gcc -Wall -o app6 app6.c  -L. -lsimplefs -lm

app7: 	app7.c
	gcc -Wall -o app7 app7.c  -L. -lsimplefs -lm

app8: 	app8.c
	gcc -Wall -o app8 app8.c  -L. -lsimplefs -lm

app9: 	app9.c
	gcc -Wall -o app9 app9.c  -L. -lsimplefs -lm

clean: 
	rm -fr *.o *.a *~ a.out vdisk create_format app1 app2 app3 app4 app5 app6 app7 app8 app9