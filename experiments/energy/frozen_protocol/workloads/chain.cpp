#include <cstdio>
int main() {
  int n = 0, s = 7;
  if (std::scanf("%d", &n) != 1) return 1;
  for (int i = 0; i < n; ++i) s = (s * 31 + i) % 1000003;
  std::printf("%d\n", s);
  return 0;
}
