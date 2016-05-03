DIR :=
DIR += allocator_test
DIR += camera_test
DIR += dp_cam_test
DIR += libnx_video_api

all:
	@for dir in $(DIR); do	\
	make -C $$dir || exit $?;	\
	make -C $$dir install;	\
	done

clean:
	@for dir in $(DIR); do	\
	make -C $$dir clean || exit $?;	\
	done
