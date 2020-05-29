/*
 * Copyright (c) 1999, 2006, Oracle and/or its affiliates. All rights reserved.
 * ORACLE PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 */

#include "incls/_precompiled.incl"
#include "incls/_c1_Canonicalizer.cpp.incl"


static void do_print_value(Value* vp) {
  (*vp)->print_line();
}

void Canonicalizer::set_canonical(Value x) {
  assert(x != NULL, "value must exist");
  // Note: we can not currently substitute root nodes which show up in
  // the instruction stream (because the instruction list is embedded
  // in the instructions).
  if (canonical() != x) {
    if (PrintCanonicalization) {
      canonical()->input_values_do(do_print_value);
      canonical()->print_line();
      tty->print_cr("canonicalized to:");
      x->input_values_do(do_print_value);
      x->print_line();
      tty->cr();
    }
    assert(_canonical->type()->tag() == x->type()->tag(), "types must match");
    _canonical = x;
  }
}


void Canonicalizer::move_const_to_right(Op2* x) {
  if (x->x()->type()->is_constant() && x->is_commutative()) x->swap_operands();
}


void Canonicalizer::do_Op2(Op2* x) {
  if (x->x() == x->y()) {
    switch (x->op()) {
    case Bytecodes::_isub: set_constant(0); return;
    case Bytecodes::_lsub: set_constant(jlong_cast(0)); return;
    case Bytecodes::_iand: // fall through
    case Bytecodes::_land: // fall through
    case Bytecodes::_ior:  // fall through
    case Bytecodes::_lor : set_canonical(x->x()); return;
    case Bytecodes::_ixor: set_constant(0); return;
    case Bytecodes::_lxor: set_constant(jlong_cast(0)); return;
    }
  }

  if (x->x()->type()->is_constant() && x->y()->type()->is_constant()) {
    // do constant folding for selected operations
    switch (x->type()->tag()) {
      case intTag:
        { jint a = x->x()->type()->as_IntConstant()->value();
          jint b = x->y()->type()->as_IntConstant()->value();
          switch (x->op()) {
            case Bytecodes::_iadd: set_constant(a + b); return;
            case Bytecodes::_isub: set_constant(a - b); return;
            case Bytecodes::_imul: set_constant(a * b); return;
            case Bytecodes::_idiv:
              if (b != 0) {
                if (a == min_jint && b == -1) {
                  set_constant(min_jint);
                } else {
                  set_constant(a / b);
                }
                return;
              }
              break;
            case Bytecodes::_irem:
              if (b != 0) {
                if (a == min_jint && b == -1) {
                  set_constant(0);
                } else {
                  set_constant(a % b);
                }
                return;
              }
              break;
            case Bytecodes::_iand: set_constant(a & b); return;
            case Bytecodes::_ior : set_constant(a | b); return;
            case Bytecodes::_ixor: set_constant(a ^ b); return;
          }
        }
        break;
      case longTag:
        { jlong a = x->x()->type()->as_LongConstant()->value();
          jlong b = x->y()->type()->as_LongConstant()->value();
          switch (x->op()) {
            case Bytecodes::_ladd: set_constant(a + b); return;
            case Bytecodes::_lsub: set_constant(a - b); return;
            case Bytecodes::_lmul: set_constant(a * b); return;
            case Bytecodes::_ldiv:
              if (b != 0) {
                set_constant(SharedRuntime::ldiv(b, a));
                return;
              }
              break;
            case Bytecodes::_lrem:
              if (b != 0) {
                set_constant(SharedRuntime::lrem(b, a));
                return;
              }
              break;
            case Bytecodes::_land: set_constant(a & b); return;
            case Bytecodes::_lor : set_constant(a | b); return;
            case Bytecodes::_lxor: set_constant(a ^ b); return;
          }
        }
        break;
      // other cases not implemented (must be extremely careful with floats & doubles!)
    }
  }
  // make sure constant is on the right side, if any
  move_const_to_right(x);

  if (x->y()->type()->is_constant()) {
    // do constant folding for selected operations
    switch (x->type()->tag()) {
      case intTag:
        if (x->y()->type()->as_IntConstant()->value() == 0) {
          switch (x->op()) {
            case Bytecodes::_iadd: set_canonical(x->x()); return;
            case Bytecodes::_isub: set_canonical(x->x()); return;
            case Bytecodes::_imul: set_constant(0); return;
              // Note: for div and rem, make sure that C semantics
              //       corresponds to Java semantics!
            case Bytecodes::_iand: set_constant(0); return;
            case Bytecodes::_ior : set_canonical(x->x()); return;
          }
        }
        break;
      case longTag:
        if (x->y()->type()->as_LongConstant()->value() == (jlong)0) {
          switch (x->op()) {
            case Bytecodes::_ladd: set_canonical(x->x()); return;
            case Bytecodes::_lsub: set_canonical(x->x()); return;
            case Bytecodes::_lmul: set_constant((jlong)0); return;
              // Note: for div and rem, make sure that C semantics
              //       corresponds to Java semantics!
            case Bytecodes::_land: set_constant((jlong)0); return;
            case Bytecodes::_lor : set_canonical(x->x()); return;
          }
        }
        break;
    }
  }
}


