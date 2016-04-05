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

int r128_parse(struct r128_ctx *ctx, struct r128_image *img, u_int8_t *symbols, int len, ufloat8 startsat, ufloat8 codewidth)
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
      return ctx->report_code(ctx, img, ctx->report_code_arg, outbuf, ppos, startsat, codewidth);
    else if(c < 0 && c > 105)
      nb = sprintf(outbuf + pos, " - ?? %d ?? - ", c); 
    else
      nb = sprintf(outbuf + pos, "%s", code_tables[table][c]);

    ppos = pos; pos += nb;
  }
  return R128_EC_NOEND;
}


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


int r128_read_code(struct r128_ctx *ctx, struct r128_image *img, struct r128_line *li, ufloat8 ppos, ufloat8 uwidth, ufloat8 threshold)
{
  int rc = R128_EC_NOEND;
  static ufloat8 uwidth_weights[] = {256, 269, 243, 261, 251};
  ufloat8 new_ppos, start_ppos = ppos;
  int w = li->linesize;
#ifdef SHADE_CAPS
  ufloat8 best_goodness;
  int32_t th_offsets[] = {0, 16, -16};
  int32_t uw_offsets[] = {0, 32, -32};
#endif
  
  while(UF8_INTCEIL(ppos) < w) 
  {
    int cs = -1, uw_w;
    u_int32_t code;
#ifdef SHADE_CAPS   
    int th_o, uw_o;
    ufloat8 best_threshold = 0;
    
    if(ctx->flags & R128_FL_RB_ADAPTATIVE)
    /* Find next symbol the adaptative way */
    {
      /* Find best goodness in 45 tries */
      best_goodness = 0;
      for(uw_w = 0; uw_w < 5; uw_w++)
      {
        ufloat8 this_uwidth = UF8_MUL(uwidth, uwidth_weights[uw_w]);
        for(th_o = 0; th_o < 3; th_o++)
        {
          ufloat8 this_threshold = threshold + th_offsets[th_o] * this_uwidth;
          if(this_threshold >= (this_uwidth * 256)) continue;
          for(uw_o = 0; uw_o<3; uw_o++)
          {
            int32_t this_offset = UF8_MUL(uw_offsets[uw_o], (int) this_uwidth);
            int this_cs;
            ufloat8 this_new_ppos;

            code = ctx->read_bits(ctx, img, li, ppos - uwidth + this_offset, this_uwidth, this_threshold, uwidth_weights[uw_w],  13, &this_new_ppos);
            this_cs = code_symbols[(code >> 2) & 0x1ff];      
            
            /* Wrong code, perhaps? */
            if(((code & 2) || (!(code & 1)) || (!(code & 0x800)) || (code & 0x1000) || this_cs == -1)) continue;
            
            /* Everything OK, let's check goodness. */
            if(ctx->symbol_goodness > best_goodness)
            {
              best_goodness = ctx->symbol_goodness;
              best_threshold = this_threshold;
              new_ppos = this_new_ppos;
              cs = this_cs;
            }
          }
        }
      }
      threshold = best_threshold;
    }
    else
#endif
    /* Find next symbol the hard way */
    {
      for(uw_w = 0; uw_w<5; uw_w++) 
      {
        int this_cs;
        code = ctx->read_bits(ctx, img, li, ppos - uwidth, UF8_MUL(uwidth, uwidth_weights[uw_w]), threshold, uwidth_weights[uw_w], 13, &new_ppos);
        this_cs = code_symbols[(code >> 2) & 0x1ff];
        if(!((code & 2) || (!(code & 1)) || (!(code & 0x800)) || (code & 0x1000) || this_cs == -1))
        {
          cs = this_cs;
          goto got_valid_symbol;
        }
      }
    }
    
    if(cs == -1)
    {
      r128_update_best_code(ctx, img, ctx->codebuf, ctx->codepos);
      return R128_EC_SYNTAX; 
    }

got_valid_symbol:
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
      return r128_parse(ctx, img, ctx->codebuf, ctx->codepos, start_ppos, new_ppos - uwidth - start_ppos);
    }
    ppos = new_ppos - uwidth;
  }
  
  return rc;
}
  
