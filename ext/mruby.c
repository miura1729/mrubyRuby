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

struct mrRArray {
  MRUBY_OBJECT_HEADER;
  size_t len;
  size_t capa;
  mrb_value *buf;
};

struct mrRString {
  MRUBY_OBJECT_HEADER;
  size_t len;
  union {
    size_t capa;
    mrb_value shared;
  } aux;
  char *buf;
};

VALUE mruby_cvt_mr2cr(mrb_value obj) {
  switch (obj.tt) {
  case MRB_TT_ICLASS:
  case MRB_TT_CLASS:
  case MRB_TT_SCLASS:
  case MRB_TT_MODULE:
  case MRB_TT_OBJECT:
  case MRB_TT_ENV:
  case MRB_TT_PROC:
  case MRB_TT_RANGE:
  case MRB_TT_SYMBOL:
    break;

  case MRB_TT_TRUE:
    return Qtrue;
    break;

  case MRB_TT_FIXNUM:
    return INT2NUM(obj.value.i);
    break;

  case MRB_TT_FLOAT:
    return rb_float_new(obj.value.f);
    break;

  case MRB_TT_ARRAY:
    {
      struct mrRArray *a = ((struct mrRArray*)((obj).value.p));
      unsigned i;
      VALUE crary = rb_ary_new();

      for (i = 0; i < a->len; i++) {
	rb_ary_push(crary, mruby_cvt_mr2cr(a->buf[i]));
      }
      return crary;
    }
    break;

  case MRB_TT_HASH:
    break;

  case MRB_TT_STRING:
    {
      struct mrRString *s = ((struct mrRString *)((obj).value.p));
      return rb_str_new(s->buf, s->len);
    }
    break;

  case MRB_TT_REGEX:
  case MRB_TT_STRUCT:
  case MRB_TT_EXCEPTION:
    break;
  }

  return Qnil;
}

VALUE mruby_eval(VALUE self, VALUE prog) {
  int n = -1;
  mrb_state *mrb = mrb_open();
  struct mrb_parser_state *p;
  mrb_value rc;

  p = mrb_parse_string(mrb, StringValueCStr(prog));
  n = mrb_generate_code(mrb, p->tree);
  mrb_pool_close(p->pool);
  if (n >= 0) {
    rc = mrb_run(mrb, mrb_proc_new(mrb, mrb->irep[n]), mrb_nil_value());
  }

  return mruby_cvt_mr2cr(rc);
}
