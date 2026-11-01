
AM_CFLAGS = -DPROGRAM_NAME=\"nice\"

bin_PROGRAMS = nice
nice_LDADD = \
  ${abs_top_builddir}/../library/vfs/libvfs.la
nice_DEPENDENCIES = \
  ${abs_top_builddir}/../library/vfs/libvfs.la
nice_SOURCES = \
  rpc/nice/set.c \
  rpc/init.c \
  main.c
nice_LDFLAGS = -all-static --static
