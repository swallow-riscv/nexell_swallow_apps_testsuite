DIR :=
DIR += src
DIR += test

all:
	@for dir in $(DIR); do	\
	make -C $$dir || exit $?;	\
	make -C $$dir install;	\
	done

clean:
	@for dir in $(DIR); do	\
	make -C $$dir clean || exit $?;	\
	done
