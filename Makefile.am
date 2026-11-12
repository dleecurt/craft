## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = $(X_CFLAGS) -DCRAFT_DATADIR='"$(pkgdatadir)"'
AM_CXXFLAGS = $(WARN_CXXFLAGS)
LDADD       = libcraft.a $(X11_LIBS) $(LIBS)

# ----------------------------------------------------------------------
# Shared code: everything except the main programs
# ----------------------------------------------------------------------

noinst_LIBRARIES = libcraft.a
libcraft_a_SOURCES = \
	building.cc building.h \
	buttons.cc buttons.h \
	cmap.cc cmap.h \
	cmap_edit.cc cmap_edit.h \
	cmap_selector.cc cmap_selector.h \
	color_trans.cc color_trans.h \
	compress.cc compress.h \
	cry.cc cry.h \
	datadir.cc datadir.h \
	dial.cc dial.h \
	dir.cc dir.h \
	errorhandling.cc errorhandling.h \
	field.cc field.h \
	file_selector.cc file_selector.h \
	getline.cc getline.h \
	history.cc history.h \
	id_handler.cc id_handler.h \
	ilfield.cc ilfield.h \
	io.cc io.h \
	land.cc land.h \
	lru.cc lru.h \
	masks.cc masks.h \
	menu.cc menu.h \
	morph.cc morph.h \
	morph_edit.cc morph_edit.h \
	multi_dial.cc multi_dial.h \
	object_handler.cc object_handler.h \
	objects.cc objects.h \
	option_menu.cc option_menu.h \
	paramreader.cc paramreader.h \
	pattern_match.cc pattern_match.h \
	pic.cc pic.h \
	player.cc player.h \
	ppm.cc ppm.h \
	ppm_handler.cc ppm_handler.h \
	robot.cc robot.h \
	savegame.cc savegame.h \
	screens.cc screens.h \
	scroller.cc scroller.h \
	selector.cc selector.h \
	setupedit.cc setupedit.h \
	ship.cc ship.h \
	sound.cc sound.h \
	table.cc table.h \
	win.cc win.h \
	xbm.cc xbm.h \
	xfile.cc xfile.h \
	xmath.cc xmath.h \
	xstring.cc xstring.h \
	zoom.cc zoom.h \
	xtimer.cc xtimer.h \
	bool.h craft.h craft_def.h

# ----------------------------------------------------------------------
# The game
# ----------------------------------------------------------------------

bin_PROGRAMS = craft
craft_SOURCES = craft.cc

# ----------------------------------------------------------------------
# Art tools (installed with a craft- prefix)
# ----------------------------------------------------------------------

if BUILD_TOOLS
bin_PROGRAMS += \
	craft-cmedit craft-compress craft-medit craft-morph craft-mirror \
	craft-ppm2cmap craft-recolor craft-shade craft-shade1 craft-split
endif

craft_cmedit_SOURCES   = cmedit.cc
craft_compress_SOURCES = hcomp.cc
craft_medit_SOURCES    = hmedit.cc
craft_morph_SOURCES    = hmorph.cc
craft_mirror_SOURCES   = mirror.cc
craft_ppm2cmap_SOURCES = ppm2cmap.cc
craft_recolor_SOURCES  = recolor.cc
craft_shade_SOURCES    = shade.cc
craft_shade1_SOURCES   = shade1.cc
craft_split_SOURCES    = split.cc

# ----------------------------------------------------------------------
# Data files: pictures, button icons and window defaults
# ----------------------------------------------------------------------

data_dirs = hcraft buttons

install-data-local:
	$(MKDIR_P) "$(DESTDIR)$(pkgdatadir)"
	$(INSTALL_DATA) "$(srcdir)/.windefaults.params" "$(DESTDIR)$(pkgdatadir)/.windefaults.params"
	for d in $(data_dirs); do \
	  $(MKDIR_P) "$(DESTDIR)$(pkgdatadir)/$$d" || exit 1; \
	  (cd "$(srcdir)/$$d" && $(INSTALL_DATA) * "$(DESTDIR)$(pkgdatadir)/$$d/") || exit 1; \
	done
	$(MKDIR_P) "$(DESTDIR)$(docdir)/html/pic"
	$(INSTALL_DATA) "$(srcdir)"/doc/*.html "$(DESTDIR)$(docdir)/html/"
	$(INSTALL_DATA) "$(srcdir)"/doc/pic/* "$(DESTDIR)$(docdir)/html/pic/"

uninstall-local:
	rm -rf "$(DESTDIR)$(pkgdatadir)" "$(DESTDIR)$(docdir)/html"

dist_doc_DATA = README.md LICENSE LICENSE-NOTES.md

dist_man_MANS = craft.6

EXTRA_DIST = autogen.sh .clang-format .windefaults.params

# Directories are copied here rather than listed in EXTRA_DIST: 'buttons'
# would otherwise match make's built-in rule for building buttons from
# buttons.cc.
dist-hook:
	for d in $(data_dirs) doc; do \
	  cp -pR "$(srcdir)/$$d" "$(distdir)/" || exit 1; \
	done
	chmod -R u+w "$(distdir)"
