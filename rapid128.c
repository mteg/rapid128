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
  c->def_threshold = 0.6;
  c->margin_low_threshold = 20;
  c->margin_high_threshold = 235;
  c->min_cuw_delta2 = 2;
  c->min_cuw_delta1 = 10;
  c->expected_min_height = 8;
  c->temp_prefix = "r128temp";
  c->batch_size = 1;
  c->rgb_channel = 4;

  c->codealloc = 128;
  c->codebuf = (u_int8_t*) r128_malloc(c, c->codealloc * sizeof(int));
}

int help(struct r128_ctx *ctx, char *progname, int vb)
{
  printf("rapid128 barcode scanner, (c) 2015 Mateusz 'mteg' Golicz\n");
  printf("Usage: %s [options] <file1> [<file2> [<file3> ...]]\n", progname);
  printf("\n");
  printf("Available options:\n");
  printf(" -h         Print help and quit\n");
  printf(" -hh        Print extended help and quit\n");
  printf("\n");
  printf("BASIC SCANNER PARAMETERS\n");
  printf(" -wa <pix>  Minimum expected width of thinnest bar in pixels [%.1f]\n", ctx->min_uwidth);
  printf(" -wb <pix>  Maximum expected width of thinnest bar in pixels [%.1f]\n", ctx->max_uwidth);
  printf(" -mh <lvl>  High (white) threshold for cutting margins [%d]\n", ctx->margin_high_threshold);
  printf(" -ml <lvl>  Low (black) threshold for cutting margins [%d]\n", ctx->margin_low_threshold);
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
  printf(" -ch <pix>  Expected minimal code height [%d]\n", ctx->expected_min_height);
  printf(" -nb        Do not use blurring at all\n");
  printf(" -bh <pix>  Blurring height [%d]\n", ctx->blurring_height ? ctx->blurring_height : (ctx->expected_min_height / 2));
  printf(" -s  <strg> Configure scanning strategy\n"); 
  printf(" -da <unts> Minimum width scans resolution (pass 1 - uppercase H) [%.1f]\n", ctx->min_cuw_delta1);
  printf(" -db <unts> Minimum width scans resolution (pass 2 - lowercase h) [%.1f]\n", ctx->min_cuw_delta2);
  if(vb == 2)
  {
    printf("Expected minimal height affects speed, but not detection. In first pass, rapid128 only"
           " scans every -ch line of the image. Subsequent passes will then scan all lines. However, selecting"
           " a good -ch will drastically increase speed of detection in case of clearly preserved codes.\n");
    printf("In one of the later passes, rapid128 applies a vertical blur to the image and repeats the scan. "
           "This sometimes improves situation in presence of noise (especially fading toner etc.). Use -bh to "
           "specify blurring range (default is half of expected code width). Use -nb in case your codes are "
           " likely to be slightly rotated - vertical blur will only make situation worse\n");
    printf("Scanning strategies are explained at the end of this help.\n");
  }

  printf("\n");
  printf("IMAGE INTERPRETATION\n");
  printf(" -t <float> Threshold [%.2f].\n", ctx->threshold);
  printf(" -r         Codes are not horizontal and oriented left to right, but rather rotated by 90 degress clockwise (use twice/three times to indicate 180/270)\n");
  printf(" -ic R/G/B  Use only the indicated channel of RGB images\n");
  if(vb == 2)
  {
    printf("By default, rapid128 only scans for codes placed horizontally, with normal left to right orientation. "
           "Use -r to scan codes oriented from top to bottom (90 degree rotation), -rr to scan codes from pages rotated 180 degrees"
           "or -rrr to scan codes rotated 270 degrees (bottom to top).\n"
           "Note that rapid128 cheats in order to save time: rotations by 180 and 270 are implemented as flips of 0 and 90.\n"
           "All processing in rapid128 takes place in grayscale. In case a P6 (RGB) image is provided as input, it will "
           "be reduced to grayscale first. The -ic option is used to select only one channel from RGB of images. "
           "For example, if you have scans of documents with handwritten signatures likely to be in blue pen, use -ic R or -ic G "
           "(channels other than blue) - this will improve detection of codes accidentaly overwritten by handwritten signatures.\n"
           "By default, RGB images are converted to grayscale by averaging all three channels (R + G + B) / 3.0. Note that "
           "-ic does not affect processing grayscale or black-and-white inputs at all\n"
           "Use -t to alter decision threshold for classifying pixels as black (1) or white (0). Use negative threshold for "
           "inverted images. Use threshold larger than 0.5 for images that are a bit too white (fading toner etc) "
           "and smaller than 0.5 for images that are a bit too black (after processing by a 'blackening' copy machine)\n"
           "If you would like to scan various orientations and/or thresholds in sequence, define your own scanning strategy "
           "(-s). The -t and -r options will then be applied only for first tactics, until instructions regarding rotation/orientation are found in the strategy.\n"
           
           );
  }

  printf("\n");
  printf("BATCH PROCESSING\n");
  printf(" -bs <cnt>  Batch size - number of files processed at once [%d]\n", ctx->batch_size);
  printf(" -bt <secs> Time limit for each batch [%.1f]\n", ctx->batch_limit);
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
  printf(" -lc <cmd>  Set loader command (spacebar + file name is appended at the end) [%s]\n", ctx->loader);
  printf(" -lt <secs> Time limit for the loader command [%.1f]\n", ctx->loader_limit);
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
  printf(" -mp <pfx>  Prefix for temporary files (NNNNNN.pgm is appended) [%s]\n", ctx->temp_prefix); 
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
    
    printf("\n"
           "ON SCANNING STRATEGIES\n"
           "Rapid128 scans barcodes using a brute force approach. There are couple of variables "
           "that need to be guessed to successfuly read a barcode: (1) narrowest bar width, "
           "(2) vertical position of the code, (3) horizontal offset of the beginning of the barcode "
           "(in fractions of bar width). Rapid128 tries to guess all of these variables by bisecting "
           "the search range, while also employing a breadth first search strategy. \n"
           "In order to speed up detection, configuration space for each of the three variables gets subdivided "
           "into two parts: coarse and fine. Scanning strategy tells rapid128 which parts of the configuration "
           "space explore first. It usually makes a lot of sense to do a coarse brute force first: for example, "
           "coarse vertical position space consists of only 1/<expected_min_barcode_height> lines that are spaced "
           "apart vertically by <expected_min_barcode_height>. If we are dealing with a clean, undamaged scan, "
           "it is sufficient to analyze only this small percentage of lines - one of them will contain the code anyway.\n"
           "Additionaly, scanning strategy can tell rapid128 to try different orientations of the image or thresholds.\n"
           "Currently configured scanning strategy is: %s\n"
           "Scanning strategy consists of comma-separated search instructions, called 'tactics'. These instructions select "
           "parts of the complete configuration space to consider and are executed sequentially for every batch of images. "
           "Every tactis is a concatenated string of the following letters:\n"
           " W or w   Select coarse or fine smallest bar width search.\n"
           " H or h   Select coarse or fine vertical position search.\n"
           " O or o   Select coarse or fine horizontal offsets search.\n"
           " I or i   Use normal or vertically blurred image.\n"
           "Additionally, it is possible to specify a threshold (eg. '@0.5') or rotation (eg. ':rr') by appending "
           "extra tags at the end of configuration space selectors. The selected rotation or threshold holds " 
           "for all subsequent tactics, until another rotation or threshold is configured.\n"
           "Please note that 'o' and 'w' parts of the configuration space make sense only in "
           "case of very wide and (possibly) damaged codes (smallest bar more than 3 - 5 pixels).\n"
           "Please consider using the following strategies:\n"
           "- default - for best detection, worst speed\n"
           "- IWHO,IWHO:r,IWhO:,IWhO:r  or similar - when not sure whether code will be horizontal or vertical (use rr and rrr for 180 and 270 guesses)\n"
           "- IWHO,IWhO,iWHO,iWhO   - for narrow codes, when speed is important and input material is of good quality\n"
           "- IWHO,IWhO   - when in need for very fast operation and not a lot of concern about undetected codes in low quality input.\n", ctx->strategy)  ;
  }
  

  return 0;
}


