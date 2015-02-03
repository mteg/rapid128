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

static inline int log2_ceil(int val)
{
  int s = 1;
  while(s <= val)
    s <<= 1;
  return s;
}


void r128_realloc_buffers(struct r128_ctx *c)
{
  if(c->line_scan_alloc < c->max_height)
  {
    c->line_scan_status = r128_realloc(c, c->line_scan_status, sizeof(int) * c->max_height);
    memset(c->line_scan_status + c->line_scan_alloc, 0, (c->max_height - c->line_scan_alloc) * sizeof(int));
    c->line_scan_alloc = c->max_height;
  }
}

int r128_page_scan(struct r128_ctx *ctx, struct r128_image *img,
          double offset, double uwidth, int minheight, int maxheight)
{
  int min_ctr, max_ctr, ctr, lines_scanned = 0, rc = R128_EC_NOCODE;
  double t_start = r128_time(ctx);
  
  if(maxheight > img->height) maxheight = img->height;
  
  min_ctr = log2_ceil(img->height / maxheight);
  max_ctr = log2_ceil(img->height / minheight);
  
  ctx->page_scan_id++;
  r128_log(ctx, R128_DEBUG1, "Page scan # %d of %simage %s starts: offs = %.3f, th = %.2f, heights = %d ~ %d, uwidth = %.2f\n", 
        ctx->page_scan_id, img->root ? "blurred " : "", img->root ? img->root->filename : img->filename, offset, ctx->threshold, minheight, maxheight, uwidth);
  
  /* Do a BFS scan of page lines @ particular offset and unit width */
  for(ctr = min_ctr; ctr < max_ctr; ctr++)
  {
    /* Determine line number */
    int line_idx = (img->height * bfsguidance(ctr)) >> 16;
    
    /* Check if this line was already done in this pass */
    if(ctx->line_scan_status[line_idx] == ctx->page_scan_id) continue;
    
    /* Mark line as checked */
    ctx->line_scan_status[line_idx] = ctx->page_scan_id;

    /* Check the line */
    rc = minrc(rc, r128_scan_line(ctx, img, r128_get_line(ctx, img, line_idx), uwidth, offset, ctx->threshold));
    
    /* Increment scanned lines counter */
    lines_scanned ++;
    
    /* If we found anything, maybe we can immediately quit? */
    if(R128_ISDONE(ctx, rc))
    {
      r128_log(ctx, R128_NOTICE, "Code was found in page scan # %d, offs = %.3f, th = %.2f, heights = %d ~ %d, uwidth = %.2f, line = %d\n", 
            ctx->page_scan_id, offset, ctx->threshold, minheight, maxheight, uwidth, line_idx);
      img->best_rc = rc;
      img->time_spent += r128_time(ctx) - t_start;
      return img->best_rc;
    }
  }
  r128_log(ctx, R128_DEBUG1, "Page scan %d finished, scanned %d lines, best rc = %d\n", ctx->page_scan_id, lines_scanned, rc);
  img->best_rc = minrc(img->best_rc, rc);
  img->time_spent += r128_time(ctx) - t_start;

  return img->best_rc;
}



