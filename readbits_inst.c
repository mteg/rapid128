
/*    u_int32_t start_symbol = ctx->read_bits(ctx, im, li, ppos, uwidth, &threshold,
                      0x0d01, 0x3fc7, 65536, &ppos);
    start_symbol &= 0x3fff;
    start_symbol == 0x0d09 || start_symbol == 0x0d21 || start_symbol == 0x0d39)
*/

#define try_return(th, res) if((res & 0x3fc7) == 0x0d01) { \
                          u_int32_t rres = res & 0x3fff; \
                          if(rres == 0x0d09 || rres == 0x0d21 || rres == 0x0d39)  \
                          {\
                            *threshold = th >> 8; \
                            if(curpos) \
                              *curpos = npos; \
                            return rres; \
                          } \
                          }

#ifdef READBITS_NAME
static u_int32_t READBITS_NAME (struct r128_ctx *ctx, struct r128_image *im, struct r128_line *li, ufloat8 ppos, 
                            ufloat8 uwidth, ufloat8 threshold,
                            int read_limit, ufloat8 *curpos)
#else
static u_int32_t FINDBITS_NAME (struct r128_ctx *ctx, struct r128_image *im, struct r128_line *li, ufloat8 ppos, 
                            ufloat8 uwidth, ufloat8 *threshold, ufloat8 *curpos)
#endif

{
  int i, w = INT_TO_UF8(li->linesize - 1);
  u_int8_t *line = im->gray_data + li->offset;
  ufloat8 npos = 0;
#ifdef FINDBITS_NAME
  int read_limit = 65535;
/* Thresholds: base, -50%, +50% */
  ufloat8 th_m = (*threshold) << 8;
  ufloat8 th_l = th_m >> 1;
  ufloat8 th_h = th_m + th_l;
/* Results */
  u_int32_t res_m = 0;
  u_int32_t res_l = 0;
  u_int32_t res_h = 0;
#else
  u_int32_t res = 0;
  
  threshold <<= 8;
#endif
  
  
  for(i = 0; i<read_limit; i++)
  {
    ufloat8 accu = 0, sppos;
    int pmin, pmax, j;

    npos = ppos + uwidth;
    if(npos >= w) break;

    sppos = UF8_CEIL(ppos);

    if(sppos < npos)
    {
      /* Add remaining fraction */
      accu += UF8_TOCEIL(ppos) * r128_read_pixel(ctx, im, li, line, UF8_INTFLOOR(ppos));
      
      /* Advance to next whole pixel */ 
      ppos = sppos;
      
      /* Number of pixels to read and average */
      pmin = UF8_INTFLOOR(ppos);
      pmax = UF8_INTFLOOR(npos);
      for(j = pmin; j<pmax; j++)
        accu += INT_TO_UF8(r128_read_pixel(ctx, im, li, line, j));
        
      /* Add remaining fraction */
      accu += UF8_FRAC(npos) * r128_read_pixel(ctx, im, li, line, UF8_INTFLOOR(npos));
    }
    else
      accu += (npos - ppos) * r128_read_pixel(ctx, im, li, line, UF8_INTFLOOR(ppos));
    

#warning lets repair negative caps
//    if(threshold < 0)
//      accu *= -1;

    /* Ready for inputting the next bit */
#ifdef FINDBITS_NAME
    res_m <<= 1; res_h <<= 1; res_l <<= 1;
    if(accu < th_l)
    {
      res_m |= 1;
      res_h |= 1;
      res_l |= 1;
    }
    else if(accu < th_m)
    {
      res_m |= 1;
      res_h |= 1;
    }
    else if(accu < th_h)
      res_h |= 1;
    
    try_return(th_h, res_h);
    try_return(th_m, res_m);
    try_return(th_l, res_l);

#else
    res <<= 1;

    /* Threshold! */
    if(accu < threshold) res |= 1;
#endif

    ppos = npos;
  }
  if(curpos)
    *curpos = npos;
  
#ifdef FINDBITS_NAME
  return 0;
#else
  return res;
#endif
}
