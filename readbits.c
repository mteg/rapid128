#include <sys/types.h>
#include <stdio.h>
#include "rapid128.h"
#include "readbits_fsm.h"

ufloat8 r128_findbits_threshold(struct r128_ctx *ctx, u_int8_t *fsm, u_int32_t bucket, ufloat8 *curpos, ufloat8 ppos, u_int8_t fsm_id)
{
  int i;
  if(curpos) *curpos = ppos;
  
  /* Complete FSM for next buckets */
  for(i = fsm_id + 1; i<15; i++)
    if((fsm[i] = fsm_tab[fsm[i] + ((bucket <= i) ? 1 : 0)]))
      break;
  
  i--;
//  fprintf(stderr, "find_thr: 1st bucket: %d last bucket: %d, thr: %d\n", fsm_id, i, ((i + 1) << 3) + ((fsm_id + 1) << 3));
//  _exit(0);
  /* Get threshold for 1st bucket and for last bucket and average them */
  return (((i + 1) << 3) + ((fsm_id + 1) << 3));
}

#define READBITS_NAME r128_read_bits_ltor
#define r128_read_pixel(ctx, im, li, line, pos) (line[pos])
#include "readbits_inst.c"
#undef READBITS_NAME
#define FINDBITS_NAME r128_find_bits_ltor
#include "readbits_inst.c"
#undef FINDBITS_NAME
#undef r128_read_pixel

#define READBITS_NAME r128_read_bits_rtol
#define r128_read_pixel(ctx, im, li, line, pos) (line[li->linesize - 1 - (pos)])
#include "readbits_inst.c"
#undef READBITS_NAME
#define FINDBITS_NAME r128_find_bits_rtol
#include "readbits_inst.c"
#undef FINDBITS_NAME
#undef r128_read_pixel

#define READBITS_NAME r128_read_bits_ttob
#define r128_read_pixel(ctx, im, li, line, pos) (line[(pos) * im->width])
#include "readbits_inst.c"
#undef READBITS_NAME
#define FINDBITS_NAME r128_find_bits_ttob
#include "readbits_inst.c"
#undef FINDBITS_NAME
#undef r128_read_pixel

#define READBITS_NAME r128_read_bits_btot
#define r128_read_pixel(ctx, im, li, line, pos) (line[(li->linesize - 1 - (pos)) * im->width])
#include "readbits_inst.c"
#undef READBITS_NAME
#define FINDBITS_NAME r128_find_bits_btot
#include "readbits_inst.c"
#undef FINDBITS_NAME
#undef r128_read_pixel

void r128_configure_rotation(struct r128_ctx *ctx)
{
  switch(ctx->rotation)
  {
    default:
      ctx->read_bits = r128_read_bits_ltor;
      ctx->find_bits = r128_find_bits_ltor;
      break;
    case 1:
      ctx->read_bits = r128_read_bits_ttob;
      ctx->find_bits = r128_find_bits_ttob;
      break;
    case 2:
      ctx->read_bits = r128_read_bits_rtol;
      ctx->find_bits = r128_find_bits_rtol;
      break;
    case 3:
      ctx->read_bits = r128_read_bits_btot;
      ctx->find_bits = r128_find_bits_btot;
      break;
  }
}