void Canonicalizer::do_Phi            (Phi*             x) {}
void Canonicalizer::do_Constant       (Constant*        x) {}
void Canonicalizer::do_Local          (Local*           x) {}
void Canonicalizer::do_LoadField      (LoadField*       x) {}

// checks if v is in the block that is currently processed by
// GraphBuilder. This is the only block that has not BlockEnd yet.
static bool in_current_block(Value v) {
  int max_distance = 4;
  while (max_distance > 0 && v != NULL && v->as_BlockEnd() == NULL) {
    v = v->next();
    max_distance--;
  }
  return v == NULL;
}

void Canonicalizer::do_StoreField     (StoreField*      x) {
  // If a value is going to be stored into a field or array some of
  // the conversions emitted by javac are unneeded because the fields
  // are packed to their natural size.
  Convert* conv = x->value()->as_Convert();
  if (conv) {
    Value value = NULL;
    BasicType type = x->field()->type()->basic_type();
    switch (conv->op()) {
    case Bytecodes::_i2b: if (type == T_BYTE)  value = conv->value(); break;
    case Bytecodes::_i2s: if (type == T_SHORT || type == T_BYTE) value = conv->value(); break;
    case Bytecodes::_i2c: if (type == T_CHAR  || type == T_BYTE)  value = conv->value(); break;
    }
    // limit this optimization to current block
    if (value != NULL && in_current_block(conv)) {
      set_canonical(new StoreField(x->obj(), x->offset(), x->field(), value, x->is_static(),
                                   x->lock_stack(), x->state_before(), x->is_loaded(), x->is_initialized()));
      return;
    }
  }

}

void Canonicalizer::do_ArrayLength    (ArrayLength*     x) {
  NewArray* array = x->array()->as_NewArray();
  if (array != NULL && array->length() != NULL) {
    Constant* length = array->length()->as_Constant();
    if (length != NULL) {
      // do not use the Constant itself, but create a new Constant
      // with same value Otherwise a Constant is live over multiple
      // blocks without being registered in a state array.
      assert(length->type()->as_IntConstant() != NULL, "array length must be integer");
      set_constant(length->type()->as_IntConstant()->value());
    }
  } else {
    LoadField* lf = x->array()->as_LoadField();
    if (lf != NULL && lf->field()->is_constant()) {
      ciObject* c = lf->field()->constant_value().as_object();
      if (c->is_array()) {
        ciArray* array = (ciArray*) c;
        set_constant(array->length());
      }
    }
  }
}

void Canonicalizer::do_LoadIndexed    (LoadIndexed*     x) {}
void Canonicalizer::do_StoreIndexed   (StoreIndexed*    x) {
  // If a value is going to be stored into a field or array some of
  // the conversions emitted by javac are unneeded because the fields
  // are packed to their natural size.
  Convert* conv = x->value()->as_Convert();
  if (conv) {
    Value value = NULL;
    BasicType type = x->elt_type();
    switch (conv->op()) {
    case Bytecodes::_i2b: if (type == T_BYTE)  value = conv->value(); break;
    case Bytecodes::_i2s: if (type == T_SHORT || type == T_BYTE) value = conv->value(); break;
    case Bytecodes::_i2c: if (type == T_CHAR  || type == T_BYTE) value = conv->value(); break;
    }
    // limit this optimization to current block
    if (value != NULL && in_current_block(conv)) {
      set_canonical(new StoreIndexed(x->array(), x->index(), x->length(),
                                     x->elt_type(), value, x->lock_stack()));
      return;
    }
  }


}


