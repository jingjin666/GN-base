#ifndef __UAPI_SYSCALL_H
#define __UAPI_SYSCALL_H

typedef enum syscall_number
{
    SVC_syscall_usleep,
    SVC_syscall_nanosleep,
    SVC_syscall_current,
    SVC_syscall_getpid,
    SVC_syscall_exit,
    SVC_syscall_yield,
    SVC_syscall_mmap,
    SVC_syscall_dumpvma,
    SVC_syscall_getipcid,
    SVC_syscall_brk,
    SVC_COUNT,
} syscall_number_t;

#define SYS_syscall 0x00

#if defined(__thumb__) || defined(__thumb2__)
#define SYS_smhcall 0xab
#else
#define SYS_smhcall 0x123456
#endif

/* SVC with SYS_ call number and no parameters */

static inline unsigned long sys_call0(unsigned int nbr)
{
  register long reg0 __asm__("r0") = (long)(nbr);

  __asm__ __volatile__
  (
    "svc %1"
    : "=r"(reg0)
    : "i"(SYS_syscall), "r"(reg0)
    : "memory", "r14"
  );

  return reg0;
}

/* SVC with SYS_ call number and one parameter */

static inline unsigned long sys_call1(unsigned int nbr, unsigned long parm1)
{
  register long reg0 __asm__("r0") = (long)(nbr);
  register long reg1 __asm__("r1") = (long)(parm1);

  __asm__ __volatile__
  (
    "svc %1"
    : "=r"(reg0)
    : "i"(SYS_syscall), "r"(reg0), "r"(reg1)
    : "memory", "r14"
  );

  return reg0;
}

/* SVC with SYS_ call number and two parameters */

static inline unsigned long sys_call2(unsigned int nbr, unsigned long parm1,
                                  unsigned long parm2)
{
  register long reg0 __asm__("r0") = (long)(nbr);
  register long reg2 __asm__("r2") = (long)(parm2);
  register long reg1 __asm__("r1") = (long)(parm1);

  __asm__ __volatile__
  (
    "svc %1"
    : "=r"(reg0)
    : "i"(SYS_syscall), "r"(reg0), "r"(reg1), "r"(reg2)
    : "memory", "r14"
  );

  return reg0;
}

/* SVC with SYS_ call number and three parameters */

static inline unsigned long sys_call3(unsigned int nbr, unsigned long parm1,
                                  unsigned long parm2, unsigned long parm3)
{
  register long reg0 __asm__("r0") = (long)(nbr);
  register long reg3 __asm__("r3") = (long)(parm3);
  register long reg2 __asm__("r2") = (long)(parm2);
  register long reg1 __asm__("r1") = (long)(parm1);

  __asm__ __volatile__
  (
    "svc %1"
    : "=r"(reg0)
    : "i"(SYS_syscall), "r"(reg0), "r"(reg1), "r"(reg2), "r"(reg3)
    : "memory", "r14"
  );

  return reg0;
}

/* SVC with SYS_ call number and four parameters */

static inline unsigned long sys_call4(unsigned int nbr, unsigned long parm1,
                                  unsigned long parm2, unsigned long parm3,
                                  unsigned long parm4)
{
  register long reg0 __asm__("r0") = (long)(nbr);
  register long reg4 __asm__("r4") = (long)(parm4);
  register long reg3 __asm__("r3") = (long)(parm3);
  register long reg2 __asm__("r2") = (long)(parm2);
  register long reg1 __asm__("r1") = (long)(parm1);

  __asm__ __volatile__
  (
    "svc %1"
    : "=r"(reg0)
    : "i"(SYS_syscall), "r"(reg0), "r"(reg1), "r"(reg2),
      "r"(reg3), "r"(reg4)
    : "memory", "r14"
  );

  return reg0;
}

/* SVC with SYS_ call number and five parameters */

static inline unsigned long sys_call5(unsigned int nbr, unsigned long parm1,
                                  unsigned long parm2, unsigned long parm3,
                                  unsigned long parm4, unsigned long parm5)
{
  register long reg0 __asm__("r0") = (long)(nbr);
  register long reg5 __asm__("r5") = (long)(parm5);
  register long reg4 __asm__("r4") = (long)(parm4);
  register long reg3 __asm__("r3") = (long)(parm3);
  register long reg2 __asm__("r2") = (long)(parm2);
  register long reg1 __asm__("r1") = (long)(parm1);

  __asm__ __volatile__
  (
    "svc %1"
    : "=r"(reg0)
    : "i"(SYS_syscall), "r"(reg0), "r"(reg1), "r"(reg2),
      "r"(reg3), "r"(reg4), "r"(reg5)
    : "memory", "r14"
  );

  return reg0;
}

/* SVC with SYS_ call number and six parameters */

static inline unsigned long sys_call6(unsigned int nbr, unsigned long parm1,
                                  unsigned long parm2, unsigned long parm3,
                                  unsigned long parm4, unsigned long parm5,
                                  unsigned long parm6)
{
  register long reg0 __asm__("r0") = (long)(nbr);
  register long reg6 __asm__("r6") = (long)(parm6);
  register long reg5 __asm__("r5") = (long)(parm5);
  register long reg4 __asm__("r4") = (long)(parm4);
  register long reg3 __asm__("r3") = (long)(parm3);
  register long reg2 __asm__("r2") = (long)(parm2);
  register long reg1 __asm__("r1") = (long)(parm1);

  __asm__ __volatile__
  (
    "svc %1"
    : "=r"(reg0)
    : "i"(SYS_syscall), "r"(reg0), "r"(reg1), "r"(reg2),
      "r"(reg3), "r"(reg4), "r"(reg5), "r"(reg6)
    : "memory", "r14"
  );

  return reg0;
}

/* semihosting(SMH) call with call number and one parameter */

static inline long smh_call(unsigned int nbr, void *parm)
{
  register long reg0 __asm__("r0") = (long)(nbr);
  register long reg1 __asm__("r1") = (long)(parm);

  __asm__ __volatile__
  (
  "svc %1"
    : "=r"(reg0)
    : "i"(SYS_smhcall), "r"(reg0), "r"(reg1)
    : "memory", "r14"
  );

  return reg0;
}

#endif
