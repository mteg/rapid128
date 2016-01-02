
/*    u_int32_t start_symbol = ctx->read_bits(ctx, im, li, ppos, uwidth, &threshold,
                      0x0d01, 0x3fc7, 65536, &ppos);
    start_symbol &= 0x3fff;
    start_symbol == 0x0d09 || start_symbol == 0x0d21 || start_symbol == 0x0d39)
*/


#ifdef READBITS_NAME
static u_int32_t READBITS_NAME (struct r128_ctx *ctx, struct r128_image *im, struct r128_line *li, ufloat8 ppos, 
                            ufloat8 uwidth, ufloat8 *threshold, ufloat8 th_weight,
                            u_int8_t negative,
                            int read_limit, ufloat8 *curpos)
#else
static u_int32_t FINDBITS_NAME (struct r128_ctx *ctx, struct r128_image *im, struct r128_line *li, ufloat8 ppos, 
                            ufloat8 uwidth, ufloat8 *threshold, ufloat8 *curpos, ufloat8 *ths)
#endif

{
  int i, w = INT_TO_UF8(li->linesize - 1);
  u_int8_t *line = im->gray_data + li->offset;
  ufloat8 npos = 0;
#ifdef FINDBITS_NAME
  int read_limit = 65535;
/* Thresholds: base, -50%, +50% */
#define READBITS_TH_STEPS 8
  u_int32_t res[READBITS_TH_STEPS];
  int z;
  memset(res, 0, sizeof(res));
  
#else
  ufloat8 th = UF8_MUL(*threshold, th_weight);
#ifdef SHADE_CAPS
  ufloat8 pos = 0, neg = 0;
  int pos_cnt = 0, neg_cnt = 0;
#endif
#ifdef NEGATIVE_CAPS
  ufloat8 neg_max = 255 * uwidth;
  if(negative) th = neg_max - th;
#endif
  u_int32_t res = 0;
#endif



#ifdef FINDBITS_NAME
  if(uwidth >= ctx->fast_uwidth)
  {    
    for(i = 0; i<read_limit; i++)
    {
      ufloat8 accu;

      npos = ppos + uwidth;
      if(unlikely(ppos >= w)) break;
      accu = r128_read_pixel(ctx, im, li, line, UF8_INTFLOOR(ppos));

      /* Ready for inputting the next bit */
      for(z = 0; z<(READBITS_TH_STEPS - 1); z++)
        res[z] = (res[z] << 1) | ((accu - (z + 1) * (256 / READBITS_TH_STEPS)) >> 31);	// < ths[z] ? 1 : 0);

  //    for(z = 0; z<(READBITS_TH_STEPS - 1); z++)
  //      if(accu < ths[z]) res[z] |= 1;

      for(z = 0; z<(READBITS_TH_STEPS - 1); z++)
        if(unlikely((res[z] & 0x3fc7) == 0x0d01))
        {
          u_int32_t rres = res[z] & 0x3fff;
          if(rres == 0x0d09 || rres == 0x0d21 || rres == 0x0d39) findbits_return(rres)
        }
  #ifdef NEGATIVE_CAPS
        else if(unlikely((res[z] & 0x3fc7) == 0x32c6))
        {
          u_int32_t rres = res[z] & 0x3fff;
          if(rres == 0x32f6 || rres == 0x32de || rres == 0x32c6) findbits_return(rres)
        }
  #endif
  #ifdef FLIP_CAPS
        else if(unlikely((res[z] & 0x3fff) == 0x05c6))
          findbits_return(0x05c6)
  #ifdef NEGATIVE_CAPS
        else if(unlikely((res[z] & 0x3fff) == 0x3a39))
          findbits_return(0x3a39)  
  #endif
  #endif 

      ppos = npos;
    }
  }
  else
#endif
  {
    
    for(i = 0; i<read_limit; i++)
    {
      ufloat8 accu = 0;
      ufloat8 sppos;
      int pmin, pmax, j;

      npos = ppos + uwidth;
      if(unlikely(npos >= w)) break;

      sppos = UF8_CEIL(ppos);

      if(likely(sppos < npos))
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
      

  //    if(threshold < 0)
  //      accu *= -1;

      /* Ready for inputting the next bit */
  #ifdef FINDBITS_NAME
      for(z = 0; z<(READBITS_TH_STEPS - 1); z++)
        res[z] = (res[z] << 1) | ((accu - ths[z]) >> 31);	// < ths[z] ? 1 : 0);

  //    for(z = 0; z<(READBITS_TH_STEPS - 1); z++)
  //      if(accu < ths[z]) res[z] |= 1;

      for(z = 0; z<(READBITS_TH_STEPS - 1); z++)
        if(unlikely((res[z] & 0x3fc7) == 0x0d01))
        {
          u_int32_t rres = res[z] & 0x3fff;
          if(rres == 0x0d09 || rres == 0x0d21 || rres == 0x0d39) findbits_return(rres)
        }
  #ifdef NEGATIVE_CAPS
        else if(unlikely((res[z] & 0x3fc7) == 0x32c6))
        {
          u_int32_t rres = res[z] & 0x3fff;
          if(rres == 0x32f6 || rres == 0x32de || rres == 0x32c6) findbits_return(rres)
        }
  #endif
  #ifdef FLIP_CAPS
        else if(unlikely((res[z] & 0x3fff) == 0x05c6))
          findbits_return(0x05c6)
  #ifdef NEGATIVE_CAPS
        else if(unlikely((res[z] & 0x3fff) == 0x3a39))
          findbits_return(0x3a39) 
  #endif
  #endif 

  #else
      res <<= 1;
      
      #ifdef NEGATIVE_CAPS
      if(negative) accu = neg_max - accu;
      #endif
      
      /* Threshold! */
      if(accu < th)
      {
#ifdef SHADE_CAPS
        neg += accu;
        neg_cnt ++;
#endif
        res |= 1;
      }
#ifdef SHADE_CAPS
      else
      {
        pos += accu;
        pos_cnt ++;
      }
#endif
  #endif

      ppos = npos;
    }
  }
  
  
  
  
  if(curpos)
    *curpos = npos;
  
#ifdef FINDBITS_NAME
  return 0;
#else
#ifdef SHADE_CAPS
  if(pos_cnt && neg_cnt)
  {
    pos = UF8_MUL(pos / pos_cnt, 65536 / th_weight);
    neg = UF8_MUL(neg / neg_cnt, 65536 / th_weight);
    *threshold = ((*threshold) * SHADE_CAPS + (pos + neg) / 2) / (SHADE_CAPS + 1);
  }
#endif
  return res;
#endif
}
