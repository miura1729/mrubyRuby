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

static VALUE
mruby_alloc(VALUE class)
{
  mrb_state *mrb = mrb_open();
  return Data_Wrap_Struct(mruby_cMRuby, NULL, NULL, mrb);
}

VALUE
mruby_gen_code(VALUE self, VALUE prog)
{
  int n = -1;
  mrb_state *mrb = (mrb_state *)DATA_PTR(self);
  struct mrb_parser_state *p;

  p = mrb_parse_string(mrb, StringValueCStr(prog));
  n = mrb_generate_code(mrb, p->tree);
  mrb_pool_close(p->pool);

  return self;
}

static const char *op_names[] = {
"NOP",/*								*/
"MOVE",/*	A B	R(A) := R(B)					*/
"LOADL",/*	A Bx	R(A) := Lit(Bx)					*/
"LOADI",/*	A sBx	R(A) := sBx					*/
"LOADSYM",/*	A Bx	R(A) := Sym(Bx)					*/
"LOADNIL",/*	A	R(A) := nil					*/
"LOADSELF",/*	A	R(A) := self					*/
"LOADT",/*	A	R(A) := true					*/
"LOADF",/*	A	R(A) := false					*/

"GETGLOBAL",/*	A Bx	R(A) := getglobal(Sym(Bx))			*/
"SETGLOBAL",/*	A Bx	setglobal(Sym(Bx), R(A))			*/
"GETSPECIAL",/*A Bx	R(A) := Special[Bx]				*/
"SETSPECIAL",/*A Bx	Special[Bx] := R(A)				*/
"GETIV",/*	A Bx	R(A) := ivget(Sym(Bx))				*/
"SETIV",/*	A Bx	ivset(Sym(Bx),R(A))				*/
"GETCV",/*	A Bx	R(A) := cvget(Sym(Bx))				*/
"SETCV",/*	A Bx	cvset(Sym(Bx),R(A))				*/
"GETCONST",/*	A Bx	R(A) := constget(Sym(Bx))			*/
"SETCONST",/*	A Bx	constset(Sym(Bx),R(A))				*/
"GETMCNST",/*	A Bx	R(A) := R(A)::Sym(B)				*/
"SETMCNST",/*	A Bx	R(A+1)::Sym(B) := R(A)				*/
"GETUPVAR",/*	A B C	R(A) := uvget(B,C)				*/
"SETUPVAR",/*	A B C	uvset(B,C,R(A))					*/

"JMP",/*	sBx	pc+=sBx						*/
"JMPIF",/*	A sBx	if R(A) pc+=sBx					*/
"JMPNOT",/*	A sBx	if !R(A) pc+=sBx				*/
"ONERR",/*	sBx	rescue_push(pc+sBx)				*/
"RESCUE",/*	A	clear(exc); R(A) := exception (ignore when A=0)	*/
"POPERR",/*	A	A.times{rescue_pop()}				*/
"RAISE",/*	A	raise(R(A))					*/
"EPUSH",/*	Bx	ensure_push(SEQ[Bx])				*/
"EPOP",/*	A	A.times{ensure_pop().call}			*/

"SEND",/*	A B C	R(A) := call(R(A),mSym(B),R(A+1),...",R(A+C))	*/
"FSEND",/*	A B C	R(A) := fcall(R(A),mSym(B),R(A+1),...",R(A+C-1)) */
"VSEND",/*	A B	R(A) := vcall(R(A)",mSym(B))			*/
"CALL",/*	A B C	R(A) := self.call(R(A),..", R(A+C))		*/
"SUPER",/*	A B C	R(A) := super(R(A+1),... ",R(A+C-1))		*/
"ARGARY",/*	A Bx	R(A) := argument array (16=6:1:5:4)		*/
"ENTER",/*	Ax	arg setup according to flags (24=5:5:1:5:5:1:1)	*/
"KARG",/*	A B C	R(A) := kdict[mSym(B)]; if C kdict.rm(mSym(B))	*/
"KDICT",/*	A C	R(A) := kdict					*/

"RETURN",/*	A B	return R(A) (B=normal",in-block return/break)	*/
"TAILCALL",/*	A B C	return call(R(A),mSym(B)",*R(C))			*/
"BLKPUSH",/*	A Bx	R(A) := block (16=6:1:5:4)			*/

"ADD",/*	A B C	R(A) := R(A)+R(A+1) (mSyms[B]=:+,C=1)		*/
"ADDI",/*	A B C	R(A) := R(A)+C (mSyms[B]=:+)			*/
"SUB",/*	A B C	R(A) := R(A)-R(A+1) (mSyms[B]=:-,C=1)		*/
"SUBI",/*	A B C	R(A) := R(A)-C (mSyms[B]=:-)			*/
"MUL",/*	A B C	R(A) := R(A)*R(A+1) (mSyms[B]=:*,C=1)		*/
"DIV",/*	A B C	R(A) := R(A)/R(A+1) (mSyms[B]=:/,C=1)		*/
"EQ",/*	A B C	R(A) := R(A)==R(A+1) (mSyms[B]=:==,C=1)		*/
"LT",/*	A B C	R(A) := R(A)<R(A+1)  (mSyms[B]=:<,C=1)		*/
"LE",/*	A B C	R(A) := R(A)<=R(A+1) (mSyms[B]=:<=,C=1)		*/
"GT",/*	A B C	R(A) := R(A)>R(A+1)  (mSyms[B]=:>,C=1)		*/
"GE",/*	A B C	R(A) := R(A)>=R(A+1) (mSyms[B]=:>=,C=1)		*/

"ARRAY",/*	A B C	R(A) := ary_new(R(B)",R(B+1)..R(B+C))		*/
"ARYCAT",/*	A B	ary_cat(R(A),R(B))				*/
"ARYPUSH",/*	A B	ary_push(R(A),R(B))				*/
"AREF",/*	A B C	R(A) := R(B)[C]					*/
"ASET",/*	A B C	R(B)[C] := R(A)					*/
"APOST",/*	A B C	*R(A),R(A+1)..R(A+C) := R(A)			*/

"STRING",/*	A Bx	R(A) := str_dup(Lit(Bx))			*/
"STRCAT",/*	A B	str_cat(R(A),R(B))				*/

"HASH",/*	A B C	R(A) := hash_new(R(B),R(B+1)..R(B+C))		*/
"LAMBDA",/*	A Bz Cz	R(A) := lambda(SEQ[Bz],Cm)			*/
"RANGE",/*	A B C	R(A) := range_new(R(B),R(B+1),C)		*/

"OCLASS",/*	A	R(A) := ::Object				*/
"CLASS",/*	A B	R(A) := newclass(R(A),mSym(B),R(A+1))		*/
"MODULE",/*	A B	R(A) := newmodule(R(A),mSym(B))			*/
"EXEC",/*	A Bx	R(A) := blockexec(R(A),SEQ[Bx])			*/
"METHOD",/*	A B	R(A).newmethod(mSym(B),R(A+1))			*/
"SCLASS",/*	A B	R(A) := R(B).singleton_class			*/
"TCLASS",/*	A	R(A) := target_class				*/

"DEBUG",/*	A	print R(A)					*/
"STOP",/*		stop VM						*/
"ERR",/*	Bx	raise RuntimeError with message Lit(Bx)		*/

"RSVD1",/*		reserved instruction #1				*/
"RSVD2",/*		reserved instruction #2				*/
"RSVD3",/*		reserved instruction #3				*/
"RSVD4",/*		reserved instruction #4				*/
"RSVD5",/*		reserved instruction #5				*/
};

#undef rb_intern

VALUE
mruby_to_a(VALUE self)
{
  unsigned i;
  int j;
  mrb_state *mrb = (mrb_state *)DATA_PTR(self);
  VALUE resarr;
  VALUE a_proc;
  VALUE a_inst;

  resarr = rb_ary_new();
  for (i = 0; i < mrb->irep_len; i++) {
    mrb_irep *irep = mrb->irep[i];
    a_proc = rb_ary_new();
    for (j = 0; j < irep->ilen; j++) {
      a_inst = rb_ary_new();
      rb_ary_push(a_inst, ID2SYM(rb_intern(op_names[irep->iseq[j] & 0x7f])));
      rb_ary_push(a_proc, a_inst);
    }
    rb_ary_push(resarr, a_proc);
  }

  return resarr;
}

void Init_mruby() {
  mruby_cMRuby = rb_define_class("MRuby", rb_cObject);
  rb_define_singleton_method(mruby_cMRuby, "eval", mruby_eval, 1);
  rb_define_alloc_func(mruby_cMRuby, mruby_alloc);
  rb_define_method(mruby_cMRuby, "gen_code", mruby_gen_code, 1);
  rb_define_method(mruby_cMRuby, "to_a", mruby_to_a, 0);
}


