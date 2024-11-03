#if !defined(__KIT_ATOMIC_H)
#define __KIT_ATOMIC_H

#include<real/simpleAsmLines.h>

#define ATOMIC_LOAD(__SRC)          __atomic_load_n(__SRC, __ATOMIC_SEQ_CST)
#define ATOMIC_STORE(__DES, __VAL)  __atomic_store_n(__DES, __VAL, __ATOMIC_SEQ_CST)

#define ATOMIC_EXCHANGE_N(__PTR, __VAL) __atomic_exchange_n(__PTR, __VAL, __ATOMIC_SEQ_CST)
#define ATOMIC_EXCHANGE(__PTR, __VAL, __RET)    __atomic_exchange(__PTR, __VAL, __RET, __ATOMIC_SEQ_CST)

#define ATOMIC_COMPARE_EXCHANGE_N(__PTR, __EXPECTED, __DESIRED) __atomic_compare_exchange_n(__PTR, __EXPECTED, __DESIRED, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)
#define ATOMIC_COMPARE_EXCHANGE(__PTR, __EXPECTED, __DESIRED) __atomic_compare_exchange(__PTR, __EXPECTED, __DESIRED, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)

#define ATOMIC_COMPARE_EXCNANGE_WRAPPER(__PTR, __OLD, __NEW) ({     \
    __typeof__(*__PTR) tmp = __OLD;                                 \
    ATOMIC_COMPARE_EXCHANGE_N(__PTR, &tmp, __NEW) ? __NEW : __OLD;  \
})

#define ATOMIC_ADD_FETCH(__PTR, __VAL)  __atomic_add_fetch(__PTR, __VAL, __ATOMIC_SEQ_CST)
#define ATOMIC_SUB_FETCH(__PTR, __VAL)  __atomic_sub_fetch(__PTR, __VAL, __ATOMIC_SEQ_CST) 
#define ATOMIC_AND_FETCH(__PTR, __VAL)  __atomic_and_fetch(__PTR, __VAL, __ATOMIC_SEQ_CST) 
#define ATOMIC_XOR_FETCH(__PTR, __VAL)  __atomic_xor_fetch(__PTR, __VAL, __ATOMIC_SEQ_CST)
#define ATOMIC_OR_FETCH(__PTR, __VAL)   __atomic_or_fetch(__PTR, __VAL, __ATOMIC_SEQ_CST)
#define ATOMIC_NAND_FETCH(__PTR, __VAL) __atomic_nand_fetch(__PTR, __VAL, __ATOMIC_SEQ_CST)

#define ATOMIC_INC_FETCH(__PTR)         ATOMIC_ADD_FETCH(__PTR, 1)
#define ATOMIC_DEC_FETCH(__PTR)         ATOMIC_SUB_FETCH(__PTR, 1)

#define ATOMIC_FETCH_ADD(__PTR, __VAL)  __atomic_fetch_add(__PTR, __VAL, __ATOMIC_SEQ_CST)
#define ATOMIC_FETCH_SUB(__PTR, __VAL)  __atomic_fetch_sub(__PTR, __VAL, __ATOMIC_SEQ_CST) 
#define ATOMIC_FETCH_AND(__PTR, __VAL)  __atomic_fetch_and(__PTR, __VAL, __ATOMIC_SEQ_CST) 
#define ATOMIC_FETCH_XOR(__PTR, __VAL)  __atomic_fetch_xor(__PTR, __VAL, __ATOMIC_SEQ_CST)
#define ATOMIC_FETCH__OR(__PTR, __VAL)  __atomic_fetch_or(__PTR, __VAL, __ATOMIC_SEQ_CST)
#define ATOMIC_FETCH_NAND(__PTR, __VAL) __atomic_fetch_nand(__PTR, __VAL, __ATOMIC_SEQ_CST)

#define ATOMIC_FETCH_INC(__PTR)         ATOMIC_FETCH_ADD(__PTR, 1)
#define ATOMIC_FETCH_DEC(__PTR)         ATOMIC_FETCH_SUB(__PTR, 1)

#define	ATOMIC_ACCESS(__VAL)			(*(volatile __typeof__(__VAL) *)&(__VAL))

#define	ATOMIC_BARRIER_WRITE(__TO, __VAL) do {  \
	barrier();			                        \
	ATOMIC_ACCESS(__TO) = (__VAL);	            \
	barrier();			                        \
} while (0)

#define	ATOMIC_BARRIER_READ(__FROM) ({		            \
    barrier();		                                    \
	__typeof__(__FROM) _tmp = ATOMIC_ACCESS(__FROM);    \
	barrier();			                                \
	_tmp;				                                \
})

#endif // __KIT_ATOMIC_H