void Canonicalizer::do_NegateOp(NegateOp* x) {
  ValueType* t = x->x()->type();
  if (t->is_constant()) {
    switch (t->tag()) {
      case intTag   : set_constant(-t->as_IntConstant   ()->value()); return;
      case longTag  : set_constant(-t->as_LongConstant  ()->value()); return;
      case floatTag : set_constant(-t->as_FloatConstant ()->value()); return;
      case doubleTag: set_constant(-t->as_DoubleConstant()->value()); return;
      default       : ShouldNotReachHere();
    }
  }
}


void Canonicalizer::do_ArithmeticOp   (ArithmeticOp*    x) { do_Op2(x); }


void Canonicalizer::do_ShiftOp        (ShiftOp*         x) {
  ValueType* t = x->x()->type();
  ValueType* t2 = x->y()->type();
  if (t->is_constant()) {
    switch (t->tag()) {
    case intTag   : if (t->as_IntConstant()->value() == 0)         { set_constant(0); return; } break;
    case longTag  : if (t->as_LongConstant()->value() == (jlong)0) { set_constant(jlong_cast(0)); return; } break;
    default       : ShouldNotReachHere();
    }
    if (t2->is_constant()) {
      if (t->tag() == intTag) {
        int value = t->as_IntConstant()->value();
        int shift = t2->as_IntConstant()->value() & 31;
        jint mask = ~(~0 << (32 - shift));
        if (shift == 0) mask = ~0;
        switch (x->op()) {
          case Bytecodes::_ishl:  set_constant(value << shift); return;
          case Bytecodes::_ishr:  set_constant(value >> shift); return;
          case Bytecodes::_iushr: set_constant((value >> shift) & mask); return;
        }
      } else if (t->tag() == longTag) {
        jlong value = t->as_LongConstant()->value();
        int shift = t2->as_IntConstant()->value() & 63;
        jlong mask = ~(~jlong_cast(0) << (64 - shift));
        if (shift == 0) mask = ~jlong_cast(0);
        switch (x->op()) {
          case Bytecodes::_lshl:  set_constant(value << shift); return;
          case Bytecodes::_lshr:  set_constant(value >> shift); return;
          case Bytecodes::_lushr: set_constant((value >> shift) & mask); return;
        }
      }
    }
  }
  if (t2->is_constant()) {
    switch (t2->tag()) {
      case intTag   : if (t2->as_IntConstant()->value() == 0)  set_canonical(x->x()); return;
      case longTag  : if (t2->as_IntConstant()->value() == 0)  set_canonical(x->x()); return;
      default       : ShouldNotReachHere();
    }
  }
}


void Canonicalizer::do_LogicOp        (LogicOp*         x) { do_Op2(x); }
void Canonicalizer::do_CompareOp      (CompareOp*       x) {
  if (x->x() == x->y()) {
    switch (x->x()->type()->tag()) {
      case longTag: set_constant(0); break;
      case floatTag: {
        FloatConstant* fc = x->x()->type()->as_FloatConstant();
        if (fc) {
          if (g_isnan(fc->value())) {
            set_constant(x->op() == Bytecodes::_fcmpl ? -1 : 1);
          } else {
            set_constant(0);
          }
        }
        break;
      }
      case doubleTag: {
        DoubleConstant* dc = x->x()->type()->as_DoubleConstant();
        if (dc) {
          if (g_isnan(dc->value())) {
            set_constant(x->op() == Bytecodes::_dcmpl ? -1 : 1);
          } else {
            set_constant(0);
          }
        }
        break;
      }
    }
  } else if (x->x()->type()->is_constant() && x->y()->type()->is_constant()) {
    switch (x->x()->type()->tag()) {
      case longTag: {
        jlong vx = x->x()->type()->as_LongConstant()->value();
        jlong vy = x->y()->type()->as_LongConstant()->value();
        if (vx == vy)
          set_constant(0);
        else if (vx < vy)
          set_constant(-1);
        else
          set_constant(1);
        break;
      }

      case floatTag: {
        float vx = x->x()->type()->as_FloatConstant()->value();
        float vy = x->y()->type()->as_FloatConstant()->value();
        if (g_isnan(vx) || g_isnan(vy))
          set_constant(x->op() == Bytecodes::_fcmpl ? -1 : 1);
        else if (vx == vy)
          set_constant(0);
        else if (vx < vy)
          set_constant(-1);
        else
          set_constant(1);
        break;
      }

      case doubleTag: {
        double vx = x->x()->type()->as_DoubleConstant()->value();
        double vy = x->y()->type()->as_DoubleConstant()->value();
        if (g_isnan(vx) || g_isnan(vy))
          set_constant(x->op() == Bytecodes::_dcmpl ? -1 : 1);
        else if (vx == vy)
          set_constant(0);
        else if (vx < vy)
          set_constant(-1);
        else
          set_constant(1);
        break;
      }
    }

  }
}


