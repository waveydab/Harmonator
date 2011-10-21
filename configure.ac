#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

#,----
#| Autoconf reqs
#`----

AC_PREREQ([2.67])
AC_INIT([harmonator],[1.0],[david.mcclanahan@gmail.com])
AM_INIT_AUTOMAKE(harmonator, 1.0)
AC_CONFIG_SRCDIR([music_clock.h])
AC_CONFIG_HEADERS([config.h])

AC_LANG([C++])
AC_PROG_CXX()


AC_ARG_VAR(
	[browser],
	[Specify browser exefile or exefile path]
)



# Disable library version checking
AC_ARG_ENABLE(
	[library_version_check],
	AS_HELP_STRING([--disable-library-version-check],[Disable library version checks(Enabled by default)
		]),
	[library_version_check=$enableval;],
	[library_version_check=yes;]
	)



AC_ARG_ENABLE(
	[test_configuration],
	AS_HELP_STRING([--enable-test-configuration],[Generate code to build testing version
	]),
	[test_configuration=$enableval;],
	[test_configuration=no;]
	)



#,----
#| Program checks
#`----

# Can we run perl
if !perl --version &> /dev/null; then 
	AC_MSG_ERROR(No Perl executable available.);	
fi

# todo: check for 'whereis'


# Checks for programs.

# function to return parts of fltk's version. Passed parameter can be
# 1 to 3 and designates major,minor,patch respectively
function fltk_version(){
[if [[ $(fltk-config --version) =~ ([0-9]+)\.([0-9]+)\.([0-9]+) ]]; then]
   echo ${BASH_REMATCH[[$1]]};
fi
}


# Later in processing I attempt to check for presence of necessary libraries. I could just use fltk-config output on my local machine and just check libraries based upon that. I think it better to use output from fltk-config run on the installer's machine. This seems to open a can of worms I don't wish to deal with right now which is trying to decipher fltk-config 's output for different compilation/link contexts to determine which libraries to check for. Yes, they're probably similar if not the same but I don't know that for sure, so I'm restricting the build to g++ for now.

if [[ $CXX != 'g++' ]]; then
	 AC_MSG_ERROR(Only supporting g++ compilation.);
fi

# Check for availability 'fltk-config'. Then check that available version is ok. I'm asserting that the local version should 1.x.y where x>=3 and y>=0
AC_MSG_CHECKING(Checking for presence of usable Fltk system.)
fltk_ok=0;
if fltk-config --version >&/dev/null; then {
      #fltk appears to be present
      if (($(fltk_version 1) == 1)); then {
         if (($(fltk_version 2) >= 3)); then {
             if (($(fltk_version 3) >= 0)); then {
             # get here-fltk version is ok
             fltk_ok=1;
             } fi
        } fi
    } fi 
} fi


if (($fltk_ok!=1)); then
	 AC_MSG_ERROR(Fltk not installed or wrong version);
else
AC_MSG_NOTICE([Fltk installed]);
fi

