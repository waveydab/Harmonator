# Target executables
bin_PROGRAMS = harmonator midiclock


bin_SCRIPTS = harmonator.sh

# Put the manual in "data" dir
#htmldir = $(datadir)/harmonator
doc_DATA = harmonator.html


sampledir = $(pkgdatadir)/samples
sample_DATA = ./samples/*.hcf

#EXTRA_DIST = harmonator.html harmonator_ui.fl midiclock.fl samples
EXTRA_DIST = harmonator.html harmonator_ui.fl midiclock.fl samples

# Source files for target 'harmonator'
#harmonator_SOURCES = harmonator.cxx harmonator.h harmonator_ui.cxx harmonator_ui.h music_clock.cxx music_clock.h
harmonator_SOURCES = harmonator.cxx harmonator.h harmonator_ui.cxx harmonator_ui.h music_clock.cxx music_clock.h

#harmonator_BUILT_SOURCES = harmonator_ui.c harmonator_ui.h

# Source files for target 'midiclock'
midiclock_SOURCES = midiclock.cxx midiclock.h

BUILT_SOURCES = midiclock.cxx midiclock.h harmonator_ui.cxx harmonator_ui.h
#midiclock_SOURCES = midiclock.cxx midiclock.h



# rule for converting fluid '.fl' files to '.cxx' and '.h'
harmonator_ui.cxx harmonator_ui.h : harmonator_ui.fl
	fluid -c harmonator_ui.fl

midiclock.cxx midiclock.h: midiclock.fl
	fluid -c midiclock.fl

CLEANFILES=harmonator_ui.cxx harmonator_ui.h harmonator.sh

# To dump some variables
dbg_mk: EXTRA_DIST
	echo "bin:" $(bindir)
	echo "sbindir:" $(sbindir)

help:
	@echo "Major Available Make Targets"
	@echo "all: make all available targets"
	@echo "harmonator: make harmonator arpeggiator"
	@echo "midiclock: make midi time code generator"


# 'sampledir' and 'helppath' should get defined automatically. 'browserpath' will be defined by a 'configure' variable assignment.
harmonator.sh:
	echo -n \#\! " " > ./harmonator.sh
	which sh >> ./harmonator.sh 
	echo -n $(bindir)/harmonator >> ./harmonator.sh
	test ${sampledir} && (echo -n " --samples-directory=$(sampledir)" >> ./harmonator.sh) || true
	test -n @browser@ && (echo -n " --browser-path=@browser@" >> ./harmonator.sh) || true
	test -n @htmldir@ && (echo -n " --manual-path=@htmldir@/harmonator.html" >> harmonator.sh) || true
	echo -n ' $$@' >> ./harmonator.sh
