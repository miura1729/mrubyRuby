require 'mkmf'
$CFLAGS += ' -I ../../mruby/include'
$CFLAGS += ' -I ../../mruby/src'
$LOCAL_LIBS += ' ../../mruby/lib/ritevm.lib'
create_makefile("mruby");