void Canonicalizer::do_IfInstanceOf(IfInstanceOf*    x) {}

void Canonicalizer::do_IfOp(IfOp* x) {
  // Caution: do not use do_Op2(x) here for now since
  //          we map the condition to the op for now!
  move_const_to_right(x);
}


void Canonicalizer::do_Intrinsic      (Intrinsic*       x) {
  switch (x->id()) {
  case vmIntrinsics::_floatToRawIntBits   : {
    FloatConstant* c = x->argument_at(0)->type()->as_FloatConstant();
    if (c != NULL) {
      JavaValue v;
      v.set_jfloat(c->value());
      set_constant(v.get_jint());
    }
    break;
  }
  case vmIntrinsics::_intBitsToFloat      : {
    IntConstant* c = x->argument_at(0)->type()->as_IntConstant();
    if (c != NULL) {
      JavaValue v;
      v.set_jint(c->value());
      set_constant(v.get_jfloat());
    }
    break;
  }
  case vmIntrinsics::_doubleToRawLongBits : {
    DoubleConstant* c = x->argument_at(0)->type()->as_DoubleConstant();
    if (c != NULL) {
      JavaValue v;
      v.set_jdouble(c->value());
      set_constant(v.get_jlong());
    }
    break;
  }
  case vmIntrinsics::_longBitsToDouble    : {
    LongConstant* c = x->argument_at(0)->type()->as_LongConstant();
    if (c != NULL) {
      JavaValue v;
      v.set_jlong(c->value());
      set_constant(v.get_jdouble());
    }
    break;
  }
  }
}

void Canonicalizer::do_Convert        (Convert*         x) {
  if (x->value()->type()->is_constant()) {
    switch (x->op()) {
    case Bytecodes::_i2b:  set_constant((int)((x->value()->type()->as_IntConstant()->value() << 24) >> 24)); break;
    case Bytecodes::_i2s:  set_constant((int)((x->value()->type()->as_IntConstant()->value() << 16) >> 16)); break;
    case Bytecodes::_i2c:  set_constant((int)(x->value()->type()->as_IntConstant()->value() & ((1<<16)-1))); break;
    case Bytecodes::_i2l:  set_constant((jlong)(x->value()->type()->as_IntConstant()->value()));             break;
    case Bytecodes::_i2f:  set_constant((float)(x->value()->type()->as_IntConstant()->value()));             break;
    case Bytecodes::_i2d:  set_constant((double)(x->value()->type()->as_IntConstant()->value()));            break;
    case Bytecodes::_l2i:  set_constant((int)(x->value()->type()->as_LongConstant()->value()));              break;
    case Bytecodes::_l2f:  set_constant(SharedRuntime::l2f(x->value()->type()->as_LongConstant()->value())); break;
    case Bytecodes::_l2d:  set_constant(SharedRuntime::l2d(x->value()->type()->as_LongConstant()->value())); break;
    case Bytecodes::_f2d:  set_constant((double)(x->value()->type()->as_FloatConstant()->value()));          break;
    case Bytecodes::_f2i:  set_constant(SharedRuntime::f2i(x->value()->type()->as_FloatConstant()->value())); break;
    case Bytecodes::_f2l:  set_constant(SharedRuntime::f2l(x->value()->type()->as_FloatConstant()->value())); break;
    case Bytecodes::_d2f:  set_constant((float)(x->value()->type()->as_DoubleConstant()->value()));          break;
    case Bytecodes::_d2i:  set_constant(SharedRuntime::d2i(x->value()->type()->as_DoubleConstant()->value())); break;
    case Bytecodes::_d2l:  set_constant(SharedRuntime::d2l(x->value()->type()->as_DoubleConstant()->value())); break;
    default:
      ShouldNotReachHere();
    }
  }

  Value value = x->value();
  BasicType type = T_ILLEGAL;
  LoadField* lf = value->as_LoadField();
  if (lf) {
    type = lf->field_type();
  } else {
    LoadIndexed* li = value->as_LoadIndexed();
    if (li) {
      type = li->elt_type();
    } else {
      Convert* conv = value->as_Convert();
      if (conv) {
        switch (conv->op()) {
          case Bytecodes::_i2b: type = T_BYTE;  break;
          case Bytecodes::_i2s: type = T_SHORT; break;
          case Bytecodes::_i2c: type = T_CHAR;  break;
        }
      }
    }
  }
  if (type != T_ILLEGAL) {
    switch (x->op()) {
      case Bytecodes::_i2b: if (type == T_BYTE)                    set_canonical(x->value()); break;
      case Bytecodes::_i2s: if (type == T_SHORT || type == T_BYTE) set_canonical(x->value()); break;
      case Bytecodes::_i2c: if (type == T_CHAR)                    set_canonical(x->value()); break;
    }
  } else {
    Op2* op2 = x->value()->as_Op2();
    if (op2 && op2->op() == Bytecodes::_iand && op2->y()->type()->is_constant()) {
      jint safebits = 0;
      jint mask = op2->y()->type()->as_IntConstant()->value();
      switch (x->op()) {
        case Bytecodes::_i2b: safebits = 0x7f;   break;
        case Bytecodes::_i2s: safebits = 0x7fff; break;
        case Bytecodes::_i2c: safebits = 0xffff; break;
      }
      // When casting a masked integer to a smaller signed type, if
      // the mask doesn't include the sign bit the cast isn't needed.
      if (safebits && (mask & ~safebits) == 0) {
        set_canonical(x->value());
      }
    }
  }

}

