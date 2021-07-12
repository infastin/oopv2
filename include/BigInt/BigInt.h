#ifndef BIGINT_H_D1VNXRPE
#define BIGINT_H_D1VNXRPE

#include <stddef.h>

#include "Base.h"
#include "Utils/Stuff.h"
#include "Interfaces/StringerInterface.h"

#define BIGINT_TYPE (bi_get_type())
DECLARE_DERIVABLE_TYPE(BigInt, bi, BIGINT, Object);

#define WORD_BIT 32
#define WORD_SIZE 4
#define WORDS(bits) (((bits) + WORD_BIT - 1) / WORD_BIT)
#define ULBIT(lword, i) (((lword) & (1UL << (i))) ? 1 : 0)
#define BIT(word, i) (((word) & (1 << (i))) ? 1 : 0)

#define BI_STR(v) ((BigInt*)object_new_stack(BIGINT_TYPE, (char[sizeof(BigInt)]){0}, BI_INIT_STR, v))
#define BI_INT(v) ((BigInt*)object_new_stack(BIGINT_TYPE, (char[sizeof(BigInt)]){0}, BI_INIT_INT, v))
#define BI_LONG(v) ((BigInt*)object_new_stack(BIGINT_TYPE, (char[sizeof(BigInt)]){0}, BI_INIT_LONG, v))

struct _BigIntClass
{
	ObjectClass parent;

	BigInt*	(*lshift)(BigInt *self, size_t shift);
	BigInt*	(*rshift)(BigInt *self, size_t shift);
	BigInt*	(*add)(const BigInt *a, const BigInt *b);
	BigInt*	(*sub)(const BigInt *a, const BigInt *b);
	BigInt*	(*mul)(const BigInt *a, const BigInt *b);
	void 	(*divrem)(const BigInt *dividend, const BigInt *divisor, BigInt **quot, BigInt **rem);
	BigInt*	(*div)(const BigInt *dividend, const BigInt *divisor);
	BigInt*	(*mod)(const BigInt *dividend, const BigInt *divisor);
};

struct _BigInt
{
	Object 			parent;
	size_t 			capacity;
	size_t 			length;
	unsigned int   *words;
	int 			sign;
};

typedef enum
{
	BI_INIT_STR,
	BI_INIT_INT,
	BI_INIT_LONG,
	BI_INIT_SIZED
} BigIntInitType;

BigInt* bi_new_str(char *numb);
BigInt* bi_new_int(int numb);
BigInt* bi_new_long(long numb);
BigInt* bi_new_sized(size_t size);
BigInt* bi_copy(const BigInt *self);
void bi_delete(BigInt *self);
BigInt* bi_lshift(BigInt *self, size_t shift);
BigInt* bi_rshift(BigInt *self, size_t shift);
BigInt* bi_add(const BigInt *a, const BigInt *b);
BigInt* bi_sub(const BigInt *a, const BigInt *b);
BigInt* bi_mul(const BigInt *a, const BigInt *b);
void bi_divrem(const BigInt *dividend, const BigInt *divisor, BigInt **quot, BigInt **rem);
BigInt* bi_div(const BigInt *dividend, const BigInt *divisor);
BigInt* bi_mod(const BigInt *dividend, const BigInt *divisor);
void bi_output(const BigInt *self);
void bi_outputln(const BigInt *self);

#endif /* end of include guard: BIGINT_H_D1VNXRPE */
