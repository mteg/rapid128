#include <stdio.h>
#include <sys/types.h>
#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include "rapid128.h"

inline static ufloat8 ufloat8opt(struct r128_ctx *ctx, char *opt, char *val)
{
  double res;
  char *eptr;
  res = strtod(val, &eptr);
  if(*eptr)
    r128_fail(ctx, "Invalid value for %s: %s\n", opt, val);
  
  return lrint(res * 256.0);
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
  clock_gettime(CLOCK_MONOTONIC, &c->startup);
  c->strategy = "IHWO@0.6,iHWO,IHWo,iHWo,IHwO,iHwO,IHwo,iHwo,IhWO,ihWO,IhWo,ihWo,IhwO,ihwO,Ihwo,ihwo";
  c->min_uwidth = 230;	/* 0.9 */
  c->max_uwidth = 4 * 256;
  c->margin_low_threshold = 20;
  c->margin_high_threshold = 235;
/*
  c->uw_delta1 = 1.0;
  c->uw_delta2 = 0.25;
*/
  c->expected_min_height = 8;
  c->temp_prefix = "r128temp";
  c->batch_size = 1;
  c->rgb_channel = 4;

  c->codealloc = 128;
  c->codebuf = (u_int8_t*) r128_malloc(c, c->codealloc * sizeof(int));
  
  c->say = (void (*)(void *, char*)) fprintf;
  c->say_arg = stderr;
  
  c->report_code = r128_report_code;
}