void Canonicalizer::do_NullCheck      (NullCheck*       x) {
  if (x->obj()->as_NewArray() != NULL || x->obj()->as_NewInstance() != NULL) {
    set_canonical(x->obj());
  } else {
    Constant* con = x->obj()->as_Constant();
    if (con) {
      ObjectType* c = con->type()->as_ObjectType();
      if (c && c->is_loaded()) {
        ObjectConstant* oc = c->as_ObjectConstant();
        if (!oc || !oc->value()->is_null_object()) {
          set_canonical(con);
        }
      }
    }
  }
}

void Canonicalizer::do_Invoke         (Invoke*          x) {}
void Canonicalizer::do_NewInstance    (NewInstance*     x) {}
void Canonicalizer::do_NewTypeArray   (NewTypeArray*    x) {}
void Canonicalizer::do_NewObjectArray (NewObjectArray*  x) {}
void Canonicalizer::do_NewMultiArray  (NewMultiArray*   x) {}
void Canonicalizer::do_CheckCast      (CheckCast*       x) {
  if (x->klass()->is_loaded()) {
    Value obj = x->obj();
    ciType* klass = obj->exact_type();
    if (klass == NULL) klass = obj->declared_type();
    if (klass != NULL && klass->is_loaded() && klass->is_subtype_of(x->klass())) {
      set_canonical(obj);
      return;
    }
    // checkcast of null returns null
    if (obj->as_Constant() && obj->type()->as_ObjectType()->constant_value()->is_null_object()) {
      set_canonical(obj);
    }
  }
}
void Canonicalizer::do_InstanceOf     (InstanceOf*      x) {
  if (x->klass()->is_loaded()) {
    Value obj = x->obj();
    ciType* exact = obj->exact_type();
    if (exact != NULL && exact->is_loaded() && (obj->as_NewInstance() || obj->as_NewArray())) {
      set_constant(exact->is_subtype_of(x->klass()) ? 1 : 0);
      return;
    }
    // instanceof null returns false
    if (obj->as_Constant() && obj->type()->as_ObjectType()->constant_value()->is_null_object()) {
      set_constant(0);
    }
  }

}
void Canonicalizer::do_MonitorEnter   (MonitorEnter*    x) {}
void Canonicalizer::do_MonitorExit    (MonitorExit*     x) {}
void Canonicalizer::do_BlockBegin     (BlockBegin*      x) {}
void Canonicalizer::do_Goto           (Goto*            x) {}


static bool is_true(jlong x, If::Condition cond, jlong y) {
  switch (cond) {
    case If::eql: return x == y;
    case If::neq: return x != y;
    case If::lss: return x <  y;
    case If::leq: return x <= y;
    case If::gtr: return x >  y;
    case If::geq: return x >= y;
  }
  ShouldNotReachHere();
  return false;
}


