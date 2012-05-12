require 'mkmf'
$CFLAGS += ' -I ../../mruby/include'
$CFLAGS += ' -I ../../mruby/src'
$LOCAL_LIBS += ' ../../mruby/lib/libmruby.a'
create_makefile("mruby");