AC_MSG_CHECKING(Checking for Fltk's gui compiler.)
# Check for the fluid program
AC_ARG_VAR([FLUID],[Check for fltk's gui compiler])
AC_PATH_PROG([FLUID],[fluid])

# Library check
AC_MSG_NOTICE([=========== Checking library versions =============]);
[declare -a check_libs]
# versions of various libs. When the user runs configure, use these versions for comparison where applicable.
[check_libs=( ]
[	'{"fltk",(1 3)}' ]
[	'{"Xext",(6 4 0)}' ]
[	'{"Xft",(2 1 2)}']
[	'{"fontconfig",(1 4 4)}']
[	'{"Xinerama",(1 0 0)}']
[	'{"pthread",()}']
[	'{"dl",(2 11 2)}']
[	'{"m",(2 11 2)}']
[	'{"X11",(6 2 0)}']
[)]


[function cl_getlib(){]
[	local obj_str=$1]
[	echo $(echo $obj_str | perl -e '$_=<>;/{\s*"([^"]+)"/ && print $1;')]
[}]

[function cl_getversion(){]
#
[	local obj_str=$1]
[	debecho "$LINENO: obj_str=$obj_str"]
[	echo $(echo $obj_str | perl -e '$_=<>;/\(([0-9 ]*)\)/ && print $1;')]
[}]

[function cl_getobj(){]
# echo the stringified object whose library name matches libname(w/o 'lib' prefix)
[	local libname=$1]
[	local i]
[	for i in ${!check_libs[@]}]
[	do]
[		if test $(cl_getlib ${check_libs[$i]}) == $libname; then]
[			echo "${check_libs[$i]}"]
[			return 0]
[		fi]
[	done]
[	return 1]
[}]

# compare version 'a' against 'b' based upon 'major','minor','patch' convention
#	return		means
#	0			versions match
#	1			a is later version
#	-1			b is later version
# 'Major' field must be present on both versions. Other missing fields of either version is treated as "earliest" 
[function compare_version_number(){]
[local a=( $1 )]
[local b=( $2 )]
[local a_major=${a[0]}]
[local a_minor=${a[1]}]
[local a_patch=${a[2]}]
[local b_major=${b[0]}]
[local b_minor=${b[1]}]
[local b_patch=${b[2]}]
[local rtn]
#debecho "a_major: $a_major"
#debecho "b_major: $b_major"
[function compare_field(){]
[	local a_field=$1]
[	local b_field=$2]
	#debecho "a_field: $a_field b_field $b_field"
	# empty so "equal"
[	if [[ -z "$a_field" && -z "$b_field" ]]; then debecho "$LINENO:"; compare_field=0;return;fi]
[	if [[ -z "$a_field" ]]; then debecho "$LINENO:";compare_field=1;return;fi]
[	if [[ -z "$b_field" ]]; then debecho "$LINENO:";compare_field=-1;return;fi]
[	if [ $a_field -gt $b_field ]; then debecho "$LINENO:";compare_field=1;return;fi]
[	if [ $a_field -lt $b_field ]; then debecho "$LINENO:";compare_field=-1;return;fi]
[	debecho "$LINENO:";]
[	compare_field=0]
[}]

# if a or b's major field missing return 666
# Compare major fields
# if not equal
#	if a.major earlier b.major return 1 else return -1
# if a and b's minor missing return 0
# Compare minor fields
# if not equal
#	if a.major earlier b.major return 1 else return -1
# Compare patch fields
# if not equal
#	if a.patch earlier b.patch return 1 else return -1
# return 0
[if [[ -z "$a_major" && -z "$b_major" ]]; then]
[	debecho "$LINENO: invalid versions"]
[	echo Invalid versions > /dev/stderr]
[	compare_version_number=66]
[	return;]
[fi]
[compare_field $a_major $b_major;]
[if [[ $compare_field -ne 0 ]]; then debecho "$LINENO: majors not equal(rtn=$compare_field)";compare_version_number=$compare_field;return;fi]
[]
[compare_field $a_minor $b_minor;]
[if [[ $compare_field -ne 0 ]]; then debecho "$LINENO: minors not equal(rtn=$compare_field)";compare_version_number=$compare_field;return;fi]

[compare_field $a_patch $b_patch]
[debecho "$LINENO: compare_field=$compare_field"]
[if [[ $compare_field -ne 0 ]]; then debecho "$LINENO: patches not equal(rtn=$compare_field)";compare_version_number=$compare_field;return;fi]
#debecho "$LINENO: versions equal";
[compare_version_number=0]
[}]

[function debecho(){]
[	if [ ! -z "$DEBUG" ]; then]
[		echo "$1" >&2]
[	fi]
[}]

[function real_lib_fnc(){]
[if whereis $1 > /dev/null; then]
[	echo $(perl -e 	'$wi = `whereis $ARGV[0]`;$foo = "$ARGV[0]";if ($wi =~ / ([^ ]*?$foo\.so)/){if (-l $1){print `readlink -e $1`;}else {print $1;}}' $1)]
[	return 0]
[else]
[	return 1]
[fi]
[}]



# return the version of a shared library in an array (major minor patch)
[declare -a lib_version;]
[function library_version(){]
[	echo $(echo $1 | perl -e '$_=<>;if (/so\.(\d+)(?:\.(\d+)(?:\.(\d+))?)?/) {print "$1 $2 $3";exit;}if (/(\d+)(?:\.(\d+)(?:\.(\d+))?)?\.so/) {print "$1 $2 $3";}')]
[}]

[declare -a fltk_lib_list]
# fetch full path to libraries specified by fltk-config

[function get_fltk_libs(){]
[	local switch]
[	for switch in $(fltk-config --ldflags)]
[	do]
[		if [[ $switch =~ '-l' ]]]
[		then]
[			if real_lib_fnc "lib${switch#-l}">/dev/null;]
[			then]
[				fltk_lib_list[${#fltk_lib_list[*]}]=$(real_lib_fnc "lib${switch#-l}")]
[			fi]
[		fi]
[	done]
[}]



[for bare_lib in $(fltk-config --ldflags | perl -e '$_=<>;/ (-l.*)$/ && print $1')]
[do]
[	debecho Searching for ${bare_lib#-l}]
#Was this lib rq'd on development environment?
[	if cl_getobj ${bare_lib#-l} > /dev/null; then]
	# yes-check version of local lib against expected
[		expected_lib_ver=$(cl_getversion "$(cl_getobj ${bare_lib#-l})")]
[		local_lib_ver=$(library_version "$(real_lib_fnc lib${bare_lib#-l})")]
[		debecho "expected_lib_ver $expected_lib_ver"]
[		debecho "local_lib_ver $local_lib_ver"]
# expected version defined do a compare		
[		if test "$expected_lib_ver" && test $library_version_check == "yes" ; then]
[			echo -n "Check ${bare_lib#-l} version... "]
[			compare_version_number "$expected_lib_ver" "$local_lib_ver"]
[			if [[ $compare_version_number -eq 0 ]]; then]
[				debecho "$LINENO: $1 and $2 are equal"]
[				echo "Versions match."]
[			elif [[ $compare_version_number -eq 1 ]]; then]
[				debecho "$LINENO: $1 later"]
[				echo "Local library is an earlier version."]
[			else]
[				debecho "$LINENO: $2 later"]
[				echo "Local library version is a later version."]
[			fi]
[		else]
[			echo "No check version for lib${bare_lib#-l} and/or version checking disabled. Skipping"]
[		fi]
[	else]
[		echo "lib${bare_lib#-l} was not in development environment. No check available."]
[	fi]
[done]







# Checks for header files.
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL

# Checks for library functions.
AC_FUNC_ALLOCA
AC_CHECK_FUNCS([gettimeofday memset strcasecmp strchr strdup strerror strpbrk strstr])

if test -e /tmp; then
	#prefix=/tmp/temp
	temp_fifo=/tmp/temp.$(date +%s)
	mkfifo $temp_fifo
else
	#prefix=$PWD
	temp_fifo=$PWD.$(date +%s)
	mkfifo $temp_fifo
fi

#suffix=$(date +%s)
#temp_fifo=$prefix.$suffix
#mkfifo $temp_fifo
header_check=0

# checks with regard to harmonator
AC_MSG_NOTICE([======== Check Inclusions for 'harmonator' source ========])
AC_CHECK_HEADER(vector,[],[header_check=1])
AC_CHECK_HEADER(alsa/asoundlib.h,[],[header_check=1])
AC_CHECK_HEADER(time.h,[],[header_check=1])
AC_CHECK_HEADER(iostream,[],[header_check=1])
AC_CHECK_HEADER(iterator,[],[header_check=1])
AC_CHECK_HEADER(set,[],[header_check=1])
AC_CHECK_HEADER(map,[],[header_check=1])
AC_CHECK_HEADER(numeric,[],[header_check=1])
AC_CHECK_HEADER(algorithm,[],[header_check=1])

AC_MSG_NOTICE([======== Check Inclusions for 'music_clock' source ========])
AC_CHECK_HEADER(string,[],[header_check=1])
AC_CHECK_HEADER(cstdlib,[],[header_check=1])
AC_CHECK_HEADER(sstream,[],[header_check=1])
AC_CHECK_HEADER(iostream,[],[header_check=1])
AC_CHECK_HEADER(iomanip,[],[header_check=1])
AC_CHECK_HEADER(cassert,[],[header_check=1])




# checks for fluid generated inclusions with regards to harmonator_ui
AC_MSG_NOTICE([======== Check Inclusions for 'harmonator_ui.cxx' source ========])
for i in $(fluid -c -h /dev/null -o $temp_fifo harmonator_ui.fl & (cat $temp_fifo | perl -e '
while (<>) {if ($_ =~ "^#include *<([a-zA-Z_./]+)>") {print qq/$1 /}};'))
do
	AC_CHECK_HEADER($i,[],[header_check=1])
done


AC_MSG_NOTICE([======== Check Inclusions for 'harmonator_ui.h' source ========])
for i in $(fluid -c -o /dev/null -h $temp_fifo harmonator_ui.fl & (cat $temp_fifo | perl -e '
while (<>) {if ($_ =~ "^#include *<([a-zA-Z_./]+)>") {print qq/$1 /}};'))
do
	AC_CHECK_HEADER($i,[],[header_check=1])
done

rm $temp_fifo



# define some build variables
if test "x${browser}" != x; then # string 'browser' 
if test -x "${browser}"; then
echo foo > /dev/null
elif which "${browser}" > /dev/null; then
browser=$(which $browser)
fi
fi 




LDFLAGS=$(fltk-config --ldflags | perl -e 'while (<>){if ($_ =~ m/(-L.+?)\s+-l/){print $1;}}')
LIBS=$(fltk-config --ldflags | perl -e 'while (<>){if ($_ =~ m/(-l.+)/){print $1;}}')
LIBS="$LIBS -lasound"
[CPPFLAGS="$(fltk-config --cxxflags) -DDATADIR=\"${datadir}/harmonator/samples\""]
# add a switch to enable the testing entry
#CXXFLAGS='-g -O'
[if [[ "${test_configuration}" == "yes" ]] ; then]
[CXXFLAGS='-g -coverage -pg'] # dbg, coverage instr, etc.
[CPPFLAGS="$CPPFLAGS -DHARMONATOR_TEST -DDATADIR=\"${datadir}/harmonator/samples\""]
[LIBS="$LIBS -lcppunit"]
[fi]

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
