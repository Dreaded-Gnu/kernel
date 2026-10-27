
AM_CFLAGS = -DPROGRAM_NAME=\"idle\"

bin_PROGRAMS = idle
idle_LDADD = \
  ${abs_top_builddir}/../library/vfs/libvfs.la
idle_DEPENDENCIES = \
  ${abs_top_builddir}/../library/vfs/libvfs.la
idle_SOURCES = \
  main.c
idle_LDFLAGS = -all-static --static
