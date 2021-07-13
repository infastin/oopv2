#ifndef BIGINT_H_D1VNXRPE
#define BIGINT_H_D1VNXRPE

#include <stddef.h>

#include "Base.h"

#define BIGINT_TYPE (bi_get_type())
DECLARE_TYPE(BigInt, bi, BIGINT, Object);

#define WORDS(bits) (((bits) + WORD_BIT - 1) / WORD_BIT)
#define BIT(word, i) (((word) & (1 << (i))) ? 1 : 0)

#define BI_STR(v) ((BigInt*)object_new_stack(BIGINT_TYPE, (char[sizeof(BigInt)]){0}, BI_INIT_STR, v))
#define BI_INT(v) ((BigInt*)object_new_stack(BIGINT_TYPE, (char[sizeof(BigInt)]){0}, BI_INIT_INT, v))

typedef enum
{
	BI_INIT_STR = 110,
	BI_INIT_INT,
	BI_INIT_SIZED
} BigIntInitType;

typedef enum
{
	BI_SET_STR = 120,
	BI_SET_INT
} BigIntSetType;

BigInt* bi_new_str(char *numb);
BigInt* bi_new_int(int numb);
BigInt* bi_new_sized(size_t size);
BigInt* bi_copy(const BigInt *self);
void bi_delete(BigInt *self);
BigInt* bi_set_int(BigInt *self, int value);
BigInt* bi_set_str(BigInt *self, char* value);
char* bi_get(const BigInt *self);
BigInt* bi_lshift(BigInt *self, size_t shift);
BigInt* bi_rshift(BigInt *self, size_t shift);
int bi_cmp(const BigInt *a, const BigInt *b);
int bi_cmp_int(const BigInt *a, int b);
BigInt* bi_add(const BigInt *a, const BigInt *b);
BigInt* bi_add_int(const BigInt *a, int b);
BigInt* bi_sub(const BigInt *a, const BigInt *b);
BigInt* bi_sub_int(const BigInt *a, int b);
BigInt* bi_mul(const BigInt *a, const BigInt *b);
BigInt* bi_mul_int(const BigInt *a, int b);
void bi_divrem(const BigInt *dividend, const BigInt *divisor, BigInt **quot, BigInt **rem);
void bi_divrem_int(const BigInt *dividend, int divisor, BigInt **quot, BigInt **rem);
BigInt* bi_div(const BigInt *dividend, const BigInt *divisor);
BigInt* bi_div_int(const BigInt *dividend, int divisor);
BigInt* bi_mod(const BigInt *dividend, const BigInt *divisor);
BigInt* bi_mod_int(const BigInt *dividend, int divisor);
void bi_output(const BigInt *self);
void bi_outputln(const BigInt *self);

#endif /* end of include guard: BIGINT_H_D1VNXRPE */
