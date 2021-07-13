#ifndef BIGINT_H_D1VNXRPE
#define BIGINT_H_D1VNXRPE

#include <stddef.h>

#include "Base.h"
#include "Utils/Stuff.h"
#include "Interfaces/StringerInterface.h"

#define BIGINT_TYPE (bi_get_type())
DECLARE_DERIVABLE_TYPE(BigInt, bi, BIGINT, Object);

#define WORD_BIT 32
#define WORDS(bits) (((bits) + WORD_BIT - 1) / WORD_BIT)
#define ULBIT(lword, i) (((lword) & (1UL << (i))) ? 1 : 0)
#define BIT(word, i) (((word) & (1 << (i))) ? 1 : 0)

#define BI_STR(v) ((BigInt*)object_new_stack(BIGINT_TYPE, (char[sizeof(BigInt)]){0}, BI_INIT_STR, v))
#define BI_INT(v) ((BigInt*)object_new_stack(BIGINT_TYPE, (char[sizeof(BigInt)]){0}, BI_INIT_INT, v))

typedef unsigned int  	word_t;
typedef unsigned long 	lword_t;
typedef long 			slword_t;

struct _BigIntClass
{
	ObjectClass parent;

	BigInt*	(*lshift)(BigInt *self, size_t shift);
	BigInt*	(*rshift)(BigInt *self, size_t shift);
	int     (*cmp)(const BigInt *a, const BigInt *b);
	int     (*cmp_int)(const BigInt *a, int b);
	BigInt*	(*add)(const BigInt *a, const BigInt *b);
	BigInt*	(*add_int)(const BigInt *a, int b);
	BigInt*	(*sub)(const BigInt *a, const BigInt *b);
	BigInt*	(*sub_int)(const BigInt *a, int b);
	BigInt*	(*mul)(const BigInt *a, const BigInt *b);
	BigInt*	(*mul_int)(const BigInt *a, int b);
	void 	(*divrem)(const BigInt *dividend, const BigInt *divisor, BigInt **quot, BigInt **rem);
	void 	(*divrem_int)(const BigInt *dividend, int divisor, BigInt **quot, BigInt **rem);
};

struct _BigInt
{
	Object 	parent;
	size_t 	capacity;
	size_t 	length;
	word_t *words;
	int 	sign;
};

typedef enum
{
	BI_INIT_STR,
	BI_INIT_INT,
	BI_INIT_SIZED
} BigIntInitType;

typedef enum
{
	BI_SET_STR,
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
