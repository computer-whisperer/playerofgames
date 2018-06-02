#include <stdio.h>
#include <unistd.h>

int main() {
  for(int i = 0; i < 10; i++) {
    usleep(1000000);
    printf("ping\n");
    fflush(stdout);
  }
}
