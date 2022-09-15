lib_LTLIBRARIES = dimensionDoorFlag.la

dimensionDoorFlag_la_SOURCES = dimensionDoorFlag.cpp
dimensionDoorFlag_la_CPPFLAGS= -I$(top_srcdir)/include -I$(top_srcdir)/plugins/plugin_utils
dimensionDoorFlag_la_LDFLAGS = -module -avoid-version -shared
dimensionDoorFlag_la_LIBADD = $(top_builddir)/plugins/plugin_utils/libplugin_utils.la

AM_CPPFLAGS = $(CONF_CPPFLAGS)
AM_CFLAGS = $(CONF_CFLAGS)
AM_CXXFLAGS = $(CONF_CXXFLAGS)

EXTRA_DIST = \
	README.dimensionDoorFlag.txt \
	dimensionDoorFlag.sln \
	dimensionDoorFlag.vcxproj	\
	dimensionDoorFlag.vcxproj.filters

MAINTAINERCLEANFILES =	\
	Makefile.in
