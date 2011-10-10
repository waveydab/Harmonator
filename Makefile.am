# Target executables
bin_PROGRAMS = harmonator midiclock

# Put the manual in "data" dir
htmldir = $(datadir)/html
html_DATA = harmonator.html

pkgdata_DATA = samples/*

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

CLEANFILES=harmonator_ui.cxx harmonator_ui.h

# To dump some variables
dbg_mk: EXTRA_DIST
	echo "bin:" $(bindir)
	echo "sbindir:" $(sbindir)

help:
	@echo "Available Make Targets"
	@echo "all: make all available targets"
	@echo "harmonator: make harmonator arpeggiator"
	@echo "midiclock: make midi time code generator"
