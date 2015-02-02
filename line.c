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



u_int32_t r128_read_bits(struct r128_ctx *ctx, u_int8_t *line, int w, double ppos, 
                            double uwidth, double threshold,
                            u_int32_t pattern, u_int32_t mask, int read_limit, double *curpos)
{
  int i;
  u_int32_t res = 0;
  
  for(i = 0; i<read_limit; i++)
  {
    double accu = 0.0, npos, sppos;
    int pmin, pmax, j;

    npos = ppos + uwidth;
 
    if(lrint(ceil(npos)) >= w) 
    {
      if(curpos) *curpos = npos;
      break;
    }

    sppos = ceil(ppos);
    
    if(sppos < npos)
    {
      /* Add remaining fraction */
      accu += (ceil(ppos) - ppos) * (double) line[lrint(floor(ppos))];
      
      /* Advance to next whole pixel */ 
      ppos = sppos;
      
      /* Number of pixels to read and average */
      pmin = lrint(ppos);
      pmax = lrint(floor(npos));
      for(j = pmin; j<pmax; j++)
        accu += (double) line[j];
        
      /* Add remaining fraction */
      accu += (npos - floor(npos)) * (double) line[lrint(floor(npos))];
    }
    else
      accu += (npos - ppos) * (double) line[lrint(floor(ppos))];    
    
    /* Ready for inputting the next bit */
    res <<= 1;

    /* Threshold! */
    if(accu < threshold)
      res |= 1;

    if(curpos)
      *curpos = npos;

    if(mask)
      if((res & mask) == pattern)
        return res;
        
    ppos = npos;
  }
  
  return res;
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


int r128_read_code(struct r128_ctx *ctx, struct r128_image *img, u_int8_t *line, int w, double ppos, double uwidth, double threshold)
{
  int rc = R128_EC_NOEND;
//  int i_thresh = lrint(threshold / uwidth);
  static double weights[] = {1.0, 1.02, 0.98, 1.01, 0.99};
  double new_ppos;
  
  while(lrint(ceil(ppos + 3.0 * uwidth)) < w) 
  {
//    int i, cs, i_ppos, offset;
    int cs, i;
    u_int32_t code;
    
    /* Przejście między symbolami: Biały/Czarny */

    /* Pod ppos powinien być czarny piksel, a przed - biały */
    
/*    i_ppos = lrint(floor(ppos));
    
    for(offset = 0; offset < 5; offset++)
    {
      if(line[i_ppos + offset] < i_thresh && line[i_ppos + offset - 1] > i_thresh)
      {
        printf("corrected by %d; @%d: %d %d\n", offset, i_ppos + offset - 1, line[i_ppos + offset - 1], line[i_ppos + offset]);
        i_ppos += offset;
        break;
      }
      if(line[i_ppos - offset] < i_thresh && line[i_ppos - offset - 1] > i_thresh)
      {
        printf("corrected by %d; @%d: %d %d\n", offset, i_ppos - offset - 1, line[i_ppos - offset - 1], line[i_ppos - offset]);
        i_ppos -= offset;
        break;
      }
    }  
    ppos = i_ppos;*/
//    ppos = floor(ppos);
    
    for(i = 0; i<5; i++) 
    {
      code = r128_read_bits(ctx, line, w, ppos - uwidth, uwidth * weights[i], threshold * weights[i], 0, 0, 13, &new_ppos);
      cs = code_symbols[(code >> 2) & 0x1ff];
    
      if(!((code & 2) || (!(code & 1)) || (!(code & 0x800)) || (code & 0x1000) || cs == -1))
        break;
      {
//        int i;
//        fprintf(stderr, " ER");
//        for(i = 12; i>=0; i--)
//          fprintf(stderr, ((code >> i) & 0x1) ? "1" : "0");
      }
    }
    
    if(i == 5)
    {
//        fprintf(stderr, "\n");
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
    
//    fprintf(stderr, "[%d] ", cs); 
    if(cs == 106)
    {
      r128_update_best_code(ctx, img, ctx->codebuf, ctx->codepos);
      /* Proper STOP! */
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
  u_int8_t *line = im->gray_data + li->offset;
  
  if(!w) return R128_EC_NOLINE;
    
  threshold *= 255.0 * uwidth;

  while(1)
  {
    u_int32_t start_symbol = r128_read_bits(ctx, line, w, ppos, uwidth, threshold, 
                      0x0d01, 0x3fc1, 65536, &ppos);
    
    if(lrint(ceil(ppos + 3.0 * uwidth)) >= w) break; /* Nothing interesting found */
    
    start_symbol &= 0x3fff;

    if(start_symbol == 0x0d09 || start_symbol == 0x0d21 || start_symbol == 0x0d39)
    {
      ctx->codepos = 0;
      ctx->codebuf[ctx->codepos++] = code_symbols[(start_symbol >> 2) & 0x1ff];

      /* Perhaps we found a code! */
      rc = minrc(rc, r128_read_code(ctx, im, line, w, ppos - uwidth, uwidth, threshold));

      /* Have we succeeded? */
      if(R128_ISDONE(ctx, rc)) 
        return rc;
    }
    
    /* Go back a little and try again */
    ppos -= uwidth * 6.0;
  }
  
  return rc;
}

