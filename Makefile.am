# $Id$
ACLOCAL_AMFLAGS = -I m4 -I libopts/m4

# test/unit holds the dependency-free unit tests, which `make check` recurses
# into.  test/ itself stays out of SUBDIRS: its integration suite needs root
# and a live NIC, so it is driven explicitly via `make test`, not by check.
if NEED_LIBOPTS
SUBDIRS = scripts lib $(LIBOPTS_DIR) src examples test/unit test/fuzz
else
SUBDIRS = scripts lib src examples test/unit test/fuzz
endif

DIST_SUBDIRS = scripts lib libopts src examples docs test test/unit test/fuzz
.PHONY: manpages docs test man2html


dist-hook: version manpages
	rm $(distdir)/src/defines.h

DOCS_DIR = $(top_builddir)/docs

manpages:
	cd src && make manpages

docs: manpages
	echo Making docs in $(DOCS_DIR)
	cd $(DOCS_DIR) && make docs

clean-docs:
	cd $(DOCS_DIR) && make clean-docs

postweb: docs doxygen
	cd $(DOCS_DIR) && make postweb

TEST_DIR = $(top_builddir)/test

autoopts:
	cd src && make autoopts

test:
	echo Making test in $(TEST_DIR)
	cd $(TEST_DIR) && make test

dlt_names:
	cat @SAVEFILE_C@ | $(top_builddir)/scripts/dlt2name.pl src/dlt_names.h

version:
	-rm -f src/common/git_version.c
	cd src/common && make git_version.c

distclean-local:
	-rm -rf autom4te.cache doxygen

doxygen: version
	doxygen doxygen.cfg

ncc:
	CC=ncc make
	
MOSTLYCLEANFILES = tcpreplay.spec *~

DISTCLEANFILES = .tm_project.cache stamp-h1 *.tar.*

MAINTAINERCLEANFILES = Makefile.in configure *.bak

EXTRA_DIST = doxygen.cfg.in \
	m4/libtool.m4 m4/ltoptions.m4 m4/ltsugar.m4 \
	m4/ltversion.m4 m4/lt~obsolete.m4 acinclude.m4 \
	CMakeLists.txt CMakePresets.json \
	cmake/config.h.cmake cmake/ConfigureChecks.cmake cmake/FindPCAP.cmake \
	cmake/GitVersion.cmake cmake/OptionalDeps.cmake cmake/PcapChecks.cmake
