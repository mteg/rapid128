#define READBITS_TH_STEPS 8
#define unlikely(x)     __builtin_expect((x),0)
#define likely(x)     __builtin_expect((x),1)
#define findbits_return(code) { if(*curpos) *curpos = npos; *threshold = ths[z]; return code; }