void Canonicalizer::do_If(If* x) {
  // move const to right
  if (x->x()->type()->is_constant()) x->swap_operands();
  // simplify
  const Value l = x->x(); ValueType* lt = l->type();
  const Value r = x->y(); ValueType* rt = r->type();

  if (l == r && !lt->is_float_kind()) {
    // pattern: If (a cond a) => simplify to Goto
    BlockBegin* sux;
    switch (x->cond()) {
    case If::eql: sux = x->sux_for(true);  break;
    case If::neq: sux = x->sux_for(false); break;
    case If::lss: sux = x->sux_for(false); break;
    case If::leq: sux = x->sux_for(true);  break;
    case If::gtr: sux = x->sux_for(false); break;
    case If::geq: sux = x->sux_for(true);  break;
    }
    // If is a safepoint then the debug information should come from the state_before of the If.
    set_canonical(new Goto(sux, x->state_before(), x->is_safepoint()));
    return;
  }

  if (lt->is_constant() && rt->is_constant()) {
    if (x->x()->as_Constant() != NULL) {
      // pattern: If (lc cond rc) => simplify to: Goto
      BlockBegin* sux = x->x()->as_Constant()->compare(x->cond(), x->y(),
                                                       x->sux_for(true),
                                                       x->sux_for(false));
      if (sux != NULL) {
        // If is a safepoint then the debug information should come from the state_before of the If.
        set_canonical(new Goto(sux, x->state_before(), x->is_safepoint()));
      }
    }
  } else if (rt->as_IntConstant() != NULL) {
    // pattern: If (l cond rc) => investigate further
    const jint rc = rt->as_IntConstant()->value();
    if (l->as_CompareOp() != NULL) {
      // pattern: If ((a cmp b) cond rc) => simplify to: If (x cond y) or: Goto
      CompareOp* cmp = l->as_CompareOp();
      bool unordered_is_less = cmp->op() == Bytecodes::_fcmpl || cmp->op() == Bytecodes::_dcmpl;
      BlockBegin* lss_sux = x->sux_for(is_true(-1, x->cond(), rc)); // successor for a < b
      BlockBegin* eql_sux = x->sux_for(is_true( 0, x->cond(), rc)); // successor for a = b
      BlockBegin* gtr_sux = x->sux_for(is_true(+1, x->cond(), rc)); // successor for a > b
      BlockBegin* nan_sux = unordered_is_less ? lss_sux : gtr_sux ; // successor for unordered
      // Note: At this point all successors (lss_sux, eql_sux, gtr_sux, nan_sux) are
      //       equal to x->tsux() or x->fsux(). Furthermore, nan_sux equals either
      //       lss_sux or gtr_sux.
      if (lss_sux == eql_sux && eql_sux == gtr_sux) {
        // all successors identical => simplify to: Goto
        set_canonical(new Goto(lss_sux, x->state_before(), x->is_safepoint()));
      } else {
        // two successors differ and two successors are the same => simplify to: If (x cmp y)
        // determine new condition & successors
        If::Condition cond;
        BlockBegin* tsux = NULL;
        BlockBegin* fsux = NULL;
             if (lss_sux == eql_sux) { cond = If::leq; tsux = lss_sux; fsux = gtr_sux; }
        else if (lss_sux == gtr_sux) { cond = If::neq; tsux = lss_sux; fsux = eql_sux; }
        else if (eql_sux == gtr_sux) { cond = If::geq; tsux = eql_sux; fsux = lss_sux; }
        else                         { ShouldNotReachHere();                           }
           If* canon = new If(cmp->x(), cond, nan_sux == tsux, cmp->y(), tsux, fsux, cmp->state_before(), x->is_safepoint());
        if (cmp->x() == cmp->y()) {
          do_If(canon);
        } else {
          set_canonical(canon);
          set_bci(cmp->bci());
        }
      }
    } else if (l->as_InstanceOf() != NULL) {
      // NOTE: Code permanently disabled for now since it leaves the old InstanceOf
      //       instruction in the graph (it is pinned). Need to fix this at some point.
      return;
      // pattern: If ((obj instanceof klass) cond rc) => simplify to: IfInstanceOf or: Goto
      InstanceOf* inst = l->as_InstanceOf();
      BlockBegin* is_inst_sux = x->sux_for(is_true(1, x->cond(), rc)); // successor for instanceof == 1
      BlockBegin* no_inst_sux = x->sux_for(is_true(0, x->cond(), rc)); // successor for instanceof == 0
      if (is_inst_sux == no_inst_sux && inst->is_loaded()) {
        // both successors identical and klass is loaded => simplify to: Goto
        set_canonical(new Goto(is_inst_sux, x->state_before(), x->is_safepoint()));
      } else {
        // successors differ => simplify to: IfInstanceOf
        set_canonical(new IfInstanceOf(inst->klass(), inst->obj(), true, inst->bci(), is_inst_sux, no_inst_sux));
      }
    }
  } else if (rt == objectNull && (l->as_NewInstance() || l->as_NewArray())) {
    if (x->cond() == Instruction::eql) {
      set_canonical(new Goto(x->fsux(), x->state_before(), x->is_safepoint()));
    } else {
      assert(x->cond() == Instruction::neq, "only other valid case");
      set_canonical(new Goto(x->tsux(), x->state_before(), x->is_safepoint()));
    }
  }
}