int r128_scan_line(struct r128_ctx *ctx, struct r128_image *im, struct r128_line *li, ufloat8 uwidth, ufloat8 offset, ufloat8 *ths, u_int8_t no_recursion)
{
  
/*
  00 1101 0000 1001 = 0x0d09
  00 1101 0010 0001 = 0x0d21
  00 1101 0011 1001 = 0x0d39  
  00 1101 00XX X001 = 0x0d01 = pattern
                      0x3fc1 = mask (bug? 3fc7 ??)
 */ 
  int rc = R128_EC_NOCODE;
  ufloat8 ppos = UF8_MUL(offset, uwidth), w = li->linesize;
  if(!w) return R128_EC_NOLINE;
    
//   *= 255.0 * uwidth;

  while(1)
  {
    ufloat8 threshold;
    u_int32_t start_symbol = ctx->find_bits(ctx, im, li, ppos, uwidth, &threshold, &ppos, ths);

    if(UF8_INTCEIL(ppos + 3 * 11 * uwidth) >= w) {
//      fprintf(stderr, "sorry, %d is too late (%d)\n", UF8_INTCEIL(ppos + 3 * 11 * uwidth), w);
      break; /* Nothing interesting found - start comes too late for a meaningful code! */    
    }

    if(start_symbol)
    {
#ifdef NEGATIVE_CAPS
      int fl = (start_symbol & 0x3000) ? R128_FL_RB_NEGATIVE : 0;
      if(fl) start_symbol = (~start_symbol) & 0x3fff;
#else
      int fl = 0;
#endif 
#ifdef FLIP_CAPS
      if(start_symbol == 0x0d09 || start_symbol == 0x0d21 || start_symbol == 0x0d39)
      {
#endif

      ctx->codepos = 0;
      ctx->codebuf[ctx->codepos++] = code_symbols[(start_symbol >> 2) & 0x1ff];

      ctx->flags = (ctx->flags & ~R128_FL_RB_MASK) | fl;
      
      /* Perhaps we found a code! */
      rc = minrc(rc, r128_read_code(ctx, im, li, ppos - uwidth, uwidth, threshold));

      /* Have we succeeded? */
      if(R128_ISDONE(ctx, rc)) 
        return rc;
#ifdef SHADE_CAPS
      else if((rc != R128_EC_SUCCESS) && (rc <= R128_EC_NOEND))
      {
        /* Turn on shade caps? */
        ctx->codepos = 1;
        ctx->flags |= R128_FL_RB_ADAPTATIVE;
        rc = minrc(rc, r128_read_code(ctx, im, li, ppos - uwidth, uwidth, threshold));
        /* Have we succeeded? */
        if(R128_ISDONE(ctx, rc)) 
          return rc;
      }
#endif
#ifdef FLIP_CAPS
      }
      else if(!no_recursion)
      {
        static ufloat8 offsets[] = {0, 128, 64, 192, 32, 96, 160, 224};
        int k;
        /* Flip rotation */
        ctx->rotation += 2; ctx->rotation &= 3; r128_configure_rotation(ctx);
        
        r128_log(ctx, R128_DEBUG2, "Performing extra flipped scan at rot %d.\n", ctx->rotation);
        /* Try all offsets */
        for(k = 0; k<8; k++)
        {
          rc = minrc(rc, r128_scan_line(ctx, im, li, uwidth, offsets[k], ths, 1));
         
          /* Have we succeeded? */
          if(R128_ISDONE(ctx, rc)) 
          {
            /* Revert to old rotation */
            ctx->rotation += 2; ctx->rotation &= 3; r128_configure_rotation(ctx);
            return rc;
          }
        }
        /* Revert to old rotation */
        ctx->rotation += 2; ctx->rotation &= 3; r128_configure_rotation(ctx);
      }
#endif
    }
    
    /* Go back and try again */
    ppos -= uwidth * 10;
  }
  
  return rc;
}


