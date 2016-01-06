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
#include "readbits.h"

static inline int log2_ceil(int val)
{
  int s = 1;
  while(s <= val)
    s <<= 1;
  return s;
}


void r128_realloc_buffers(struct r128_ctx *c)
{
  int lss = c->max_height;  
  if(c->max_width > lss) lss = c->max_width;

  if(c->line_scan_alloc < lss)
  {
    c->line_scan_status = r128_realloc(c, c->line_scan_status, sizeof(int) * lss);
    memset(c->line_scan_status + c->line_scan_alloc, 0, (lss - c->line_scan_alloc) * sizeof(int));
    c->line_scan_alloc = lss;
  }
}

int r128_page_scan(struct r128_ctx *ctx, struct r128_image *img,
          ufloat8 offset, ufloat8 uwidth, int minheight, int maxheight, ufloat8 *ths)
{
  int min_ctr, max_ctr, ctr, lines_scanned = 0, rc = R128_EC_NOCODE;
  unsigned int t_start = r128_time(ctx), t_spent;
  int linemax;
  
  if(ctx->rotation & 1)
  {
    if(maxheight > img->width) maxheight = img->width;
    
    min_ctr = log2_ceil(img->width / maxheight);
    max_ctr = log2_ceil(img->width / minheight);
    linemax = img->width;
  }
  else
  {
    if(maxheight > img->height) maxheight = img->height;
  
    min_ctr = log2_ceil(img->height / maxheight);
    max_ctr = log2_ceil(img->height / minheight);
    linemax = img->height;
  }
  
  ctx->page_scan_id++;
  r128_log(ctx, R128_DEBUG1, "Page scan # %d of %simage %s starts: offs = %.3f, heights = %d ~ %d, uwidth = %.2f\n", 
        ctx->page_scan_id, img->root ? "blurred " : "", img->root ? img->root->filename : img->filename,
        UF8_FLOAT(offset), minheight, maxheight, UF8_FLOAT(uwidth));
  
  /* Do a BFS scan of page lines @ particular offset and unit width */
  for(ctr = min_ctr; ctr < max_ctr; ctr++)
  {
    /* Determine line number */
    int line_idx = (linemax * bfsguidance(ctr)) >> 16;
    
    /* Check if this line was already done in this pass */
    if(ctx->line_scan_status[line_idx] == ctx->page_scan_id) continue;
    
    /* Mark line as checked */
    ctx->line_scan_status[line_idx] = ctx->page_scan_id;
    
    /* Mark current line for code reporting */
    ctx->current_line = line_idx;
    
    /* Check the line */
    rc = minrc(rc, r128_scan_line(ctx, img, r128_get_line(ctx, img, line_idx, ctx->rotation), uwidth, offset, ths, 0));
    
    /* Increment scanned lines counter */
    lines_scanned ++;
    
    /* If we found anything, maybe we can immediately quit? */
    if(R128_ISDONE(ctx, rc))
    {
      r128_log(ctx, R128_NOTICE, "Code was found in page scan # %d, offs = %.3f, heights = %d ~ %d, uwidth = %.2f, line = %d\n", 
            ctx->page_scan_id, UF8_FLOAT(offset),  minheight, maxheight, UF8_FLOAT(uwidth), line_idx);
      img->best_rc = rc;
      img->time_spent += (t_spent = r128_time(ctx) - t_start);
      if(img->root)
      {
        img->root->best_rc = rc;
        img->root->time_spent += t_spent;
      }
      return img->best_rc;
    }
  }
  r128_log(ctx, R128_DEBUG1, "Page scan %d finished, scanned %d lines, best rc = %d\n", ctx->page_scan_id, lines_scanned, rc);
  img->best_rc = minrc(img->best_rc, rc);
  img->time_spent += (t_spent = r128_time(ctx) - t_start);
  if(img->root)
  {
    img->root->best_rc = minrc(img->root->best_rc, rc);
    img->root->time_spent += t_spent;
  }

  return img->best_rc;
}



