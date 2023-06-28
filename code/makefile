all : c1-term.out c1-disp.out c2-term.out c2-disp.out c3-term.out c3-disp.out c4-term.out c4-disp.out

c1-term.out : c-term.c
	gcc -o c-term-1 -DCLIENT1 c-term.c
 
c1-disp.out : c-disp.c
	gcc -o c-disp-1 -DCLIENT1 c-disp.c
c2-term.out : c-term.c
	gcc -o c-term-2 -DCLIENT2 c-term.c

c2-disp.out : c-disp.c
	gcc -o c-disp-2 -DCLIENT2 c-disp.c
c3-term.out : c-term.c
	gcc -o c-term-3 -DCLIENT3 c-term.c

c3-disp.out : c-disp.c
	gcc -o c-disp-3 -DCLIENT3 c-disp.c
c4-term.out : c-term.c
	gcc -o c-term-4 -DCLIENT4 c-term.c

c4-disp.out : c-disp.c
	gcc -o c-disp-4 -DCLIENT4 c-disp.c