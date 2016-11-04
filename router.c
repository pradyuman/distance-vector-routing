#include <stdio.h>

#include "ne.h"
#include "router.h"

int main(int argc, char **argv) {
  if (argc < 5) {
    fprintf(stderr, "USAGE ./router <router id> <ne hostname> <ne UDP port> <router UDP port>");
    return 1;
  }
}
