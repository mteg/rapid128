#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ctype.h>
#include "rapid128.h"

extern char code_symbols[];
extern char ** code_tables[];

int r128_cksum(u_int8_t *symbols, int len)
{
  int i, wsum = symbols[0];

  for(i = 1; i<len; i++)
    wsum += ((int) symbols[i]) * i;

  return wsum % 103;
}

int r128_parse(struct r128_ctx *ctx, struct r128_image *img, u_int8_t *symbols, int len)
{
  int table = 0, i;
  char outbuf[1024];
  int pos = 0, ppos = 0;

  /* Min-length */
  if(len < 3)
    return R128_EC_SYNTAX;
  
  /* Checksum! */
  if(!(ctx->flags & R128_FL_NOCKSUM))
    if(r128_cksum(symbols, len - 2) != symbols[len - 2])
      return R128_EC_CHECKSUM; 
  
  outbuf[0] = 0;
  
  for(i = 0; i<len; i++)
  {
    int c = symbols[i], nb = 0;
   
    if(c == 103)
      table = 0; /* START A */
    else if(c == 104)
      table = 1; /* START B */
    else if(c == 105)
      table = 2; /* START C */
    else if(c == 99 && table != 2)
      table = 2; /* CODE C */
    else if(c == 100 && table != 1)
      table = 1; /* CODE B */
    else if(c == 101 && table != 0)
      table = 0; /* CODE A */
    else if(c == 106)
      return r128_report_code(ctx, img, outbuf, ppos);
    else if(c < 0 && c > 105)
      nb = sprintf(outbuf + pos, " - ?? %d ?? - ", c); 
    else
      nb = sprintf(outbuf + pos, "%s", code_tables[table][c]);

    ppos = pos; pos += nb;
  }
  return R128_EC_NOEND;
}

#define READBITS_NAME r128_read_bits_ltor
#define r128_read_pixel(ctx, im, li, line, pos) (line[pos])
#include "readbits.c"
#undef READBITS_NAME
#undef r128_read_pixel

#define READBITS_NAME r128_read_bits_rtol
#define r128_read_pixel(ctx, im, li, line, pos) (line[li->linesize - 1 - (pos)])
#include "readbits.c"
#undef READBITS_NAME
#undef r128_read_pixel

#define READBITS_NAME r128_read_bits_ttob
#define r128_read_pixel(ctx, im, li, line, pos) (line[(pos) * im->width])
#include "readbits.c"
#undef READBITS_NAME
#undef r128_read_pixel

#define READBITS_NAME r128_read_bits_btot
#define r128_read_pixel(ctx, im, li, line, pos) (line[(li->linesize - 1 - (pos)) * im->width])
#include "readbits.c"
#undef READBITS_NAME
#undef r128_read_pixel

void r128_update_best_code(struct r128_ctx *ctx, struct r128_image *im, u_int8_t *symbols, int len)
{
  if(im->bestcode_len > len) return;
  if(im->bestcode_alloc < len)
  {
    while(im->bestcode_alloc < len)
      im->bestcode_alloc *= 2;
    im->bestcode = (u_int8_t*) r128_realloc(ctx, im->bestcode, im->bestcode_alloc);
  }
  
  memcpy(im->bestcode, symbols, len);
  im->bestcode_len = len;
}


int r128_read_code(struct r128_ctx *ctx, struct r128_image *img, struct r128_line *li, double ppos, double uwidth, double threshold)
{
  int rc = R128_EC_NOEND;
  static double weights[] = {1.0, 1.05, 0.95, 1.02, 0.98};
  double new_ppos;
  int w = li->linesize;
  
  while(lrint(ceil(ppos)) < w) 
  {
    int cs, i;
    u_int32_t code;
    
    for(i = 0; i<5; i++) 
    {
      code = ctx->read_bits(ctx, img, li, ppos - uwidth, uwidth * weights[i], threshold * weights[i], 0, 0, 13, &new_ppos);
      cs = code_symbols[(code >> 2) & 0x1ff];
    
      if(!((code & 2) || (!(code & 1)) || (!(code & 0x800)) || (code & 0x1000) || cs == -1))
        break;
    }
    
    if(i == 5)
    {
        r128_update_best_code(ctx, img, ctx->codebuf, ctx->codepos);
        return R128_EC_SYNTAX; 
    }
    
    ctx->codebuf[ctx->codepos++] = cs;
    
    /* Perhaps we need to extend the code */
    if(ctx->codepos >= ctx->codealloc)
    {
      ctx->codealloc *= 2;
      assert((ctx->codebuf = (u_int8_t*) r128_realloc(ctx, ctx->codebuf, sizeof(u_int8_t) * ctx->codealloc)));
    }
    
    if(cs == 106)
    {
      /* Proper STOP! */
      r128_update_best_code(ctx, img, ctx->codebuf, ctx->codepos);
      return r128_parse(ctx, img, ctx->codebuf, ctx->codepos);
    }
    ppos = new_ppos - uwidth;
  }
  
  return rc;
}
  
int r128_scan_line(struct r128_ctx *ctx, struct r128_image *im, struct r128_line *li, double uwidth, double offset, double threshold)
{
  
/*
  00 1101 0000 1001 = 0x0d09
  00 1101 0010 0001 = 0x0d21
  00 1101 0011 1001 = 0x0d39  
  00 1101 00XX X001 = 0x0d01 = pattern
                      0x3fc1 = mask
 */ 
  int rc = R128_EC_NOCODE;
  double ppos = offset * uwidth, w = li->linesize;
  
  if(!w) return R128_EC_NOLINE;
    
  threshold *= 255.0 * uwidth;

  while(1)
  {
    u_int32_t start_symbol = ctx->read_bits(ctx, im, li, ppos, uwidth, threshold, 
                      0x0d01, 0x3fc1, 65536, &ppos);
    if(lrint(ceil(ppos + 3.0 * 11.0 * uwidth)) >= w) break; /* Nothing interesting found - start comes too late for a meaningful code! */
    
    start_symbol &= 0x3fff;

    if(start_symbol == 0x0d09 || start_symbol == 0x0d21 || start_symbol == 0x0d39)
    {
      ctx->codepos = 0;
      ctx->codebuf[ctx->codepos++] = code_symbols[(start_symbol >> 2) & 0x1ff];

      /* Perhaps we found a code! */
      rc = minrc(rc, r128_read_code(ctx, im, li, ppos - uwidth, uwidth, threshold));
      /* Have we succeeded? */
      if(R128_ISDONE(ctx, rc)) 
        return rc;
    }
    
    /* Go back half a symbol and try again */
    ppos -= uwidth * 6.0; 
  }
  
  return rc;
}


void r128_configure_rotation(struct r128_ctx *ctx)
{
  switch(ctx->rotation)
  {
    default: ctx->read_bits = r128_read_bits_ltor; break;
    case 1: ctx->read_bits = r128_read_bits_ttob; break;
    case 2: ctx->read_bits = r128_read_bits_rtol; break;
    case 3: ctx->read_bits = r128_read_bits_btot; break;
  }
}