void Canonicalizer::do_TableSwitch(TableSwitch* x) {
  if (x->tag()->type()->is_constant()) {
    int v = x->tag()->type()->as_IntConstant()->value();
    BlockBegin* sux = x->default_sux();
    if (v >= x->lo_key() && v <= x->hi_key()) {
      sux = x->sux_at(v - x->lo_key());
    }
    set_canonical(new Goto(sux, x->state_before(), x->is_safepoint()));
  } else if (x->number_of_sux() == 1) {
    // NOTE: Code permanently disabled for now since the switch statement's
    //       tag expression may produce side-effects in which case it must
    //       be executed.
    return;
    // simplify to Goto
    set_canonical(new Goto(x->default_sux(), x->state_before(), x->is_safepoint()));
  } else if (x->number_of_sux() == 2) {
    // NOTE: Code permanently disabled for now since it produces two new nodes
    //       (Constant & If) and the Canonicalizer cannot return them correctly
    //       yet. For now we copied the corresponding code directly into the
    //       GraphBuilder (i.e., we should never reach here).
    return;
    // simplify to If
    assert(x->lo_key() == x->hi_key(), "keys must be the same");
    Constant* key = new Constant(new IntConstant(x->lo_key()));
    set_canonical(new If(x->tag(), If::eql, true, key, x->sux_at(0), x->default_sux(), x->state_before(), x->is_safepoint()));
  }
}


void Canonicalizer::do_LookupSwitch(LookupSwitch* x) {
  if (x->tag()->type()->is_constant()) {
    int v = x->tag()->type()->as_IntConstant()->value();
    BlockBegin* sux = x->default_sux();
    for (int i = 0; i < x->length(); i++) {
      if (v == x->key_at(i)) {
        sux = x->sux_at(i);
      }
    }
    set_canonical(new Goto(sux, x->state_before(), x->is_safepoint()));
  } else if (x->number_of_sux() == 1) {
    // NOTE: Code permanently disabled for now since the switch statement's
    //       tag expression may produce side-effects in which case it must
    //       be executed.
    return;
    // simplify to Goto
    set_canonical(new Goto(x->default_sux(), x->state_before(), x->is_safepoint()));
  } else if (x->number_of_sux() == 2) {
    // NOTE: Code permanently disabled for now since it produces two new nodes
    //       (Constant & If) and the Canonicalizer cannot return them correctly
    //       yet. For now we copied the corresponding code directly into the
    //       GraphBuilder (i.e., we should never reach here).
    return;
    // simplify to If
    assert(x->length() == 1, "length must be the same");
    Constant* key = new Constant(new IntConstant(x->key_at(0)));
    set_canonical(new If(x->tag(), If::eql, true, key, x->sux_at(0), x->default_sux(), x->state_before(), x->is_safepoint()));
  }
}


void Canonicalizer::do_Return         (Return*          x) {}
void Canonicalizer::do_Throw          (Throw*           x) {}
void Canonicalizer::do_Base           (Base*            x) {}
void Canonicalizer::do_OsrEntry       (OsrEntry*        x) {}
void Canonicalizer::do_ExceptionObject(ExceptionObject* x) {}

