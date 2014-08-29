/* Testing Code */

#include <limits.h>
int test_bang(int x)
{
  return !x;
}
int test_bitCount(int x) {
  int result = 0;
  int i;
  for (i = 0; i < 32; i++)
    result += (x >> i) & 0x1;
  return result;
}
int test_copyLSB(int x)
{
  return (x & 0x1) ? -1 : 0;
}
int test_divpwr2(int x, int n)
{
    int p2n = 1<<n;
    return x/p2n;
}







int test_evenBits(void) {
  int result = 0;
  int i;
  for (i = 0; i < 32; i+=2)
    result |= 1<<i;
  return result;
}
int test_fitsBits(int x, int n)
{
  int TMin_n = -(1 << (n-1));
  int TMax_n = (1 << (n-1)) - 1;
  return x >= TMin_n && x <= TMax_n;
}
int test_getByte(int x, int n)
{
  union {
    int word;
    unsigned char bytes[4];
  } u;
  int test = 1;
  int littleEndian = (int) *(char *) &test;
  u.word = x;
  return littleEndian ? (unsigned) u.bytes[n] : (unsigned) u.bytes[3-n];
}
int test_isGreater(int x, int y)
{
  return x > y;
}
int test_isNonNegative(int x) {
  return x >= 0;
}
int test_isNotEqual(int x, int y)
{
  return x != y;
}
int test_isPower2(int x) {
  int i;
  for (i = 0; i < 31; i++) {
    if (x == 1<<i)
      return 1;
  }
  return 0;
}
int test_leastBitPos(int x) {
  int mask = 1;
  if (x == 0)
    return 0;
  while (!(mask & x)) {
    mask = mask << 1;
  }
  return mask;
}
int test_logicalShift(int x, int n) {
  unsigned u = (unsigned) x;
  unsigned shifted = u >> n;
  return (int) shifted;
}
int test_satAdd(int x, int y)
{
  if (x > 0 && y > 0 && x+y < 0)
    return LONG_MAX;
  if (x < 0 && y < 0 && x+y >= 0)
    return LONG_MIN;
  return x + y;
}
int test_tc2sm(int x) {
  int sign = x < 0;
  int mag = x < 0 ? -x : x;
  return (sign << 31) | mag;
}