int main(int argc, char ** argv)
{
  struct r128_ctx ctx;
  int c, nh = 0, i;

  r128_defaults(&ctx);
  clock_gettime(CLOCK_MONOTONIC, &ctx.startup);
  
  while((c = getopt(argc, argv, "abcdehilmnqrvs:t:w")) != EOF)
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
      case 'i':
        switch((c = getopt(argc, argv, "c:")))
        {
          case 'c': 
            if(strlen(optarg) != 1)
              r128_fail(&ctx, "Invalid image channel specification: %s, use R, G or B\n", optarg);
              
            if(*optarg == 'R')
              ctx.rgb_channel = 0;
            else if(*optarg == 'G')
              ctx.rgb_channel = 1;
            else if(*optarg == 'B')
              ctx.rgb_channel = 2;
            else
              r128_fail(&ctx, "Invalid image channel specification: %s, use R, G or B\n", optarg);            

            break;
          
          default:  r128_fail(&ctx, "Unknown option: -i%c\n", c); break;
        }
        break;
        
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
      case 'r':
        ctx.def_rotation++;
        ctx.def_rotation = ctx.def_rotation % 4;
        break;
      case 's': assert((ctx.strategy = strdup(optarg))); break;
      case 't':
        ctx.def_threshold = floatopt(&ctx, "-t", optarg);
        break;
        
      case 'v': ctx.logging_level++; break;        
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
    return help(&ctx, argv[0], nh);
  
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
