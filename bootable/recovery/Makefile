version:
	@if [ -f .svn/entries ] ; then \
		echo "#define SVN_VERSION `sed -n -e 11p .svn/entries`" >version.h; \
	else \
	echo "#define SVN_VERSION 0" > version.h; \
	fi;

