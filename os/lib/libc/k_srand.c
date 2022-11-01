#include <chinos/config.h>

#include <k_stddef.h>
#include <k_limits.h>

#include <k_stdlib.h>

typedef float        float_t;
typedef double       double_t;

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* First, second, and thired order congruential generators are supported */

#ifndef CONFIG_LIB_RAND_ORDER
#  define CONFIG_LIB_RAND_ORDER 1
#endif

#if CONFIG_LIB_RAND_ORDER > 3
#  undef CONFIG_LIB_RAND_ORDER
#  define CONFIG_LIB_RAND_ORDER 3
#endif

/* Values needed by the random number generator */

#define RND1_CONSTK  470001
#define RND1_CONSTP  999563
#define RND2_CONSTK1 366528
#define RND2_CONSTK2 508531
#define RND2_CONSTP  998917
#define RND3_CONSTK1 360137
#define RND3_CONSTK2 519815
#define RND3_CONSTK3 616087
#define RND3_CONSTP  997783

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/* First order congruential generators */

static inline unsigned long fgenerate1(void);
#if (CONFIG_LIB_RAND_ORDER == 1)
static double_t frand1(void);
#endif

/* Second order congruential generators */

#if (CONFIG_LIB_RAND_ORDER > 1)
static inline unsigned long fgenerate2(void);
#if (CONFIG_LIB_RAND_ORDER == 2)
static double_t frand2(void);
#endif

/* Third order congruential generators */

#if (CONFIG_LIB_RAND_ORDER > 2)
static inline unsigned long fgenerate3(void);
static double_t frand3(void);
#endif
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static unsigned long g_randint1;
#if (CONFIG_LIB_RAND_ORDER > 1)
static unsigned long g_randint2;
#if (CONFIG_LIB_RAND_ORDER > 2)
static unsigned long g_randint3;
#endif
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* First order congruential generators */

static inline unsigned long fgenerate1(void)
{
  unsigned long randint;

  /* First order congruential generator.  One may be added to the result of the
   * generated value to avoid the value zero.  This would be fatal for the
   * first order random number generator.
   */

  randint    = (RND1_CONSTK * g_randint1) % RND1_CONSTP;
  g_randint1 = (randint == 0 ? 1 : randint);
  return randint;
}

#if (CONFIG_LIB_RAND_ORDER == 1)
static double_t frand1(void)
{
  /* First order congruential generator. */

  unsigned long randint = fgenerate1();

  /* Construct an floating point value in the range from 0.0 up to 1.0 */

  return ((double_t)randint) / ((double_t)RND1_CONSTP);
}
#endif

/* Second order congruential generators */

#if (CONFIG_LIB_RAND_ORDER > 1)
static inline unsigned long fgenerate2(void)
{
  unsigned long randint;

  /* Second order congruential generator. */

  randint    = (RND2_CONSTK1 * g_randint1 +
                RND2_CONSTK2 * g_randint2) % RND2_CONSTP;

  g_randint2 = g_randint1;
  g_randint1 = randint;

  /* We cannot permit both values to become zero.  That would be fatal for the
   * second order random number generator.
   */

  if (g_randint2 == 0 && g_randint1 == 0)
    {
      g_randint2 = 1;
    }

  return randint;
}

#if (CONFIG_LIB_RAND_ORDER == 2)
static double_t frand2(void)
{
  /* Second order congruential generator */

  unsigned long randint = fgenerate2();

  /* Construct an floating point value in the range from 0.0 up to 1.0 */

  return ((double_t)randint) / ((double_t)RND2_CONSTP);
}
#endif

/* Third order congruential generators */

#if (CONFIG_LIB_RAND_ORDER > 2)
static inline unsigned long fgenerate3(void)
{
  unsigned long randint;

  /* Third order congruential generator. */

  randint    = (RND3_CONSTK1 * g_randint1 +
                RND3_CONSTK2 * g_randint2 +
                RND3_CONSTK2 * g_randint3) % RND3_CONSTP;

  g_randint3 = g_randint2;
  g_randint2 = g_randint1;
  g_randint1 = randint;

  /* We cannot permit all three values to become zero.  That would be fatal for the
   * third order random number generator.
   */

  if (g_randint3 == 0 && g_randint2 == 0 && g_randint1 == 0)
    {
      g_randint3 = 1;
    }

  return randint;
}

static double_t frand3(void)
{
  /* Third order congruential generator */

  unsigned long randint = fgenerate3();

  /* Construct an floating point value in the range from 0.0 up to 1.0 */

  return ((double_t)randint) / ((double_t)RND3_CONSTP);
}
#endif
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: srand
 *
 * Description:
 *   Seed the congruential random number generator.
 *
 ****************************************************************************/

void srand(unsigned int seed)
{
  g_randint1 = seed;
#if (CONFIG_LIB_RAND_ORDER > 1)
  g_randint2 = seed;
  (void)fgenerate1();
#if (CONFIG_LIB_RAND_ORDER > 2)
  g_randint3 = seed;
  (void)fgenerate2();
#endif
#endif
}

/****************************************************************************
 * Name: nrand
 *
 * Description:
 *   Return a random, unsigned long value in the range of 0 to (limit - 1)
 *
 ****************************************************************************/

unsigned long nrand(unsigned long limit)
{
  unsigned long result;
  double_t ratio;

  /* Loop to be sure a legal random number is generated */

  do
    {
      /* Get a random integer in the range 0.0 - 1.0 */

#if (CONFIG_LIB_RAND_ORDER == 1)
      ratio = frand1();
#elif (CONFIG_LIB_RAND_ORDER == 2)
      ratio = frand2();
#else /* if (CONFIG_LIB_RAND_ORDER > 2) */
      ratio = frand3();
#endif

      /* Then, produce the return-able value in the requested range */

      result = (unsigned long)(((double_t)limit) * ratio);

      /* Loop because there is an (unlikely) possibility that rounding
       * could increase the result at the limit value about the limit.
       */
    }
  while (result >= limit);

  return result;
}

int rand(void)
{
	return (int)nrand(INT_MAX);
}

long random(void)
{
	return (long)nrand(LONG_MAX);
}
