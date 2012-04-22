#include "ruby.h"
#define st_foreach_safe mruby_st_foreach_safe
#define RBasic mruby_RBasic
#define RObject mruby_RObject
#include "mruby.h"
#include "mruby/proc.h"
#include "compile.h"
#include "dump.h"
#include "stdio.h"
#include "string.h"

VALUE mruby_cMRuby = Qnil;
VALUE mruby_eval(VALUE, VALUE);

void Init_mruby() {
  mruby_cMRuby = rb_define_class("MRuby", rb_cObject);
  rb_define_singleton_method(mruby_cMRuby, "eval", mruby_eval, 1);
}

VALUE mruby_eval(VALUE self, VALUE prog) {
  int n = -1;
  mrb_state *mrb = mrb_open();
  struct mrb_parser_state *p;

  p = mrb_parse_string(mrb, StringValueCStr(prog));
  n = mrb_generate_code(mrb, p->tree);
  if (n >= 0) {
    mrb_run(mrb, mrb_proc_new(mrb, mrb->irep[n]), mrb_nil_value());
  }

  return Qnil;
}
