#include "csstv.h"
#if !CSSTV_ENABLE_ENCODER
#error encoder should be auto-enabled
#endif
#if CSSTV_ENABLE_DECODER
#error decoder should stay off
#endif
int main(){return 0;}
