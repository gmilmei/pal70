PAL70=../src/pal70 -v

all: fact_run.pocode list_run.pocode

list_run.pocode: list.pal list_run.pal
	${PAL70} -c -o $@ $^

fact_run.pocode: fact.pal fact_run.pal
	${PAL70} -c -o $@ $^

runall: all
	${PAL70} fact_run.pocode
	${PAL70} list_run.pocode

%.pocode: %.pal
	${PAL70} -c -o $@ $<

clean:
	rm -f *.pocode
