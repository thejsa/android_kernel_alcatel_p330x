#include <linux/ppp-ioctl.h>

/*[BUGFIX]-Add-BEGIN by TCTNB.Dandan.Fang,09/24/2013,FR467914,*/
/*Failed to Connect PPTP with Encryption*/
/*porting from bug367411 of scribe5*/
#define        PPP_MTU         1500    /* Default MTU (size of Info field) */
#define        PPP_MAXMRU     65000   /* Largest MRU we allow */
/*[BUGFIX]-Add-END by TCTNB.Dandan.Fang*/
