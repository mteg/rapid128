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

inline static double floatopt(struct r128_ctx *ctx, char *opt, char *val)
{
  double res;
  char *eptr;
  res = strtod(val, &eptr);
  if(*eptr)
    r128_fail(ctx, "Invalid value for %s: %s\n", opt, val);
  return res;
}

inline static int intopt(struct r128_ctx *ctx, char *opt, char *val)
{
  int res;
  char *eptr;
  res = strtol(val, &eptr, 0);
  if(*eptr)
    r128_fail(ctx, "Invalid value for %s: %s\n", opt, val);
  return res;
}

void r128_defaults(struct r128_ctx *c)
{
  memset(c, 0, sizeof(struct r128_ctx));
  c->strategy = "IHWO@0.6,iHWO,IHWo,iHWo,IHwO,iHwO,IHwo,iHwo,IhWO,ihWO,IhWo,ihWo,IhwO,ihwO,Ihwo,ihwo";
  c->min_uwidth = 0.9;
  c->max_uwidth = 4;
  c->threshold = 0.5;
  c->margin_low_threshold = 20;
  c->margin_high_threshold = 235;
  c->min_cuw_delta2 = 2;
  c->min_cuw_delta1 = 10;
  c->expected_min_height = 8;
  c->temp_prefix = "r128temp";
  c->batch_size = 1;

  c->codealloc = 128;
  c->codebuf = (u_int8_t*) r128_malloc(c, c->codealloc * sizeof(int));
}

