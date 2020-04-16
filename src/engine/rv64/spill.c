#include "spill.h"
#include "tran.h"
#include "rv64-backend/rv_ins_def.h"
#include "rtl/rtl.h"

typedef struct {
    uint32_t rvidx;
    uint32_t spmidx;
    bool used;
} Tmp_reg;
static Tmp_reg tmp_regs[TMP_REG_NUM];

void tmp_regs_init() {
  assert(TMP_REG_NUM == 2);
  tmp_regs[0].rvidx = TMP_REG_1;
  tmp_regs[1].rvidx = TMP_REG_2;
}

void tmp_regs_reset() {
  tmp_regs[0].used = 0;
  tmp_regs[0].spmidx = 0;
  tmp_regs[1].spmidx = 0;
  tmp_regs[1].used = 0;
}

uint32_t spmidx2rvidx(uint32_t spmidx) {
  for (int i = 0; i < TMP_REG_NUM; i++) {
    if (tmp_regs[i].spmidx == spmidx) {
      tmp_regs[i].used = 1;
      return tmp_regs[i].rvidx;
    }
  }
  return 0;
}

void spill_writeback(uint32_t i) {
  if (tmp_regs[i].spmidx != 0) {
    spm(sw, tmp_regs[i].rvidx, 4 * (tmp_regs[i].spmidx & ~SPMIDX_MASK));
  }
}

void spill_writeback_all() {  // can be 0/1/2 inst
  for (int i = 0; i < TMP_REG_NUM; i++) {
    spill_writeback(i);
  }
}

void spill_flush(uint32_t spmidx) {
  for (int i = 0; i < TMP_REG_NUM; i++) {
    if (tmp_regs[i].spmidx == spmidx) {
      tmp_regs[i].used = 0;
      return;
    }
  }
}

void spill_flush_all() {
  for (int i = 0; i < TMP_REG_NUM; i++) {
    tmp_regs[i].used = 0;
  }
}

uint32_t spill_out_and_remap(DecodeExecState *s, uint32_t spmidx) {
  int tmpidx;
  for (tmpidx = 0; tmpidx < TMP_REG_NUM; tmpidx ++) {
    if (tmp_regs[tmpidx].used == 0) break;
  }
  Assert(tmpidx < TMP_REG_NUM, "no clean tmp_regs!\nalready used:%u %u, req: %u\n",
      tmp_regs[0].spmidx, tmp_regs[1].spmidx, spmidx);

  spill_writeback(tmpidx);
  spm(lw, tmp_regs[tmpidx].rvidx, 4 * (spmidx & ~SPMIDX_MASK));

  tmp_regs[tmpidx].spmidx = spmidx;
  tmp_regs[tmpidx].used = 1;

  return tmp_regs[tmpidx].rvidx;
}
