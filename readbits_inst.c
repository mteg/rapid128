
/*    u_int32_t start_symbol = ctx->read_bits(ctx, im, li, ppos, uwidth, &threshold,
                      0x0d01, 0x3fc7, 65536, &ppos);
    start_symbol &= 0x3fff;
    start_symbol == 0x0d09 || start_symbol == 0x0d21 || start_symbol == 0x0d39)
     12c4
     0001 0010 1100 0100
*/

#ifdef READBITS_NAME
static u_int32_t READBITS_NAME (struct r128_ctx *ctx, struct r128_image *im, struct r128_line *li, ufloat8 ppos, 
                            ufloat8 uwidth, ufloat8 threshold,
                            int read_limit, ufloat8 *curpos)
#else
ufloat8 FINDBITS_NAME (struct r128_ctx *ctx, struct r128_image *im, struct r128_line *li, ufloat8 ppos, 
                            ufloat8 uwidth, u_int32_t norm_coeff, ufloat8 *curpos)
#endif

{
  int i, w = INT_TO_UF8(li->linesize - 1);
  u_int8_t *line = im->gray_data + li->offset;
  ufloat8 npos = 0;
#ifdef FINDBITS_NAME
  const int read_limit = 65535;
  u_int8_t fsm[16] = {2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2};
  u_int32_t bucket;
  
#else
  u_int32_t res = 0;
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
    /* Need to find the bucket id */
    /*	Average pixel value | bucket ID
        0  -  15  | bucket 0
        16  -  31 | bucket 1 
        ...       |
        240 - 255 | bucket 15
        
        Floating point:
        bucket ID = accu / uwidth * 16
        bucket ID = 16 * accu / uwidth
        bucket ID = (16 / uwidth) * accu
        
        Fixed point:
        bucket ID = (16 / (uwidth / 256)) * accu / 256
        bucket ID = 16 * 1024 / uwidth * accu / 1024 (>> 10)
        
    */     
    
    /* Compute bucket */
    bucket = (norm_coeff * accu) >> 18;
//    fprintf(stderr, "bucket for %d at uw = %d is %d (norm_coeff = %d)\n", accu, uwidth, bucket, norm_coeff);
    
    /* Test if bucket is OK */
    if(bucket > 15) bucket = 15;
    
    #define try_fsm(k) if(!(fsm[k] = fsm_tab[fsm[k] + ((bucket <= k) ? 1 : 0)])) return r128_findbits_threshold(ctx, fsm, bucket, curpos, npos, k)
    try_fsm(0);  try_fsm(1);   try_fsm(2);  try_fsm(3);
    try_fsm(4);  try_fsm(5);   try_fsm(6);  try_fsm(7);
    try_fsm(8);  try_fsm(9);   try_fsm(10); try_fsm(11);
    try_fsm(12); try_fsm(13);  try_fsm(14);
    #undef try_fsm

#else
    res <<= 1;

//    fprintf(stderr, "accu is %d, th is %d\n", accu, threshold);
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