int help(char *progname, int vb)
{
  printf("rapid128 barcode scanner, (c) 2015 Mateusz 'mteg' Golicz\n");
  printf("Usage: %s [options] <file1> [<file2> [<file3> ...]]\n", progname);
  printf("\n");
  printf("Available options:\n");
  printf(" -h         Print help and quit\n");
  printf(" -hh        Print extended help and quit\n");
  printf("\n");
  printf("BASIC SCANNER PARAMETERS\n");
  printf(" -wa <pix>  Minimum expected width of thinnest bar in pixels\n");
  printf(" -wb <pix>  Maximum expected width of thinnest bar in pixels\n");
  printf(" -mh <lvl>  High (white) threshold for cutting margins\n");
  printf(" -ml <lvl>  Low (black) threshold for cutting margins\n");
  if(vb == 2)
  {
    printf("Please carefully select a good range for -wa / -wb. These options practically "
           "define a range of horizontal resolutions to try and thus affect both detection and performance. "
           "Barcodes sized so that their thinnest "
           "bars are outside the given range will not be detected. On the other hand, too wide "
           "range of expected horizontal resolutions will make rapid128 run much slower.\n");
    printf("Both range ends might be given as floating point values. Note that rapid128 can manage "
           " barcodes that are scanned at a resolution of less than 1 pixel per thinnest bar "
           "(down to approx. 0.7 - 0.9).\n");
  }
  printf("\n");
  printf("PERFORMANCE TUNING\n");
  printf(" -ch <pix>  Expected minimal code height\n");
  printf(" -nb        Do not use blurring at all\n");
  printf(" -bh <pix>  Blurring height\n");
  printf(" -s  <strg> Configure scanning strategy\n"); 
  printf(" -da <unts> Minimum width scans resolution (pass 1 - uppercase H)\n");
  printf(" -db <unts> Minimum width scans resolution (pass 2 - lowercase h)\n");
  if(vb == 2)
  {
    printf("Expected minimal height affects speed, but not detection. In first pass, rapid128 only"
           " scans every -ch line of the image. Subsequent passes will then scan all lines. However, selecting"
           " a good -ch will drastically increase speed of detection in case of clearly preserved codes.\n");
    printf("In one of the later passes, rapid128 applies a vertical blur to the image and repeats the scan. "
           "This sometimes improves situation in presence of noise (especially fading toner etc.). Use -bh to "
           "specify blurring range (default is half of expected code width). Use -nb in case your codes are "
           " likely to be slightly rotated - vertical blur will only make situation worse\n");
           
  }
  printf("\n");
  printf("BATCH PROCESSING\n");
  printf(" -bs <cnt>  Batch size - number of files processed at once\n");
  printf(" -bt <secs> Time limit for each batch\n");
  if(vb == 2)
  {
    printf("Without -bs, files are processed one by one. However, if a batch size is given with -bs, rapid128 "
           "will load and scan <cnt> files at once. Note that this may use a lot of RAM (unless -mm is given too). \n"
           "Parallel processing of multiple files makes sense especially with -bt option. If a time limit is "
           "specified, rapid128 will first detect codes in 'easiest' files, and then use all remaining time "
           "to detect codes in noisiest and most difficult files.\n");
  }
  printf("\n");
  printf("FILE INPUT\n");
  printf(" -lc <cmd>  Set loader command (spacebar + file name is appended at the end)\n");
  printf(" -lt <secs> Time limit for the loader command\n");
  if(vb == 2)
  {
    printf("Normally, rapid128 processes only PGM files. To load other file formats, a loader command"
           " must be specified. The loader is executed for every file provided after the option list."
           "Use two dollars ($$) in place where the file name should be inserted. If no two dollars"
           " are present in the loader command, the file name will be appended at the end\n");
    printf("Example 1: -lc 'pdfimages -f 0 -l 0 -j $$ a ; jpegtran -rotate 270 a-000.jpg | jpegtopnm -dct fast'\n");
    printf("Example 2: -lc '/usr/bin/gs -dSAFER -dBATCH -dQUIET -dNOPAUSE -sDEVICE=pgmraw -sOutputFile=- -dTextAlphaBits=4 -dGraphicsAlphaBits=4 -dLastPage=1 -r300 '\n");
    
  }
  printf("\n");
  printf("MEMORY MANAGEMENT\n");
  printf(" -mm        Save RAM: write to temporary files and mmap() everything\n");
  printf(" -mp <pfx>  Prefix for temporary files (NNNNNN.pgm is appended)\n"); 
  printf(" -ma        Never mmap() anything, load all into RAM\n");
  if(vb == 2)
  {
    printf("By default, to process a grayscale PNM file, rapid128 uses mmap() instead of loading it into RAM."
           " Everything else is placed in RAM: conversions from P4 or P6 (RGB) files, output from loader, as well as blurred "
            "versions of images. The -ma or -mm options switch between two opposite ends of this compromise. "
            "Specifying -mm will cause rapid128 to save RAM and use temporary files for all intermediate "
            "images. These files will then be mmap()ed for processing. This makes sense with very large "
            "batches and a SSD drive. On the other hand, -ma will cause rapid128 to load and buffer everything "
            "in RAM and abstain from any use of mmap()\n");
  }
  printf("\n");
  printf("REPORTING AND DEBUGGING\n");
  printf(" -a         Continue scanning the document even after a valid code is found.\n");
  printf(" -e         Extended reporting: print file name, tactics, time spent, best result, code symbols.\n");
  printf(" -v         Increase out-of-band verbosity level\n");
  printf(" -q         Decrease out-of-band verbosity level\n");
  printf(" -nd        Do not delete temporary files (use with -mm)\n");
  printf(" -nm        Do not crop lines\n");
  if(vb == 2)
  {
    printf("rapid128 is designed for images containing just ONE barcode. As soon as a valid code is found in an image, "
           "processing of that image is stopped and rapid128 proceeds to next images. Use -a to continue processing the "
           "file even if a barcode has already been found. Note that codes will be reported many, many times, since "
           "no detection of distinct codes is provided.\n");
    printf("By default, rapid128 shaves groups of white or black pixels off left and right edge of every line. "
           "For the purpose of this operation, black and white levels are controlled by -mh / -ml (0 - 255). "
           "Use -nm to disable this feature. In most cases, however, this will only decrease performance and "
           "not affect detection in any way. If a code is detected with -nm that was not detected otherwise, it"
           " is most probably a bug\n");
  
  }
  return 0;
}