static bool match_index_and_scale(Instruction*  instr,
                                  Instruction** index,
                                  int*          log2_scale,
                                  Instruction** instr_to_unpin) {
  *instr_to_unpin = NULL;

  // Skip conversion ops
  Convert* convert = instr->as_Convert();
  if (convert != NULL) {
    instr = convert->value();
  }

  ShiftOp* shift = instr->as_ShiftOp();
  if (shift != NULL) {
    if (shift->is_pinned()) {
      *instr_to_unpin = shift;
    }
    // Constant shift value?
    Constant* con = shift->y()->as_Constant();
    if (con == NULL) return false;
    // Well-known type and value?
    IntConstant* val = con->type()->as_IntConstant();
    if (val == NULL) return false;
    if (shift->x()->type() != intType) return false;
    *index = shift->x();
    int tmp_scale = val->value();
    if (tmp_scale >= 0 && tmp_scale < 4) {
      *log2_scale = tmp_scale;
      return true;
    } else {
      return false;
    }
  }

  ArithmeticOp* arith = instr->as_ArithmeticOp();
  if (arith != NULL) {
    if (arith->is_pinned()) {
      *instr_to_unpin = arith;
    }
    // Check for integer multiply
    if (arith->op() == Bytecodes::_imul) {
      // See if either arg is a known constant
      Constant* con = arith->x()->as_Constant();
      if (con != NULL) {
        *index = arith->y();
      } else {
        con = arith->y()->as_Constant();
        if (con == NULL) return false;
        *index = arith->x();
      }
      if ((*index)->type() != intType) return false;
      // Well-known type and value?
      IntConstant* val = con->type()->as_IntConstant();
      if (val == NULL) return false;
      switch (val->value()) {
      case 1: *log2_scale = 0; return true;
      case 2: *log2_scale = 1; return true;
      case 4: *log2_scale = 2; return true;
      case 8: *log2_scale = 3; return true;
      default:            return false;
      }
    }
  }

  // Unknown instruction sequence; don't touch it
  return false;
}


static bool match(UnsafeRawOp* x,
                  Instruction** base,
                  Instruction** index,
                  int*          log2_scale) {
  Instruction* instr_to_unpin = NULL;
  ArithmeticOp* root = x->base()->as_ArithmeticOp();
  if (root == NULL) return false;
  // Limit ourselves to addition for now
  if (root->op() != Bytecodes::_ladd) return false;
  // Try to find shift or scale op
  if (match_index_and_scale(root->y(), index, log2_scale, &instr_to_unpin)) {
    *base = root->x();
  } else if (match_index_and_scale(root->x(), index, log2_scale, &instr_to_unpin)) {
    *base = root->y();
  } else if (root->y()->as_Convert() != NULL) {
    Convert* convert = root->y()->as_Convert();
    if (convert->op() == Bytecodes::_i2l && convert->value()->type() == intType) {
      // pick base and index, setting scale at 1
      *base  = root->x();
      *index = convert->value();
      *log2_scale = 0;
    } else {
      return false;
    }
  } else {
    // doesn't match any expected sequences
    return false;
  }

  // If the value is pinned then it will be always be computed so
  // there's no profit to reshaping the expression.
  return !root->is_pinned();
}


void Canonicalizer::do_UnsafeRawOp(UnsafeRawOp* x) {
  Instruction* base = NULL;
  Instruction* index = NULL;
  int          log2_scale;

  if (match(x, &base, &index, &log2_scale)) {
    x->set_base(base);
    x->set_index(index);
    x->set_log2_scale(log2_scale);
    if (PrintUnsafeOptimization) {
      tty->print_cr("Canonicalizer: UnsafeRawOp id %d: base = id %d, index = id %d, log2_scale = %d",
                    x->id(), x->base()->id(), x->index()->id(), x->log2_scale());
    }
  }
}

void Canonicalizer::do_RoundFP(RoundFP* x) {}
void Canonicalizer::do_UnsafeGetRaw(UnsafeGetRaw* x) { if (OptimizeUnsafes) do_UnsafeRawOp(x); }
void Canonicalizer::do_UnsafePutRaw(UnsafePutRaw* x) { if (OptimizeUnsafes) do_UnsafeRawOp(x); }
void Canonicalizer::do_UnsafeGetObject(UnsafeGetObject* x) {}
void Canonicalizer::do_UnsafePutObject(UnsafePutObject* x) {}
void Canonicalizer::do_UnsafePrefetchRead (UnsafePrefetchRead*  x) {}
void Canonicalizer::do_UnsafePrefetchWrite(UnsafePrefetchWrite* x) {}
void Canonicalizer::do_ProfileCall(ProfileCall* x) {}
void Canonicalizer::do_ProfileCounter(ProfileCounter* x) {}
