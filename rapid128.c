#include <stdio.h>
#include <errno.h>
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

int main(int argc, char ** argv)
{
  struct r128_ctx ctx;
  int c, i;

  r128_defaults(&ctx);
  
  while(r128_getopt(&ctx, argc, argv) != EOF);
  

  if(ctx.batch_size < 1) ctx.batch_size = 1;
  if(ctx.blurring_height == 0)
    ctx.blurring_height = ctx.expected_min_height / 2;
  r128_compute_uwidth_space(&ctx);
  
  if(ctx.nh)
    return r128_help(&ctx, argv[0], ctx.nh);
  
  if(!argv[optind])
  {
    fprintf(stderr, "Usage: %s [options] <filename> [<filename> ...]\n", argv[0]);
    fprintf(stderr, "Run %s -h to get help\n", argv[0]);
    return 1;
  }
  
  
  ctx.n_images = argc - optind;
  ctx.im = (struct r128_image*) r128_zalloc(&ctx, sizeof(struct r128_image) * ctx.n_images);
  ctx.im_blurred = (struct r128_image*) r128_zalloc(&ctx, sizeof(struct r128_image) * ctx.n_images);
  
  for(i = 0; i<ctx.n_images; i++)
  {
    assert((ctx.im[i].filename = strdup(argv[optind + i])));

    ctx.im[i].best_rc = R128_EC_NOTHING;
    ctx.im[i].fd = -1;

    ctx.im[i].bestcode = r128_malloc(&ctx, R128_EXP_CODE_MAX);
    ctx.im[i].bestcode_alloc = R128_EXP_CODE_MAX;
  }
  
  r128_log(&ctx, R128_DEBUG1, "Command line arguments parsed. Processing starts.\n");
  for(i = 0; i<ctx.n_images; i += ctx.batch_size)
  {
    int j, batch_size = ctx.batch_size;
    
    if((i + batch_size) > ctx.n_images)
      batch_size = ctx.n_images - i;

    r128_log(&ctx, R128_DEBUG1, "Starting processing of batch %d.\n", i / ctx.batch_size);
    ctx.batch_start = r128_time(&ctx);
    
    ctx.min_width = 65536;
    ctx.min_height = 65536;

    ctx.max_width = 0;
    ctx.max_height = 0;
    
    /* Load all images for this batch */
    for(j = 0; j<batch_size; j++)
    {
      struct r128_image * im = &ctx.im[i + j];
      im->best_rc = minrc(r128_load_file(&ctx, im), im->best_rc);

      if(!im->gray_data) continue;
      
      if(im->height > ctx.max_height) ctx.max_height = im->height;
      if(im->width > ctx.max_width) ctx.max_width = im->width;

      if(im->height < ctx.min_height) ctx.min_height = im->height;
      if(im->width < ctx.min_width) ctx.min_width = im->width;
    }
    
    /* Find the CODES! */
    ctx.n_codes_found += r128_run_strategy(&ctx, ctx.strategy, i, batch_size);

    /* Free all images of this batch */
    for(j = 0; j<batch_size; j++)
    {
      r128_free_image(&ctx, &ctx.im[i + j]);
      r128_free_image(&ctx, &ctx.im_blurred[i + j]);
    }
    
    /* Delete all temporary files */
    if(!(ctx.flags & R128_FL_KEEPTEMPS))
    {
      for(j = ctx.first_temp; j<ctx.last_temp; j++)
      {
#warning arbitrary limit
        char tempname[4097];
        snprintf(tempname, 4096, "%s%08d.pgm", ctx.temp_prefix, j);
        if(unlink(tempname) == -1)
          r128_log(&ctx, R128_ERROR, "Cannot delete temporary file %s. unlink(): %s\n", tempname, strerror(errno));
      }
      ctx.first_temp = ctx.last_temp;  
    }

    /* Report batch finished */
    r128_log(&ctx, R128_DEBUG1, "Finished processing of batch %d. So far, %d images processed, %d codes found.\n",
      i / ctx.batch_size, i + batch_size, ctx.n_codes_found);
  }
  r128_log(&ctx, R128_DEBUG1, "All batches processed. Terminating.\n");
  
  return 0;
}
