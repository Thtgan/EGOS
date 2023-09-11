#if !defined(__ASM_H)
#define __ASM_H

#define FUNC_BEGIN(name)    \
    .globl name;            \
    name:

#define FUNC_END(__FUNC)

#endif // __ASM_H
