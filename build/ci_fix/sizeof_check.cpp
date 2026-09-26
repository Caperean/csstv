#include "pd/pd.h"
#include <cstdio>
int main() {
  std::printf("Driver=%zu ModeParams=%zu pixels=%zu chunk=%zu storage=%zu\n",
    sizeof(csstv::pd::Driver),
    sizeof(csstv::pd::ModeParams),
    size_t(8*4*3),
    size_t(64*2),
    sizeof(csstv::pd::Driver));
  size_t ram = sizeof(csstv::pd::Driver) + 8*4*3 + 64*2 + 2;
  std::printf("approx .bss=%zu (avr budget 1024)\n", ram);
}
