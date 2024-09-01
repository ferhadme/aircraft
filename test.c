#include <stdio.h>

struct {
  int a;
} test;

int main(void)
{
  test.a = 0;
  printf("A: %d\n", test.a);
}