int r128_try_tactics(struct r128_ctx *ctx, char *tactics, int start, int len, int codes_to_find)
{
  static double offsets[] = {0, 0.5, 0.25, 0.75, 0.125, 0.375, 0.625, 0.875};
  int w_ctr_min = 1, w_ctr_max = ctx->wctrmax_stage1, w_ctr;
  int offs, offs_min = 0, offs_max = 4;
  int h_min = ctx->expected_min_height, h_max = ctx->max_height;
  char *thresh_input;

  int codes_found = 0;
  
  ctx->tactics = tactics;
  
  if(strstr(tactics, "o")) { offs_min = 4; offs_max = 8; }
  if(strstr(tactics, "h")) { h_min = 1; h_max = ctx->expected_min_height; }
  if(strstr(tactics, "w")) { w_ctr_min = w_ctr_max; w_ctr_max = ctx->wctrmax_stage2; }

  if((thresh_input = strstr(tactics, "@")))
    if(sscanf(thresh_input + 1, "%lf", &ctx->threshold) != 1)
      r128_log(ctx, R128_NOTICE, "Invalid threshold in tactics: '%s', keeping to old threshold, equal %.2f.\n", tactics, ctx->threshold);

  r128_log(ctx, R128_DEBUG1, "Now assuming tactics '%s'\n", tactics);
  r128_configure_rotation(ctx);
  
  for(offs = offs_min; offs < offs_max && (codes_found < codes_to_find || (ctx->flags & R128_FL_READALL)); offs++)
    for(w_ctr = w_ctr_min; w_ctr < w_ctr_max; w_ctr++)
    {
      /* First, determine document width in code units for this try */
      double cuw = ctx->min_doc_cuw + ctx->doc_cuw_span * bfsguidance(w_ctr) / 65536.0;
      int i;
      
      /* For all images */
      for(i = 0; i<len; i++)
      {
        /* Get image */
        struct r128_image *img = &ctx->im[start + i];

        /* Determine unit width to act with for this w_ctr */
        double uwidth = ((double) img->width) / cuw;
      
        if(ctx->batch_limit != 0.0)
        {
          double t = r128_time(ctx);
          if((t - ctx->batch_start) > ctx->batch_limit)
          {
            r128_log(ctx, R128_NOTICE, "Finishing tactics '%s'. Time exceeded, %d of %d codes found\n", tactics, codes_found, codes_to_find);
            return codes_found;
          }
        }     
      
        /* Check if we can do anything */
        if(!img->gray_data) continue;
        if(uwidth < ctx->min_uwidth || uwidth > ctx->max_uwidth) continue;

        if(strstr(tactics, "i"))
        {
          img = r128_blur_image(ctx, start + i);
          if(!img->gray_data) continue;
        }
      
        if(r128_page_scan(ctx, img, offsets[offs], uwidth, h_min, h_max) == R128_EC_SUCCESS)
        {
          codes_found++;
          if(!(ctx->flags & R128_FL_READALL))
          {
            r128_free_image(ctx, &ctx->im[start + i]);
            r128_free_image(ctx, &ctx->im_blurred[start + i]);            
          }
        }
        
        
      }
      if(codes_found >= codes_to_find) 
       if(!(ctx->flags & R128_FL_READALL)) 
        break;
    }
  
  r128_log(ctx, R128_DEBUG2, "Finishing tactics '%s': %d of %d codes found\n", tactics, codes_found, codes_to_find);
  return codes_found;
}

int r128_run_strategy(struct r128_ctx *ctx, char *strategy, int start, int len)
{
  int codes_found = 0, codes_to_find = 0, i; 
  char *oalloc;

  /* Reallocate global buffers to match potentially new image sizes */
  r128_realloc_buffers(ctx);
  
  assert((oalloc = strategy = strdup(strategy)));

  /* Precompute data for cropping lines */
  
  /* A meaningful barcode is at least 4 * 13 bits * unit_width pixels wide */
  /* (START-X) 1 symbol checksum (END) */
  
  /* This means, that, taking a minimal assumed unit width,
     if at position 4*13*min_uwidth there was still no barcode, it means
     all there was is a garbage 
     
     On the other hand, there cannot be a gap of more than
     max_uwidth * 4 white/black pixels. This follows from how
     the code is constructed (Never more than 4 ones or zeros in row)
     
     We take an extra margin and assume 6.
  */
  ctx->threshold = ctx->def_threshold;
  ctx->rotation = ctx->def_rotation;
  
  printf("ROT = %d\n", ctx->rotation);
  
  ctx->min_len = lrint(floor(4.0 * 13.0 * ctx->min_uwidth));
  ctx->max_gap = lrint(ceil(6.0 * ctx->max_uwidth));
  
  /* Precompute data for unit width search */
  
  /* Minimum codeunit width of document is? */

#warning think a while!
  ctx->min_doc_cuw = ((double) ctx->min_width) / ctx->max_uwidth;
  ctx->max_doc_cuw = ((double) ctx->max_width) / ctx->min_uwidth;
  
  /* Therefore, we span a range of...? */
  ctx->doc_cuw_span = ctx->max_doc_cuw - ctx->min_doc_cuw;
  
  /* Number of subdivisions required to reach a difference less than cuw_delta1? */
  ctx->wctrmax_stage1 = log2_ceil(lrint(ctx->doc_cuw_span / ctx->min_cuw_delta1));
  ctx->wctrmax_stage2 = log2_ceil(lrint(ctx->doc_cuw_span / ctx->min_cuw_delta2));
  
  /* Compute # of codes to find */
  for(i = 0; i<len; i++)
    if(ctx->im[start + i].gray_data)
      codes_to_find++;
  
  while(strategy && (codes_found < codes_to_find || (ctx->flags & R128_FL_READALL)))
  {
    char *next_strategy = index(strategy, ',');
    
    if(next_strategy) *(next_strategy++) = 0;
    codes_found += r128_try_tactics(ctx, strategy, start, len, codes_to_find - codes_found);
    strategy = next_strategy;
  }
  if(ctx->flags & R128_FL_EREPORT)
    for(i = 0; i<len; i++)
      if(ctx->im[start + i].best_rc != R128_EC_SUCCESS)
        r128_report_code(ctx, &ctx->im[start + i], NULL, 0); 

  r128_log(ctx, R128_DEBUG2, "Giving up a batch strategy run, found %d of %d codes.\n", codes_found, codes_to_find);
  free(oalloc);
  
  return codes_found;
}

