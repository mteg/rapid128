
u_int32_t READBITS_NAME (struct r128_ctx *ctx, struct r128_image *im, struct r128_line *li, double ppos, 
                            double uwidth, double threshold,
                            u_int32_t pattern, u_int32_t mask, int read_limit, double *curpos)
{
  int i, w = li->linesize;
  u_int32_t res = 0;
  u_int8_t *line = im->gray_data + li->offset;
  double npos = 0;
  
  for(i = 0; i<read_limit; i++)
  {
    double accu = 0.0, sppos;
    int pmin, pmax, j;

    npos = ppos + uwidth;
 
    if(lrint(ceil(npos)) >= w) break;

    sppos = ceil(ppos);
    
    if(sppos < npos)
    {
      /* Add remaining fraction */
      accu += (ceil(ppos) - ppos) * (double) r128_read_pixel(ctx, im, li, line, lrint(floor(ppos)));
      
      /* Advance to next whole pixel */ 
      ppos = sppos;
      
      /* Number of pixels to read and average */
      pmin = lrint(ppos);
      pmax = lrint(floor(npos));
      for(j = pmin; j<pmax; j++)
        accu += (double) r128_read_pixel(ctx, im, li, line, j);
        
      /* Add remaining fraction */
      accu += (npos - floor(npos)) * (double) r128_read_pixel(ctx, im, li, line, lrint(floor(npos)));
    }
    else
      accu += (npos - ppos) * (double) r128_read_pixel(ctx, im, li, line, lrint(floor(ppos)));    
    
    /* Ready for inputting the next bit */
    res <<= 1;

    if(threshold < 0)
      accu *= -1;

    /* Threshold! */
    if(accu < threshold)
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
