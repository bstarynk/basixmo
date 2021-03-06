// file primes.c - around primes, useful in hash tables, etc...

/**   Copyright (C)  2016 Basile Starynkevitch

      BASIXMO is free software; you can redistribute it and/or modify
      it under the terms of the GNU General Public License as published by
      the Free Software Foundation; either version 3, or (at your option)
      any later version.

      BASIXMO is distributed in the hope that it will be useful,
      but WITHOUT ANY WARRANTY; without even the implied warranty of
      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
      GNU General Public License for more details.
      You should have received a copy of the GNU General Public License
      along with BASIXMO; see the file COPYING3.   If not see
      <http://www.gnu.org/licenses/>.
**/

// mark unlikely conditions to help optimization
#ifdef __GNUC__
#define BXO_UNLIKELY(P) __builtin_expect(!!(P),0)
#define BXO_LIKELY(P) !__builtin_expect(!(P),0)
#define BXO_UNUSED __attribute__((unused))
#define BXO_OPTIMIZEDFUN __attribute__((optimize("O2")))
#else
#define BXO_UNLIKELY(P) (P)
#define BXO_LIKELY(P) (P)
#define BXO_UNUSED
#define BXO_OPTIMIZEDFUN
#endif

#include <stdint.h>

// an array of primes, gotten with something similar to
//   /usr/games/primes 3  | awk '($1>p+p/9){print $1, ","; p=$1}' 
const int64_t bxo_primes_tab[] = {
  3LL, 5LL, 7LL, 11LL, 13LL, 17LL, 19LL, 23LL, 29LL, 37LL, 43LL, 53LL, 59LL,
  67LL, 79LL, 89LL, 101LL, 113LL,
  127LL, 149LL, 167LL, 191LL, 223LL, 251LL, 281LL, 313LL, 349LL, 389LL, 433LL,
  487LL, 547LL, 613LL,
  683LL, 761LL, 853LL, 953LL, 1061LL, 1181LL, 1319LL, 1471LL, 1637LL, 1823LL,
  2027LL, 2267LL,
  2521LL, 2803LL, 3119LL, 3467LL, 3853LL, 4283LL, 4759LL, 5297LL, 5897LL,
  6553LL, 7283LL, 8093LL,
  8999LL, 10007LL, 11119LL, 12373LL, 13751LL, 15287LL, 16987LL, 18899LL,
  21001LL, 23339LL,
  25933LL, 28817LL, 32027LL, 35591LL, 39551LL, 43951LL, 48847LL, 54277LL,
  60317LL, 67021LL,
  74471LL, 82757LL, 91957LL, 102181LL, 113537LL, 126173LL, 140197LL, 155777LL,
  173087LL,
  192319LL, 213713LL, 237467LL, 263863LL, 293201LL, 325781LL, 361979LL,
  402221LL, 446921LL,
  496579LL, 551767LL, 613097LL, 681221LL, 756919LL, 841063LL, 934517LL,
  1038383LL, 1153759LL,
  1281961LL, 1424407LL, 1582697LL, 1758553LL, 1953949LL, 2171077LL, 2412323LL,
  2680367LL,
  2978189LL, 3309107LL, 3676789LL, 4085339LL, 4539277LL, 5043653LL, 5604073LL,
  6226757LL,
  6918619LL, 7687387LL, 8541551LL, 9490631LL, 10545167LL, 11716879LL,
  13018757LL,
  14465291LL, 16072547LL, 17858389LL, 19842659LL, 22047401LL, 24497113LL,
  27219019LL,
  30243359LL, 33603743LL, 37337497LL, 41486111LL, 46095719LL, 51217477LL,
  56908337LL,
  63231499LL, 70257241LL, 78063641LL, 86737379LL, 96374881LL, 107083213LL,
  118981367LL,
  132201521LL, 146890631LL, 163211821LL, 181346479LL, 201496157LL,
  223884629LL,
  248760703LL, 276400823LL, 307112027LL, 341235667LL, 379150777LL,
  421278643LL,
  468087391LL, 520097111LL, 577885681LL, 642095213LL, 713439127LL,
  792710159LL,
  880789067LL, 978654533LL, 1087393949LL, 1208215531LL, 1342461719LL,
  1491624137LL,
  1657360153LL, 1841511311LL, 2046123679LL, 2273470799LL, 2526078691LL,
  2806754123LL,
  3118615693LL, 3465128567LL, 3850142869LL, 4277936557LL, 4753262879LL,
  5281403287LL,
  5868225889LL, 6520251019LL, 7244723357LL, 8049692639LL, 8944102939LL,
  9937892189LL,
  11042102507LL, 12269002787LL, 13632225331LL, 15146917109LL, 16829907907LL,
  18699897683LL, 20777664097LL, 23086293457LL, 25651437191LL, 28501596883LL,
  31668440987LL, 35187156661LL, 39096840751LL, 43440934181LL, 48267704657LL,
  53630782993LL, 59589758903LL, 66210843271LL, 73567603643LL, 81741781829LL,
  90824202037LL, 100915780061LL, 112128644521LL, 124587382807LL,
  138430425373LL,
  153811583771LL, 170901759761LL, 189890844179LL, 210989826869LL,
  234433140979LL,
  260481267763LL, 289423630891LL, 321581812103LL, 357313124579LL,
  397014582917LL,
  441127314403LL, 490141460453LL, 544601622727LL, 605112914149LL,
  672347682449LL,
  747052980503LL, 830058867229LL, 922287630259LL, 1024764033637LL,
  1138626704071LL,
};

const unsigned bxo_primes_num =
  sizeof (bxo_primes_tab) / sizeof (bxo_primes_tab[0]);

int64_t
bxo_prime_above (int64_t n)
{
  unsigned numprimes = sizeof (bxo_primes_tab) / sizeof (bxo_primes_tab[0]);
  int lo = 0, hi = numprimes;
  if (BXO_UNLIKELY (n >= bxo_primes_tab[numprimes - 1]))
    return 0;
  if (BXO_UNLIKELY (n < 2))
    return 2;
  while (lo + 6 < hi)
    {
      int md = (lo + hi) / 2;
      if (bxo_primes_tab[md] > n)
        hi = md;
      else
        lo = md;
    };
  if (hi < (int) numprimes - 1)
    hi++;
  if (hi < (int) numprimes - 1)
    hi++;
  for (int ix = lo; ix < hi; ix++)
    if (bxo_primes_tab[ix] > n)
      return bxo_primes_tab[ix];
  return 0;
}

int64_t
bxo_prime_below (int64_t n)
{
  unsigned numprimes = sizeof (bxo_primes_tab) / sizeof (bxo_primes_tab[0]);
  int lo = 0, hi = numprimes;
  if (BXO_UNLIKELY (n >= bxo_primes_tab[numprimes - 1]))
    return 0;
  if (BXO_UNLIKELY (n < 2))
    return 2;
  while (lo + 6 < hi)
    {
      int md = (lo + hi) / 2;
      if (bxo_primes_tab[md] > n)
        hi = md;
      else
        lo = md;
    };
  if (hi < (int) numprimes - 1)
    hi++;
  if (hi < (int) numprimes - 1)
    hi++;
  for (int ix = hi; ix >= 0; ix--)
    if (bxo_primes_tab[ix] < n)
      return bxo_primes_tab[ix];
  return 0;
}

/// eof primes.c