int r128_help(struct r128_ctx *ctx, const char *progname, int vb)
{
  printf("rapid128 barcode scanner, (c) 2015 Mateusz 'mteg' Golicz\n");
  printf("Usage: %s [options] <file1> [<file2> [<file3> ...]]\n", progname);
  if(vb == 2)
    printf("Use '-' as a file name to read from stdin\n");
  printf("\n");
  printf("Available options:\n");
  printf(" -h         Print help and quit\n");
  printf(" -hh        Print extended help and quit\n");
  printf(" -V         Print version and quit\n");
  printf("\n");
  printf("BASIC SCANNER PARAMETERS\n");
  printf(" -wa <pix>  Minimum expected width of thinnest bar in pixels [%.1f]\n", UF8_FLOAT(ctx->min_uwidth));
  printf(" -wb <pix>  Maximum expected width of thinnest bar in pixels [%.1f]\n", UF8_FLOAT(ctx->max_uwidth));
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
  printf(" -bh <pix>  Blurring height [%d]\n", ctx->blurring_height);
  printf(" -bx / -by  Blur in X or Y axis (default: Y)\n");
  printf(" -bS / -bL  Blur along shorter or longer edge (only one of -bx, -by, -bS or -bL makes sense)\n");
  printf(" -s  <strg> Configure scanning strategy\n"); 
  printf(" -u1 <stpc> Unit width search step count (pass 1 - uppercase W) [%d]\n", ctx->uw_steps1);
  printf(" -u2 <stpc> Unit width search step count (pass 2 - lowercase w) [%d]\n", ctx->uw_steps2);
  if(vb == 2)
  {
    printf("Expected minimal height affects speed, but not detection. In first pass, rapid128 only"
           " scans every -ch line of the image. Subsequent passes will then scan all lines. However, selecting"
           " a good -ch will drastically increase speed of detection in case of clearly preserved codes.\n");
    printf("In one of the later passes, rapid128 applies a vertical blur to the image and repeats the scan. "
           "This sometimes improves situation in presence of noise (especially fading toner etc.). Use -bh to "
           "specify blurring range (default is half of expected code width). Use -nb in case your codes are "
           " likely to be slightly rotated - vertical blur will only make situation worse\n");
    printf("The -u1 and -u2 options define number of steps in which to guess the thinnest bar size. "
           "By default, they are computed to match -wa and -wb. You can alter (lower) the precomputed values to "
           "gain some speed at the cost of detection rate\n");
    printf("Scanning strategies are explained at the end of this help.\n");
  }

  printf("\n");
  printf("IMAGE INTERPRETATION\n");
  printf(" -r         Codes are not horizontal and oriented left to right, but rather rotated by 90 degress clockwise (use twice/three times to indicate 180/270)\n");
  printf(" -ps        Assume codes are in parallel to shorter edges of images (ie. horizontal on portrait pages or vertical on landscape pages). Do not process images in tactics using rotations not matching this scheme (eg. skip processing 0 deg rotations for an image when its width > height).\n");
  printf(" -pl        Same, but assume codes are parallel to longer edges\n");
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
  printf(" -bt <secs> Time limit for each batch in ms [%d]\n", ctx->batch_limit);
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
  printf(" -lc <cmd>  Set loader command [%s]\n", ctx->loader ? ctx->loader : "");
  printf(" -lt <secs> Time limit for the loader command in ms [%d]\n", ctx->loader_limit);
  if(vb == 2)
  {
    printf("Normally, rapid128 processes only PGM files. To load other file formats, a loader command"
           " must be specified. The loader is executed for every file provided after the option list."
           "Use two dollars ($$) in place where the file name should be inserted. If no two dollars"
           " are present in the loader command, the file name will be appended at the end. The file name is escaped.\n");
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
           "(in fractions of bar width). "
           "In order to speed up detection, configuration space for each of the three variables gets subdivided "
           "into two parts: coarse and fine. Scanning strategy tells rapid128 which parts of the configuration "
           "space explore first. It usually makes a lot of sense to do a coarse brute force first in the first place: for example, "
           "coarse vertical position space consists of only 1/<expected_min_barcode_height> lines that are spaced "
           "apart vertically by <expected_min_barcode_height>. If we are dealing with a clean, undamaged scan, "
           "it is sufficient to analyze only this small percentage of lines - one of them will contain the code anyway.\n"
           "Additionaly, scanning strategy can tell rapid128 to try different orientations of the image or thresholds.\n"
           "Currently configured scanning strategy is: %s\n"
           "Scanning strategy consists of comma-separated search instructions, called 'tactics'. These instructions select "
           "parts of the complete configuration space to consider and are executed sequentially for every batch of images. "
           "Every tactis is a concatenated string of the following letters:\n"
           " W or w   Select coarse (-u1) or fine (-u2) smallest bar width search.\n"
           " H or h   Select coarse (-ch) or fine (every line) vertical position search.\n"
           " O or o   Select coarse (0, 0.5, 0.25, 0.75 times smallest bar width) or fine (every 1/8th) horizontal offset search.\n"
           " I or i   Use normal or vertically blurred image.\n"
           "Additionally, it is possible to specify a threshold (eg. '@0.5') or rotation (eg. '/rr') by appending "
           "extra tags at the end of configuration space selectors. The selected rotation or threshold holds " 
           "for all subsequent tactics, until another rotation or threshold is configured.\n"
           "Please note that 'o' and 'w' parts of the configuration space make sense only in "
           "case of very wide and (possibly) damaged codes (smallest bar more than 3 - 5 pixels).\n"
           "Please consider using the following strategies:\n"
           "- default - for best detection of horizontal, left to right codes; worst speed\n"
           "- IWHO,IWHO/r,IWhO/,IWhO/r  or similar - when not sure whether code will be horizontal or vertical (use rr and rrr for 180 and 270 guesses)\n"
           "- IWHO,IWhO,iWHO,iWhO   - for narrow codes, when speed is important and input material is of good quality\n"
           "- IWHO,IWhO   - when in need for very fast operation and not a lot of concern about undetected codes in low quality input.\n", ctx->strategy)  ;
  }
  

  return 0;
}

