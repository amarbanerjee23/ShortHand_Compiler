#include <stdio.h>
int main(void) {
  int n = 0, s = 7;
  if (scanf("%d", &n) != 1) return 1;
  for (int i = 0; i < n; ++i) s = (s * 31 + i) % 1000003;
  printf("%d\n", s);
  return 0;
}
