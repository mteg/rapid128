
typedef u_int32_t ufloat8;

#define UF8_FRAC(x) ((x) & 0xff)
#define UF8_TOCEIL(x) (256 - UF8_FRAC(x))
#define UF8_INTFLOOR(x) ((x) >> 8)
#define UF8_INTCEIL(x) (UF8_INTFLOOR(x) + ((UF8_FRAC(x) == 0) ? 0 : 1))
#define INT_TO_UF8(x) ((x) << 8)
#define UF8_CEIL(x)  INT_TO_UF8(UF8_INTCEIL(x))
#define UF8_MUL(x, y) (((x) * (y)) >> 8)
#define UF8_FLOAT(x) (((double) x) / 256.0)
