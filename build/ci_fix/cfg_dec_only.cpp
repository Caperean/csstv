#include "csstv.h"
#if CSSTV_ENABLE_ENCODER
#error encoder should stay off
#endif
#if !CSSTV_ENABLE_DECODER
#error decoder should be auto-enabled
#endif
int main(){return 0;}
