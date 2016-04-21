include buildcfg.mk

DIR		:= 
######################################################################
# Build options

# VPU V4L2 Driver Wrapper
DIR += src

# V4L2 Test Module
DIR += test

######################################################################
# Build
all:
	@for dir in $(DIR); do			\
	make -s -C $$dir || exit $?;	\
	done

install:
	@for dir in $(DIR); do			\
	make -C $$dir install || exit $?;	\
	done

clean:
	@for dir in $(DIR); do			\
	make -s -C $$dir clean || exit $?;	\
	done

distclean:
	@for dir in $(DIR); do			\
	make -C $$dir distclean || exit $?;	\
	done
