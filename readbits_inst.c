
/*    u_int32_t start_symbol = ctx->read_bits(ctx, im, li, ppos, uwidth, &threshold,
                      0x0d01, 0x3fc7, 65536, &ppos);
    start_symbol &= 0x3fff;
    start_symbol == 0x0d09 || start_symbol == 0x0d21 || start_symbol == 0x0d39)
*/


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
#define TH_STEPS 8
  ufloat8 th_step = uwidth * (256 / TH_STEPS);
  u_int32_t res[TH_STEPS];
  ufloat8 ths[TH_STEPS];
  int z;
  memset(res, 0, sizeof(res));
  ths[0] = th_step;
  for(i = 1; i<TH_STEPS; i++)
    ths[i] = ths[i - 1] + th_step;
  
#else
  u_int32_t res = 0;
#endif
  
  
  for(i = 0; i<read_limit; i++)
  {
    int32_t accu = 0;
    ufloat8 sppos;
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
    for(z = 0; z<(TH_STEPS - 1); z++)
    {
      res[z] <<= 1;
      if(accu < ths[z]) res[z] |= 1;

      if((res[z] & 0x3fc7) == 0x0d01)
      {
        u_int32_t rres = res[z] & 0x3fff;
        if(rres == 0x0d09 || rres == 0x0d21 || rres == 0x0d39)
        {
          if(curpos) *curpos = npos;
          *threshold = ths[z];
//          fprintf(stderr, "GOTIT %04x th = %d\n", rres, *threshold);
          return rres;
        }
      }
    }
#else
    res <<= 1;
    
//    fprintf(stderr, "accu = %d, th = %d\n", accu, threshold);

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
//  fprintf(stderr, "exit!\n");
  return res;
#endif
}
