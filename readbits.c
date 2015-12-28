
u_int32_t READBITS_NAME (struct r128_ctx *ctx, struct r128_image *im, struct r128_line *li, ufloat8 ppos, 
                            ufloat8 uwidth, ufloat8 threshold,
                            u_int32_t pattern, u_int32_t mask, int read_limit, ufloat8 *curpos)
{
  int i, w = INT_TO_UF8(li->linesize - 1);
  u_int32_t res = 0;
  u_int8_t *line = im->gray_data + li->offset;
  ufloat8 npos = 0;
  
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
    
    /* Ready for inputting the next bit */
    res <<= 1;

#warning lets repair negative caps
//    if(threshold < 0)
//      accu *= -1;

    /* Threshold! */
    if(accu < (threshold << 8))
      res |= 1;

    if(mask)
      if((res & mask) == pattern)
        break;
        
    ppos = npos;
  }
  if(curpos)
    *curpos = npos;
  
  return res;
}
