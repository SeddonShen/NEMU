#include <rtl/rtl.h>
#include <softfloat.h>
#include <specialize.h>
#include <internals.h>

#define BOX_MASK 0xFFFFFFFF00000000
#define F32_SIGN ((uint64_t)1ul << 31)
#define F64_SIGN ((uint64_t)1ul << 63)

static inline rtlreg_t unbox(rtlreg_t r) {
  if ((r & BOX_MASK) == BOX_MASK) return r & ~BOX_MASK;
  else return defaultNaNF32UI;
}

static inline float32_t rtlToF32(rtlreg_t r) {
  float32_t f = { .v = unbox(r) };
  return f;
}

static inline float64_t rtlToF64(rtlreg_t r) {
  float64_t f = { .v = r };
  return f;
}

static inline float32_t f32_min(float32_t a, float32_t b){
  bool less = f32_lt_quiet(a, b) || (f32_eq(a, b) && (a.v & F32_SIGN));
  if(isNaNF32UI(a.v) && isNaNF32UI(b.v)) return rtlToF32(defaultNaNF32UI);
  else return(less || isNaNF32UI(b.v) ? a : b);
}

static inline float32_t f32_max(float32_t a, float32_t b){
  bool greater = f32_lt_quiet(b, a) || (f32_eq(b, a) && (b.v & F32_SIGN));
  if(isNaNF32UI(a.v) && isNaNF32UI(b.v)) return rtlToF32(defaultNaNF32UI);
  else return(greater || isNaNF32UI(b.v) ? a : b);
}

static inline float64_t f64_min(float64_t a, float64_t b){
  bool less = f64_lt_quiet(a, b) || (f64_eq(a, b) && (a.v & F64_SIGN));
  if(isNaNF64UI(a.v) && isNaNF64UI(b.v)) return rtlToF64(defaultNaNF64UI);
  else return(less || isNaNF64UI(b.v) ? a : b);
}

static inline float64_t f64_max(float64_t a, float64_t b){
  bool greater = f64_lt_quiet(b, a) || (f64_eq(b, a) && (b.v & F64_SIGN));
  if(isNaNF64UI(a.v) && isNaNF64UI(b.v)) return rtlToF64(defaultNaNF64UI);
  else return(greater || isNaNF64UI(b.v) ? a : b);
}

uint32_t isa_fp_get_rm(Decode *s);
void isa_fp_update_ex_flags(Decode *s, uint32_t ex_flags);

def_rtl(fpcall, rtlreg_t *dest, const rtlreg_t *src1, const rtlreg_t *src2, uint32_t cmd) {
  softfloat_roundingMode = isa_fp_get_rm(s);
  int w = FPCALL_W(cmd);
  int op = FPCALL_OP(cmd);

  if (w == FPCALL_W32) {
    float32_t fsrc1 = rtlToF32(*src1);
    float32_t fsrc2 = rtlToF32(*src2);
    switch (op) {
      case FPCALL_ADD: *dest = f32_add(fsrc1, fsrc2).v; break;
      case FPCALL_SUB: *dest = f32_sub(fsrc1, fsrc2).v; break;
      case FPCALL_MUL: *dest = f32_mul(fsrc1, fsrc2).v; break;
      case FPCALL_DIV: *dest = f32_div(fsrc1, fsrc2).v; break;
      case FPCALL_MIN: *dest = f32_min(fsrc1, fsrc2).v; break;
      case FPCALL_MAX: *dest = f32_max(fsrc1, fsrc2).v; break;

      case FPCALL_SQRT: *dest = f32_sqrt(fsrc1).v; break;

      case FPCALL_MADD: *dest = f32_mulAdd(fsrc1, fsrc2, rtlToF32(*dest)).v; break;

      case FPCALL_LE: *dest = f32_le(fsrc1, fsrc2); break;
      case FPCALL_LT: *dest = f32_lt(fsrc1, fsrc2); break;
      case FPCALL_EQ: *dest = f32_eq(fsrc1, fsrc2); break;

      case FPCALL_I32ToF: *dest = i32_to_f32 (*src1).v; break;
      case FPCALL_U32ToF: *dest = ui32_to_f32(*src1).v; break;
      case FPCALL_I64ToF: *dest = i64_to_f32 (*src1).v; break;
      case FPCALL_U64ToF: *dest = ui64_to_f32(*src1).v; break;

      case FPCALL_FToI32: *dest = f32_to_i32 (fsrc1, softfloat_roundingMode, true); break;
      case FPCALL_FToU32: *dest = f32_to_ui32(fsrc1, softfloat_roundingMode, true); break;
      case FPCALL_FToI64: *dest = f32_to_i64 (fsrc1, softfloat_roundingMode, true); break;
      case FPCALL_FToU64: *dest = f32_to_ui64(fsrc1, softfloat_roundingMode, true); break;
      default: panic("op = %d not supported", op);
    }
  } else if (w == FPCALL_W64) {
    float64_t fsrc1 = rtlToF64(*src1);
    float64_t fsrc2 = rtlToF64(*src2);
    switch (op) {
      case FPCALL_ADD: *dest = f64_add(fsrc1, fsrc2).v; break;
      case FPCALL_SUB: *dest = f64_sub(fsrc1, fsrc2).v; break;
      case FPCALL_MUL: *dest = f64_mul(fsrc1, fsrc2).v; break;
      case FPCALL_DIV: *dest = f64_div(fsrc1, fsrc2).v; break;
      case FPCALL_MAX: *dest = f64_max(fsrc1, fsrc2).v; break;
      case FPCALL_MIN: *dest = f64_min(fsrc1, fsrc2).v; break;

      case FPCALL_SQRT: *dest = f64_sqrt(fsrc1).v; break;

      case FPCALL_MADD: *dest = f64_mulAdd(fsrc1, fsrc2, rtlToF64(*dest)).v; break;

      case FPCALL_LE: *dest = f64_le(fsrc1, fsrc2); break;
      case FPCALL_LT: *dest = f64_lt(fsrc1, fsrc2); break;
      case FPCALL_EQ: *dest = f64_eq(fsrc1, fsrc2); break;

      case FPCALL_I32ToF: *dest = i32_to_f64 (*src1).v; break;
      case FPCALL_U32ToF: *dest = ui32_to_f64(*src1).v; break;
      case FPCALL_I64ToF: *dest = i64_to_f64 (*src1).v; break;
      case FPCALL_U64ToF: *dest = ui64_to_f64(*src1).v; break;

      case FPCALL_FToI32: *dest = f64_to_i32 (fsrc1, softfloat_roundingMode, true); break;
      case FPCALL_FToU32: *dest = f64_to_ui32(fsrc1, softfloat_roundingMode, true); break;
      case FPCALL_FToI64: *dest = f64_to_i64 (fsrc1, softfloat_roundingMode, true); break;
      case FPCALL_FToU64: *dest = f64_to_ui64(fsrc1, softfloat_roundingMode, true); break;

      case FPCALL_F32ToF64: *dest = f32_to_f64(rtlToF32(*src1)).v; break;
      case FPCALL_F64ToF32: *dest = f64_to_f32(fsrc1).v; break;
      default: panic("op = %d not supported", op);
    }
  }

  if (softfloat_exceptionFlags) {
    isa_fp_update_ex_flags(s, softfloat_exceptionFlags);
    softfloat_exceptionFlags = 0;
  }
}