int main(int argc, char ** argv)
{
  struct r128_ctx ctx;
  int c, nh = 0, i;

  r128_defaults(&ctx);
  clock_gettime(CLOCK_MONOTONIC, &ctx.startup);
  
  while((c = getopt(argc, argv, "abcdehlmnqvs:w")) != EOF)
  {
    switch(c)
    {
      case 'a': ctx.flags |= R128_FL_READALL; break;
      case 'b':
        switch((c = getopt(argc, argv, "h:s:t:")))
        {
          case 'h': ctx.blurring_height = intopt(&ctx, "-bh", optarg); break;
          case 's': ctx.batch_size = intopt(&ctx, "-bs", optarg); break;
          case 't': ctx.batch_limit = floatopt(&ctx, "-bt", optarg); break;
          default:  r128_fail(&ctx, "Unknown option: -b%c\n", c); break;
        }
        break;
      case 'c':
        switch((c = getopt(argc, argv, "h:")))
        {
          case 'h': ctx.expected_min_height = intopt(&ctx, "-ch", optarg); break;
          default:  r128_fail(&ctx, "Unknown option: -c%c\n", c); break;
        }
        break;
      case 'd':
        switch((c = getopt(argc, argv, "a:b:")))
        {
          case 'a': ctx.min_cuw_delta1 = floatopt(&ctx, "-da", optarg); break;
          case 'b': ctx.min_cuw_delta2 = floatopt(&ctx, "-db", optarg); break;
          default:  r128_fail(&ctx, "Unknown option: -d%c\n", c); break;
        }
        break;
      
      case 'e': ctx.flags |= R128_FL_EREPORT; break;
      case 'h': nh++; break;
      case 'l':
        switch((c = getopt(argc, argv, "c:t:")))
        {
          case 'c': assert((ctx.loader = strdup(optarg))); break;
          case 't': ctx.loader_limit = floatopt(&ctx, "-lt", optarg); break;
          default:  r128_fail(&ctx, "Unknown option: -l%c\n", c); break;
        }
        break;
      case 'm':
        switch((c = getopt(argc, argv, "map:h:l:")))
        {
          case 'm': ctx.flags |= R128_FL_MMAPALL; break;
          case 'a': ctx.flags |= R128_FL_RAMALL; break;
          case 'p': assert((ctx.temp_prefix = strdup(optarg))); break;
          case 'l': ctx.margin_low_threshold = floatopt(&ctx, "-ml", optarg); break;
          case 'h': ctx.margin_high_threshold = floatopt(&ctx, "-mh", optarg); break;
          default:  r128_fail(&ctx, "Unknown option: -m%c\n", c); break;
        }
        break;
      case 'n':
        switch((c = getopt(argc, argv, "dmb")))
        {
          case 'd': ctx.flags |= R128_FL_KEEPTEMPS; break;
          case 'm': ctx.flags |= R128_FL_NOCROP; break;
          case 'b': ctx.flags |= R128_FL_NOBLUR; break;
        }
        break;
      case 'q': ctx.logging_level--; break;
      case 'v': ctx.logging_level++; break;        
      case 's': assert((ctx.strategy = strdup(optarg))); break;
      case 'w':
        switch((c = getopt(argc, argv, "a:b:")))
        {
          case 'a': ctx.min_uwidth = floatopt(&ctx, "-wa", optarg); break;
          case 'b': ctx.max_uwidth = floatopt(&ctx, "-wb", optarg); break;
          default:  r128_fail(&ctx, "Unknown option: -w%c\n", c); break;
        }
        break;
      default:
        r128_fail(&ctx, "Unknown option: %c\n", c);
        break;
    }
  }
  
  if(nh)
    return help(argv[0], nh);
  
  if(!argv[optind])
  {
    fprintf(stderr, "Usage: %s [options] <file name>.pgm\n", argv[0]);
    fprintf(stderr, "Run %s -h to get help\n", argv[0]);
    return 1;
  }
  
  if(ctx.batch_size < 1) ctx.batch_size = 1;
  if(ctx.blurring_height == 0)
    ctx.blurring_height = ctx.expected_min_height / 2;
  
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