int r128_getopt(struct r128_ctx *c, int argc, char ** argv)
{
  int opt;
  opt = getopt(argc, argv, "abcehilmnpqrvs:uwV");
  if(opt == EOF) return EOF;
  switch(opt)
  {
    case 'a': c->flags |= R128_FL_READALL; break;
    case 'b':
      switch((opt = getopt(argc, argv, "h:s:t:xySL")))
      {
        case 'h': c->blurring_height = intopt(c, "-bh", optarg); break;
        case 's': c->batch_size = intopt(c, "-bs", optarg); break;
        case 't': c->batch_limit = intopt(c, "-bt", optarg); break;
        case 'x': c->flags |= R128_FL_BLUR_X; break;
        case 'S': c->flags |= R128_FL_BLUR_SHORT; break;
        case 'L': c->flags |= R128_FL_BLUR_LONG; break;
        default:  r128_fail(c, "Unknown option: -b%c\n", c); break;
      }
      break;
    case 'c':
      switch((opt = getopt(argc, argv, "h:")))
      {
        case 'h': c->expected_min_height = intopt(c, "-ch", optarg); break;
        default:  r128_fail(c, "Unknown option: -c%c\n", opt); break;
      }
      break;
    
    case 'e': c->flags |= R128_FL_EREPORT; break;
    case 'h': c->nh++; break;
    case 'i':
      switch((opt = getopt(argc, argv, "c:")))
      {
        case 'c': 
          if(strlen(optarg) != 1)
            r128_fail(c, "Invalid image channel specification: %s, use R, G or B\n", optarg);
            
          if(*optarg == 'R')
            c->rgb_channel = 0;
          else if(*optarg == 'G')
            c->rgb_channel = 1;
          else if(*optarg == 'B')
            c->rgb_channel = 2;
          else
            r128_fail(c, "Invalid image channel specification: %s, use R, G or B\n", optarg);            
          break;
        
        default:  r128_fail(c, "Unknown option: -i%c\n", opt); break;
      }
      break;
    case 'l':
      switch((opt = getopt(argc, argv, "c:t:")))
      {
        case 'c': assert((c->loader = strdup(optarg))); break;
        case 't': c->loader_limit = intopt(c, "-lt", optarg); break;
        default:  r128_fail(c, "Unknown option: -l%c\n", opt); break;
      }
      break;
    case 'm':
      switch((opt = getopt(argc, argv, "map:h:l:")))
      {
        case 'm': c->flags |= R128_FL_MMAPALL; break;
        case 'a': c->flags |= R128_FL_RAMALL; break;
        case 'p': assert((c->temp_prefix = strdup(optarg))); break;
        case 'l': c->margin_low_threshold = intopt(c, "-ml", optarg); break;
        case 'h': c->margin_high_threshold = intopt(c, "-mh", optarg); break;
        default:  r128_fail(c, "Unknown option: -m%c\n", opt); break;
      }
      break;
    case 'n':
      switch((opt = getopt(argc, argv, "dmb")))
      {
        case 'd': c->flags |= R128_FL_KEEPTEMPS; break;
        case 'm': c->flags |= R128_FL_NOCROP; break;
        case 'b': c->flags |= R128_FL_NOBLUR; break;
      }
      break;
    case 'p':
      switch((opt = getopt(argc, argv, "sl")))
      {
        case 's': c->flags |= R128_FL_ALIGNMENT_SHORT; break;
        case 'l': c->flags |= R128_FL_ALIGNMENT_LONG; break;
      }
      break;
    
    case 'q': c->logging_level--; break;
    case 'r':
      c->def_rotation++;
      c->def_rotation = c->def_rotation % 4;
      break;
    case 's': assert((c->strategy = strdup(optarg))); break;
    case 'u':
      switch((opt = getopt(argc, argv, "1:2:")))
      {
        case '1': c->uw_steps1 = intopt(c, "-u1", optarg); break;
        case '2': c->uw_steps2 = intopt(c, "-u2", optarg); break;
        default:  r128_fail(c, "Unknown option: -u%c\n", opt); break;
      }
      break;
      
    case 'v': c->logging_level++; break;        
    case 'w':
      switch((opt = getopt(argc, argv, "a:b:")))
      {
        case 'a': c->min_uwidth = ufloat8opt(c, "-wa", optarg); break;
        case 'b': c->max_uwidth = ufloat8opt(c, "-wb", optarg); break;
        default:  r128_fail(c, "Unknown option: -w%c\n", opt); break;
      }
      break;
    case 'V':
#warning HARDCODED VERSION
      r128_fail(c, "rapid128 v0.9\n");
      break;
    default:
      r128_fail(c, "Unknown option: %c\n", opt);
      break;
  }
  return opt;
}