int r128_try_tactics(struct r128_ctx *ctx, char *tactics, int start, int len, int codes_to_find)
{
  static ufloat8 offsets[] = {0, 128, 64, 192, 32, 96, 160, 224};
/*  static double offsets[] = {0, 0.5, 0.25, 0.75, 0.125, 0.375, 0.625, 0.875}; */
  int offs, offs_min = 0, offs_max = 4;
  int h_min = ctx->expected_min_height, h_max;
  char *param_input;

  int min_uw_ctr, max_uw_ctr;
  int uw_ctr, min_uw_step = 4, max_uw_step = ctx->uw_steps2;
  

  int codes_found = 0;
  
  ctx->tactics = tactics;

  if((param_input = strstr(tactics, "/")))
  {
   ctx->rotation = 0; param_input++;
   while(*(param_input++) == 'r')
    ctx->rotation++;
  }

  if(ctx->rotation & 1)
    h_max = ctx->max_width;
  else
    h_max = ctx->max_height;
  
  if(strstr(tactics, "o")) { offs_min = 4; offs_max = 8; }
  if(strstr(tactics, "h")) { h_min = 1; h_max = ctx->expected_min_height; }
  if(strstr(tactics, "w")) { min_uw_step = 1; max_uw_step = 4; }


  r128_log(ctx, R128_DEBUG1, "Now assuming tactics '%s'\n", tactics);
  r128_configure_rotation(ctx);
  

  min_uw_ctr = log2_ceil(ctx->uw_steps2 / max_uw_step);
  max_uw_ctr = log2_ceil(ctx->uw_steps2 / min_uw_step);
  memset(ctx->uw_space_visited, 0, ctx->uw_steps2);

  for(offs = offs_min; offs < offs_max && (codes_found < codes_to_find || (ctx->flags & R128_FL_READALL)); offs++)
  {
    memset(ctx->uw_space_visited, 0, ctx->uw_steps2);
    for(uw_ctr = min_uw_ctr; uw_ctr < max_uw_ctr; uw_ctr++)
    {
      int i;
      int uw_idx = (ctx->uw_steps2 * bfsguidance(uw_ctr)) >> 16;
      ufloat8 uwidth;
      ufloat8 th_step;
      ufloat8 ths[READBITS_TH_STEPS];      
      
      if(ctx->uw_space_visited[uw_idx]) continue;
      ctx->uw_space_visited[uw_idx] = 1;

      uwidth = ctx->uwidth_space[uw_idx];
      th_step = uwidth * (256 / READBITS_TH_STEPS);

      ths[0] = th_step;
      for(i = 1; i<READBITS_TH_STEPS; i++)
        ths[i] = ths[i - 1] + th_step;

      
      /* For all images */
      for(i = 0; i<len; i++)
      {
        /* Get image */
        struct r128_image *img = &ctx->im[start + i];

        if(ctx->batch_limit != 0)
        {
          unsigned int t = r128_time(ctx);
          if((t - ctx->batch_start) > ctx->batch_limit)
          {
            r128_log(ctx, R128_NOTICE, "Finishing tactics '%s'. Time exceeded, %d of %d codes found\n", tactics, codes_found, codes_to_find);
            return codes_found;
          }
        }     
      
        /* Check if we can do anything */
        if(!img->gray_data) continue;
//        if(uwidth < ctx->min_uwidth || uwidth > ctx->max_uwidth) continue;

        if(strstr(tactics, "i"))
        {
          img = r128_blur_image(ctx, start + i);
          if(!img->gray_data) continue;
        }
      
        if(ctx->flags & (R128_FL_ALIGNMENT_SHORT | R128_FL_ALIGNMENT_LONG))
        {
          if(((ctx->rotation & 1) && (ctx->flags & R128_FL_ALIGNMENT_SHORT)) ||
             ((!(ctx->rotation & 1)) && (ctx->flags & R128_FL_ALIGNMENT_LONG)))
          {
            if(img->height > img->width) /* Incorrectly rotated */
              continue;
          }
          else
          {
            if(img->width > img->height) /* Incorrectly rotated */
              continue;
          }
        }
      
        if(r128_page_scan(ctx, img, offsets[offs], uwidth, h_min, h_max, ths) == R128_EC_SUCCESS)
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
  ctx->rotation = ctx->def_rotation;
  
  ctx->min_len = UF8_INTFLOOR(4 * 13 * ctx->min_uwidth);
  ctx->max_gap = UF8_INTCEIL(6 * ctx->max_uwidth);
  
  /* Precompute data for unit width search */
  
  /* Minimum codeunit width of document is? */

  
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
        ctx->report_code(ctx, &ctx->im[start + i], ctx->report_code_arg, NULL, 0, 0, 0); 

  r128_log(ctx, R128_DEBUG2, "Giving up a batch strategy run, found %d of %d codes.\n", codes_found, codes_to_find);
  free(oalloc);
  
  return codes_found;
}

void r128_compute_uwidth_space(struct r128_ctx *ctx)
{
  ufloat8 uw = ctx->min_uwidth;	
  int i = 0, uws_alloc = 128;
  if(ctx->uw_steps2) uws_alloc = ctx->uw_steps2;
  ctx->uwidth_space = (ufloat8*) r128_malloc(ctx, sizeof(ufloat8) * uws_alloc);

  if(!ctx->uw_steps2)
  {
    /* Defaults: small step is 1 + 1/13 * 0.25, big step is 1 + 1/13 * 1.0 */
//    ufloat8 mul = 256 + 5;  /* = 1.0 + 1.0/13.0 * 0.25 */
    
    for(i = 0; uw <= ctx->max_uwidth; uw += UF8_MUL(uw, 5))
    {
      ctx->uwidth_space[i++] = uw;
      if(i == uws_alloc)
        ctx->uwidth_space = (ufloat8*) r128_realloc(ctx, ctx->uwidth_space, sizeof(ufloat8) * (uws_alloc = uws_alloc * 2));
      if(UF8_MUL(uw, 5) == 0) uw++;
    }
    
    ctx->uw_steps2 = i;
    /* small step ^ 4 = big step */
    if((!ctx->uw_steps1) || (ctx->uw_steps1 > ctx->uw_steps2))
      ctx->uw_steps1 = ctx->uw_steps2 / 4;
  }
  else
  {
    double delta = pow(UF8_FLOAT(ctx->max_uwidth) / UF8_FLOAT(ctx->min_uwidth), 1.0 / ((double) ctx->uw_steps2));
    for(i = 0; i<uws_alloc; i++, uw *= delta)
      ctx->uwidth_space[i] = uw;
  }
  if(!ctx->uw_steps1) ctx->uw_steps1 = ctx->uw_steps2;
  
  ctx->uw_space_visited = (u_int8_t*) r128_malloc(ctx, ctx->uw_steps2);
}
