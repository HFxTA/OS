SRCROOT=./test_root
DSTROOT=./dst_root
BIN=`readlink -e $1`
PWD=`readlink -e $(pwd)`
echo "BIN:" $BIN
function prepare {
	pushd ${BIN}
	make
	popd

	mkdir -p ${DSTROOT}
	cp ${SRCROOT}/* ${DSTROOT}
}

function cleanup {
	pushd ${BIN}
	make clean
	popd

	rm -rf ${DSTROOT}
}

function check_execs {
	if [ -x ${BIN}/init ]
	then
		INIT=${BIN}/init
	else
		echo "init does not exist"
		exit 1
	fi

	if [ -x ${BIN}/format ]
	then
		FORMAT=${BIN}/format
	else
		echo "format does not exist"
		exit 1
	fi

	if [ -x ${BIN}/import ]
	then
		IMPORT=${BIN}/import
	else
		echo "import does not exist"
		exit 1
	fi

	if [ -x ${BIN}/export ]
	then
		EXPORT=${BIN}/export
	else
		echo "export does not exist"
		exit 1
	fi

	if [ -x ${BIN}/ls ]
	then
		LS=${BIN}/ls
	else
		echo "ls does not exist"
		exit 1
	fi

	if [ -x ${BIN}/copy ]
	then
		COPY=${BIN}/copy
	else
		echo "copy does not exist"
		exit 1
	fi

	if [ -x ${BIN}/move ]
	then
		MOVE=${BIN}/move
	else
		echo "move does not exist"
		exit 1
	fi

	if [ -x ${BIN}/rm ]
	then
		RM=${BIN}/rm
	else
		echo "rm does not exist"
		exit 1
	fi

	if [ -x ${BIN}/mkdir ]
	then
		MKDIR=${BIN}/mkdir
	else
		echo "mkdir does not exist"
		exit 1
	fi
}

function check_files {
	echo "test files ..."
	files=`ls ${DSTROOT} | grep -ivP "block\d+|config|d+"`
	if [ -n "$files" ]
	then
		echo "unexpected files" "$files"
		cleanup
		exit 1
	fi
}

function check_same {
	${EXPORT} ${DSTROOT} $1 ./tmp
	if ! cmp -s $2 ./tmp
	then
		rm -f ./tmp
		echo "file $1 differs from $2"
		cleanup
		exit 1
	fi
	rm -f ./tmp
}

echo "PREPARE"
prepare
echo "Cehk execs"
check_execs

echo "Pre export"
export LD_PRELOAD=/lib/x86_64-linux-gnu/libSegFault.so

echo -e "+++++++++++++++++ INIT +++++++++++++++++++++"
echo ${INIT}
${INIT} ${DSTROOT}
#check_files

echo -e "\n\n+++++++++++++++++ FORMAT +++++++++++++++++++++"
${FORMAT} ${DSTROOT}

echo -e "\n\n+++++++++++++++++ LS +++++++++++++++++++++"
lsresult=`${LS} ${DSTROOT} /`
echo -e "expected output empty\noutput: $lsresult"

echo -e "\n\n+++++++++++++++++ IMPORT /small +++++++++++++++++++++"
${IMPORT} ${DSTROOT} ./small /small
lsresult=`${LS} ${DSTROOT} /small`
echo -e "expected output /small <size>\noutput: $lsresult"

echo -e "\n\n+++++++++++++++++ IMPORT /medium +++++++++++++++++++++"
${IMPORT} ${DSTROOT} ./medium /medium
lsresult=`${LS} ${DSTROOT} /medium`
echo -e "expected output /medium <size>\noutput: $lsresult"

echo -e "\n\n+++++++++++++++++ IMPORT /large +++++++++++++++++++++"
${IMPORT} ${DSTROOT} ./large /large
lsresult=`${LS} ${DSTROOT} /large`
echo -e "expected output /large <size>\noutput: $lsresult"

echo -e "\n\n+++++++++++++++++ LS +++++++++++++++++++++"
lsresult=`${LS} ${DSTROOT} / | sort`
echo -e "expecected output:\nlarge f\nmedium f\nsmall f\noutput:\n$lsresult"


echo -e "\n\n++++++++++++++++++ CHECK FILES ++++++++++++++++++++++"
check_same "/small" "./small"
check_same "/medium" "./medium"
check_same "/large" "./large"


echo -e "\n\n++++++++++++++++++ MKDIR ++++++++++++++++++++++"
${MKDIR} ${DSTROOT} "/dir1"
${MKDIR} ${DSTROOT} "/dir2"
${MKDIR} ${DSTROOT} "/dir3/dir1"
${MKDIR} ${DSTROOT} "/dir3/dir2"

echo -e "\n\n+++++++++++++++++ LS +++++++++++++++++++++"
lsresult=`${LS} ${DSTROOT} / | sort`
echo -e "expecected output:\ndir1 d\ndir2 d\ndir3 d\nlarge f\nmedium f\nsmall f\noutput:\n$lsresult"

echo -e "\n\n+++++++++++++++++ LS +++++++++++++++++++++"
lsresult=`${LS} ${DSTROOT} /dir3 | sort`
echo -e "expecected output:\ndir1 d\ndir2 d\noutput:\n$lsresult"

echo -e "\n\n+++++++++++++++++ COPY +++++++++++++++++++++"
${COPY} ${DSTROOT} "/small" "/dir1/small"
${COPY} ${DSTROOT} "/small" "/dir2/small"
${COPY} ${DSTROOT} "/small" "/dir3/dir1/small"
${COPY} ${DSTROOT} "/small" "/dir3/dir2/small"

echo -e "\n\n+++++++++++++++++ LS +++++++++++++++++++++"
lsresult=`${LS} ${DSTROOT} /dir3 | sort`
echo -e "expected output:\ndir1 d\ndir2 d\noutput:\n$lsresult"


echo -e "\n\n+++++++++++++++++ CHECK FILES +++++++++++++++++++++"
check_same "/dir1/small" "./small"
check_same "/dir2/small" "./small"
check_same "/dir3/dir1/small" "./small"
check_same "/dir3/dir2/small" "./small"

echo -e "\n\n+++++++++++++++++ COPY +++++++++++++++++++++"
${COPY} ${DSTROOT} "/dir3" "/copy"

echo -e "\n\n+++++++++++++++++ LS +++++++++++++++++++++"
lsresult=`${LS} ${DSTROOT} /copy | sort`
echo -e "expected output:\ndir1 d\ndir2 d\noutput:\n$lsresult"

echo -e "\n\n+++++++++++++++++ CHECK FILES +++++++++++++++++++++"
check_same "/copy/dir1/small" "./small"
check_same "/copy/dir2/small" "./small"

echo -e "\n\n+++++++++++++++++ LS +++++++++++++++++++++"
lsresult=`${LS} ${DSTROOT} / | sort`
echo -e "expecected output:\ncopy d\ndir1 d\ndir2 d\ndir3 d\nlarge f\nmedium f\nsmall f\noutput:\n$lsresult"

echo -e "\n\n+++++++++++++++++ RM +++++++++++++++++++++"
${RM} ${DSTROOT} "/dir3"

echo -e "\n\n+++++++++++++++++ LS +++++++++++++++++++++"
lsresult=`${LS} ${DSTROOT} / | sort`
echo -e "expecected output:\ncopy d\ndir1 d\ndir2 d\nlarge f\nmedium f\nsmall f" "\noutput:\n$lsresult"

echo -e "\n\n+++++++++++++++++ CHECK FILES +++++++++++++++++++++"
check_same "/copy/dir1/small" "./small"
check_same "/copy/dir2/small" "./small"

echo -e "\n\n+++++++++++++++++ MOVE +++++++++++++++++++++"
${MOVE} ${DSTROOT} "/copy" "/dir3"


echo -e "\n\n+++++++++++++++++ LS +++++++++++++++++++++"
lsresult=`${LS} ${DSTROOT} / | sort`
echo -e "expecected output:\ndir1 d\ndir2 d\ndir3 d\nlarge f\nmedium f\nsmall f\noutput:\n$lsresult"

echo -e "\n\n+++++++++++++++++ CHECK FILES / +++++++++++++++++++++"
check_same "/dir3/dir1/small" "./small"
check_same "/dir3/dir2/small" "./small"

#check_files
cleanup
