lib_LTLIBRARIES             = libcoloredstderr.la
libcoloredstderr_la_SOURCES = coloredstderr.c \
                              compiler.h \
                              constants.h \
                              debug.h \
                              hookmacros.h \
                              ldpreload.h \
                              trackfds.h

# Make sure the library is not writable. See README why this is important. Is
# not run with `make libcoloredstderr.la`, but this isn't common usage.
all-local: $(lib_LTLIBRARIES)
	find .libs -type f -exec chmod ugo-w '{}' \;
# `install` uses normal permissions, "fix" them so `make install` also
# installs non-writable files.
install-exec-hook:
	find $(DESTDIR)$(libdir) -type f -exec chmod ugo-w '{}' \;
