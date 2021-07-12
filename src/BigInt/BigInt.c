#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "BigInt/BigInt.h"
#include "Utils/Stuff.h"

/* Predefinitions {{{ */

static void stringer_interface_init(StringerInterface *iface);

DEFINE_TYPE_WITH_IFACES(BigInt, bi, object, 1, 
		USE_INTERFACE(STRINGER_INTERFACE_TYPE, stringer_interface_init));

#define bi_set_bit(self, bit, val)                                         \
	{                                                                      \
		if ((val) == 0)                                                    \
			(self)->words[(bit) / WORD_BIT] &= ~(1 << ((bit) % WORD_BIT)); \
		else                                                               \
		{                                                                  \
			if (WORDS(bit) > (self)->length)                               \
				(self)->length = WORDS(bit);                               \
			(self)->words[(bit) / WORD_BIT] |= 1 << ((bit) % WORD_BIT);    \
		}                                                                  \
	}

#define bi_get_bit(self, bit) (BIT((self)->words[(bit) / WORD_BIT], (bit)))
#define bi_last_bit(self) (((self)->length == 0) ? 0 : bi_get_bit(self, (self)->length - 1))

#define BI_ZERO ((BigInt*)object_new(BIGINT_TYPE, BI_INIT_INT, 0))
#define BI_ONE ((BigInt*)object_new(BIGINT_TYPE, BI_INIT_INT, 1))

/* }}} */

/* Private methods {{{ */

static BigInt* _BigInt_growcap(BigInt *self, size_t add)
{
	if (add == 0)
		return self;

	size_t mincap = self->capacity + add;
	size_t new_allocated = (mincap >> 3) + (mincap < 9 ? 3 : 6);

	if (mincap > SIZE_MAX - new_allocated)
	{
		msg_error("bi capacity overflow!");
		return NULL;
	}

	mincap += new_allocated;

	unsigned int *words = (unsigned int*)realloc(self->words, mincap * WORD_SIZE);

	if (words == NULL)
	{
		msg_error("couldn't reallocate memory for bi!");
		return NULL;
	}

	self->words = words;
	self->capacity = mincap;

	return self;
}

static void _BigInt_clamp(BigInt *self)
{
	while (self->length > 1 && (self->words[self->length - 1] == 0)) 
		self->length--;
}

static BigInt* _BigInt_add(const BigInt *hi, const BigInt *lo)
{
	BigInt *result = (BigInt*)object_new(BIGINT_TYPE, BI_INIT_SIZED, hi->length + 1);
	return_val_if_fail(result != NULL, NULL);

	unsigned long carry = 0;
	size_t i = 0;

	for (; i < lo->length; ++i) 
	{
		unsigned long hi_word = hi->words[i];
		unsigned long lo_word = lo->words[i];

		unsigned long tmp = hi_word + lo_word + carry;
		carry = tmp >> WORD_BIT;

		result->words[i] = (unsigned int) tmp;
	}

	for (; i < hi->length; ++i)
	{
		unsigned long hi_word = hi->words[i];
		unsigned long tmp = hi_word + carry;

		carry = tmp >> WORD_BIT;

		result->words[i] = (unsigned int) tmp;
	}

	if (carry != 0)
	{
		result->words[i] = carry;
		result->length = hi->length + 1;
	}
	else
		result->length = hi->length;

	return result;
}

static BigInt* _BigInt_sub(const BigInt *hi, const BigInt *lo)
{
	BigInt *result = (BigInt*)object_new(BIGINT_TYPE, BI_INIT_SIZED, hi->length);
	return_val_if_fail(result != NULL, NULL);

	long carry = 0;
	size_t i = 0;

	for (; i < lo->length; ++i)
	{
		long hi_word = hi->words[i];
		long lo_word = lo->words[i];

		long tmp = hi_word - lo_word - carry;

		carry = (tmp >> (WORD_BIT + 1)) & 0x1;
		result->words[i] = (unsigned int) tmp;
	}

	for (; i < hi->length; ++i)
	{
		long hi_word = hi->words[i];
		long tmp = hi_word - carry;

		carry = (tmp >> (WORD_BIT + 1)) & 0x1;
		result->words[i] = (unsigned int) tmp;
	}

	result->length = hi->length;

	return result;
}

/* }}} */

/* Public methods {{{ */

static BigInt* BigInt_addInt(const BigInt *self, int b);
static BigInt* BigInt_mulInt(const BigInt *self, int b);

static Object* BigInt_ctor(Object *_self, va_list *ap)
{
	BigInt *self = BIGINT(object_super_ctor(BIGINT_TYPE, _self, ap));

	BigIntInitType type = va_arg(*ap, BigIntInitType);

	if (type == BI_INIT_SIZED)
	{
		size_t numb = va_arg(*ap, size_t);

		if (numb == 0)
			self->capacity = 1;
		else
			self->capacity = numb;
	}
	else if (type == BI_INIT_LONG)
		self->capacity = 2;
	else
		self->capacity = 1;

	self->length = 0;
	self->words = (unsigned int*)calloc(self->capacity, WORD_SIZE);
	self->sign = 0;

	if (self->words == NULL)
	{
		object_delete((Object*) self);
		msg_error("couldn't allocate memory for bi!");
		return NULL;
	}

	if (type == BI_INIT_SIZED)
		return (Object*) self;

	if (type == BI_INIT_INT)
	{
		int numb = va_arg(*ap, int);
		
		if (numb != 0)
		{
			self->words[0] = numb;
			self->length = 1;

			if (numb < 0)
				self->sign = 1;
		}
	}
	else if (type == BI_INIT_LONG)
	{
		long numb = va_arg(*ap, long);

		if (numb != 0)
		{
			int numb_len = 64 - __builtin_clzl(numb);

			if (numb_len > WORD_BIT)
			{
				self->length = 2;
				self->words[0] = (unsigned int) numb;
				self->words[1] = (unsigned int) (numb >> WORD_BIT);
			}
			else
			{
				self->length = 1;
				self->words[0] = (unsigned int) numb;
			}

			if (numb < 0)
				self->sign = 1;
		}
	}
	else
	{
		char *numb = va_arg(*ap, char*);
		char *p = numb;

		if (*p == '-')
		{
			self->sign = 1;
			p++;
		}

		for (; *p != '\0'; ++p)
		{
			if (!isdigit(*p))
			{
				object_delete((Object*) self);
				msg_error("given string is not a number!");
				return NULL;
			}

			if (self->length != 0)
			{
				BigInt *self_mul = BigInt_mulInt(self, 10);
				object_delete((Object*) self);

				if (self_mul == NULL)
					return_val_if_fail(self_mul != NULL, NULL);

				self = self_mul;
			}

			if (*p - '0' != 0)
			{
				BigInt *self_add = BigInt_addInt(self, *p - '0');
				object_delete((Object*) self);

				if (self_add == NULL)
					return_val_if_fail(self_add != NULL, NULL);

				self = self_add;
			}
		}
	}

	return (Object*) self;
}

static Object* BigInt_dtor(Object *_self, va_list *ap)
{
	BigInt *self = BIGINT(_self);
	
	if (self->words)
		free(self->words);

	return _self;
}

static Object* BigInt_cpy(const Object *_self, Object *_object)
{
	const BigInt *self = BIGINT(_self);
	BigInt *object = (BigInt*) object_super_cpy(BIGINT_TYPE, _self, _object);

	object->capacity = self->capacity;
	object->length = self->length;
	object->words = (unsigned int*)calloc(self->capacity, WORD_SIZE);

	if (object->words == NULL)
	{
		object_delete((Object*) object);
		msg_error("couldn't allocate memory for the copy of bi!");
		return NULL;
	}

	memcpy(object->words, self->words, self->length * WORD_SIZE);

	return _object;
}

static void BigInt_string(const Stringer *_self, va_list *ap)
{
	const BigInt *self = BIGINT((const Object*) _self);

	if (self->length == 0)
		printf("0000 0000");

	/*char *str = NULL;

	BigInt *t = (BigInt*)object_copy((const Object*) self);

	while (t->length != 0) 
	{
		BigInt *q, *r;
		BigInt_divremInt(t, 10, &q, &r);

		if (str == NULL)
			str = strdup_printf("%d", r->words[0]);
		else
		{
			char *new_str = strdup_printf("%s%d", r->words[0]);
			free(str);
			str = new_str;
		}

		object_delete((Object*) t);
		object_delete((Object*) r);

		t = q;
	}

	printf("%s", str);
	free(str); */

	for (size_t i = self->length - 1; i >= 0; --i)
	{
		for (int j = 31; j >= 0; --j)
		{
			printf("%d", BIT(self->words[i], j));
		}

		printf(" ");

		if (i == 0)
			break;
	}
}

/* Comparing {{{ */

static int BigInt_cmp(const BigInt *a, const BigInt *b)
{
	if (a->sign < b->sign)
		return 1;
	if (a->sign > b->sign)
		return -1;

	if (a->length > b->length)
		return 1;
	if (a->length < b->length)
		return -1;

	if (a->length == 0)
		return 0;

	for (size_t i = a->length - 1; i >= 0; --i)
	{
		if (a->words[i] > b->words[i])
			return 1;
		
		if (a->words[i] < b->words[i])
			return -1;

		if (i == 0)
			break;
	}

	return 0;
}

static int BigInt_sccmp(const BigInt *a, unsigned int b)
{
	int b_sign = (b < 0) ? 1 : 0;

	if (a->sign < b_sign)
		return 1;
	if (a->sign > b_sign)
		return -1;

	if (a->length == 0 && b != 0)
		return -1;
	if (a->length > 1)
		return 1;

	if (a->words[0] > b)
		return 1;
	if (a->words[0] < b)
		return -1;

	return 0;
}

static int BigInt_sccmpl(const BigInt *a, unsigned long b)
{
	int b_sign = (b < 0) ? 1 : 0;

	if (a->sign < b_sign)
		return 1;
	if (a->sign > b_sign)
		return -1;

	if (a->length == 0 && b != 0)
		return -1;
	if (a->length > 2)
		return 1;

	unsigned int b_bits = 64 - __builtin_clzl(b);

	if (b_bits <= WORD_BIT && a->length == 2)
		return 1;
	if (b_bits > WORD_BIT && a->length < 2)
		return -1;

	if (b_bits <= WORD_BIT)
	{
		if (a->words[0] > (unsigned int) b)
			return 1;
		if (a->words[0] < (unsigned int) b)
			return -1;
	}
	else
	{
		for (int i = 1; i >= 0; --i)
		{
			if (a->words[i] > (unsigned int) (b >> i * WORD_BIT))
				return 1;
			if (a->words[i] < (unsigned int) (b >> i * WORD_BIT))
				return -1;
		}
	}

	return 0;
}

/* }}} */

/* Sum {{{ */

static BigInt* BigInt_add(const BigInt *a, const BigInt *b)
{
	const BigInt *hi, *lo;
	BigInt *result;

	int cmp = BigInt_cmp(a, b);

	if (cmp > 0)
	{
		hi = a;
		lo = b;
	}
	else
	{
		hi = b;
		lo = a;
	}

	if (a->sign == b->sign)
	{
		result = _BigInt_add(hi, lo);
		return_val_if_fail(result != NULL, NULL);
		result->sign = a->sign;
	}
	else
	{
		int sign;

		if (cmp > 0)
			sign = a->sign;
		else if (cmp < 0)
			sign = b->sign;
		else
		{
			hi = NULL;
			lo = NULL;
			sign = 0;
		}

		if (hi == NULL && lo == NULL)
			result = BI_ZERO;
		else
			result = _BigInt_sub(hi, lo);

		return_val_if_fail(result != NULL, NULL);
		result->sign = sign;
	}

	return result;
}

static BigInt* BigInt_addInt(const BigInt *self, int b)
{
	BigInt *result;

	if (b == 0)
	{
		result = (BigInt*)object_copy((const Object*) self);
		return_val_if_fail(result != NULL, NULL);

		return result;
	}

	result = BigInt_add(self, BI_INT(b));
	return_val_if_fail(result != NULL, NULL);

	return result;
}

static BigInt* BigInt_addLong(const BigInt *self, long b)
{
	BigInt *result;

	if (b == 0)
	{
		result = (BigInt*)object_copy((const Object*) self);
		return_val_if_fail(result != NULL, NULL);

		return result;
	}

	result = BigInt_add(self, BI_LONG(b));
	return_val_if_fail(result != NULL, NULL);

	return result;
}

/* }}} */

/* Subtract {{{ */

static BigInt* BigInt_sub(const BigInt *a, const BigInt *b)
{
	BigInt *result;
	const BigInt *hi, *lo;

	int cmp = BigInt_cmp(a, b);

	if (cmp > 0)
	{
		hi = a;
		lo = b;
	}
	else
	{
		hi = b;
		lo = a;
	}

	if (a->sign != b->sign)
	{
		result = _BigInt_add(hi, lo);
		return_val_if_fail(result != NULL, NULL);
		result->sign = a->sign;
	}
	else
	{
		int sign;

		if (cmp > 0)
			sign = a->sign;
		else if (cmp < 0)
			sign = (b->sign == 1) ? 0 : 1;
		else
		{
			hi = NULL;
			lo = NULL;
			sign = 0;
		}

		if (hi == NULL && lo == NULL)
			result = BI_ZERO;
		else
			result = _BigInt_sub(hi, lo);

		return_val_if_fail(result != NULL, NULL);
		result->sign = sign;
	}

	return result;
}

static BigInt* BigInt_subInt(BigInt *self, int b)
{
	BigInt *result = BigInt_sub(self, BI_INT(b));
	return_val_if_fail(result != NULL, NULL);

	return result;
}

static BigInt* BigInt_subLong(BigInt *self, long b)
{
	BigInt *result = BigInt_sub(self, BI_LONG(b));
	return_val_if_fail(result != NULL, NULL);

	return result;
}

/* }}} */

/* Shifts {{{ */

static BigInt* BigInt_lshift(BigInt *self, size_t shift)
{
	if (shift == 0)
		return self;

	size_t wlshift = shift / WORD_BIT;
	size_t lshift = shift % WORD_BIT;

	if (self->length + WORDS(shift) > self->capacity)
	{
		self = _BigInt_growcap(self, (self->length + WORDS(shift)) - self->capacity);
		return_val_if_fail(self != NULL, NULL);
	}

	if (wlshift != 0)
	{
		memmove(self->words + wlshift, self->words, self->length * WORD_SIZE);
		self->length += wlshift;
	}

	if (lshift == 0)
		return self;

	size_t sh = WORD_BIT - lshift;
	size_t i = wlshift;

	unsigned long mask = (1UL << lshift) - 1;
	unsigned long r = 0;

	for (; i < self->length; ++i)
	{
		unsigned long self_word = self->words[i]; 
		unsigned long rr = (self_word >> sh) & mask;
		unsigned long new_word = (self_word << lshift) | r;
		
		self->words[i] = (unsigned int) new_word;
		r = rr;
	}

	if (r != 0)
	{
		self->words[i] = (unsigned int) r;
		self->length++;
	}

	return self;
}

static BigInt* BigInt_rshift(BigInt *self, size_t shift)
{
	if (shift == 0)
		return self;

	size_t wrshift = shift / WORD_BIT;
	size_t rshift = shift % WORD_BIT;

	if (wrshift != 0)
	{
		if (wrshift >= self->length)
		{
			memset(self->words, 0, self->length * WORD_SIZE);
			self->length = 0;

			return self;
		}

		memmove(self->words, self->words + wrshift, (self->length - wrshift) * WORD_SIZE);
	}

	if (rshift == 0)
		return self;

	size_t sh = WORD_BIT - rshift;
	size_t i = self->length - wrshift - 1;

	unsigned long mask = (1UL << rshift) - 1;
	unsigned long r = 0;

	for (; i >= 0; --i) 
	{
		unsigned long self_word = self->words[i];
		unsigned long rr = self_word & mask;
		unsigned long new_word = (self_word >> rshift) | (rr << sh);
		
		self->words[i] = (unsigned int) new_word;
		r = rr;
	}

	self->length = WORDS(self->length * WORD_BIT - shift);

	return self;
}

/* }}} */

/* Multiplication {{{ */

static BigInt* BigInt_mul(const BigInt *a, const BigInt *b)
{	
	BigInt *result;

	if (a->length == 0 || b->length == 0)
	{
		result = BI_ZERO;
		return_val_if_fail(result != NULL, NULL);

		return result;
	}

	const BigInt *hi, *lo;

	if (BigInt_cmp(a, b) > 0)
	{
		hi = a;
		lo = b;
	}
	else 
	{
		hi = b;
		lo = a;
	}

	result = (BigInt*)object_new(BIGINT_TYPE, BI_INIT_SIZED, hi->length + lo->length);
	return_val_if_fail(result != NULL, NULL);

	result->length = hi->length + lo->length;

	for (size_t i = 0; i < hi->length; ++i)
	{
		unsigned long carry = 0;

		for (size_t j = 0; j < lo->length; ++j)
		{
			unsigned long result_word = result->words[i + j];
			unsigned long hi_word = hi->words[i];
			unsigned long lo_word = lo->words[j];

			unsigned long temp = result_word + (hi_word * lo_word) + carry;
			
			carry = temp >> WORD_BIT;
			result->words[i + j] = (unsigned int) temp;
		}

		result->words[i + lo->length] = (unsigned int) carry;
	}

	_BigInt_clamp(result);

	return result;
}

static BigInt* BigInt_mulInt(const BigInt *self, int b)
{
	BigInt *result = BigInt_mul(self, BI_INT(b));
	return_val_if_fail(result != NULL, NULL);

	return result;
}

static BigInt* BigInt_mulLong(BigInt *self, long b)
{
	BigInt *result = BigInt_mul(self, BI_LONG(b));
	return_val_if_fail(result != NULL, NULL);

	return result;
}

/* }}} */

/* Division {{{ */

static void BigInt_divrem(const BigInt *dividend, const BigInt *divisor, BigInt **ret_quot, BigInt **ret_rem)
{
}

static void BigInt_divremInt(const BigInt *dividend, int divisor, BigInt **ret_quot, BigInt **ret_rem)
{
	return_if_fail(divisor != 0);

	BigInt *quot = NULL, *rem = NULL;

	if (dividend->length == 0)
	{
		quot = BI_ZERO;
		return_if_fail(quot != NULL);

		rem = BI_ZERO;

		if (rem == NULL)
		{
			object_delete((Object*) quot);
			return_if_fail(quot != NULL);
		}

		*ret_quot = quot;
		*ret_rem = rem;
	}

	BigInt_divrem(dividend, BI_INT(divisor), &quot, &rem);
	return_if_fail(quot != NULL && rem != NULL);

	*ret_quot = quot;
	*ret_rem = rem;
}

static BigInt* BigInt_div(const BigInt *dividend, const BigInt *divisor)
{
	BigInt *quot = NULL;
	BigInt_divrem(dividend, divisor, &quot, NULL);
	return_val_if_fail(quot != NULL, NULL);

	return quot;
}

static BigInt* BigInt_mod(const BigInt *dividend, const BigInt *divisor)
{
	BigInt *rem = NULL;
	BigInt_divrem(dividend, divisor, NULL, &rem);
	return_val_if_fail(rem != NULL, NULL);

	return rem;
}

/* }}} */

/* }}} */

/* Selectors {{{ */

BigInt* bi_new_str(char *numb)
{
	return (BigInt*)object_new(BIGINT_TYPE, BI_INIT_STR, numb);
}

BigInt* bi_new_int(int numb)
{
	return (BigInt*)object_new(BIGINT_TYPE, BI_INIT_INT, numb);
}

BigInt* bi_new_long(long numb)
{
	return (BigInt*)object_new(BIGINT_TYPE, BI_INIT_LONG, numb);
}

BigInt* bi_new_sized(size_t size)
{
	return (BigInt*)object_new(BIGINT_TYPE, BI_INIT_SIZED, size);
}

BigInt* bi_copy(const BigInt *self)
{
	return_val_if_fail(IS_BIGINT(self), NULL);
	return (BigInt*)object_copy((const Object*) self);
}

BigInt* bi_set(BigInt *self, size_t bit, int value)
{
	return_val_if_fail(IS_BIGINT(self), NULL);
	return (BigInt*)object_set((Object*) self, bit, value);
}

void bi_get(const BigInt *self, size_t bit, int *ret)
{
	return_if_fail(IS_BIGINT(self));
	return_if_fail(ret != NULL);

	object_get((Object*) self, bit, ret);
}

void bi_delete(BigInt *self)
{
	return_if_fail(IS_BIGINT(self));
	object_delete((Object*) self);
}

BigInt* bi_lshift(BigInt *self, size_t shift)
{
	return_val_if_fail(IS_BIGINT(self), NULL);

	BigIntClass *klass = BIGINT_GET_CLASS(self);

	return_val_if_fail(klass->lshift != NULL, NULL);
	return klass->lshift(self, shift);
}

BigInt* bi_rshift(BigInt *self, size_t shift)
{
	return_val_if_fail(IS_BIGINT(self), NULL);

	BigIntClass *klass = BIGINT_GET_CLASS(self);

	return_val_if_fail(klass->rshift != NULL, NULL);
	return klass->rshift(self, shift);
}

BigInt* bi_add(const BigInt *a, const BigInt *b)
{
	return_val_if_fail(IS_BIGINT(a), NULL);
	return_val_if_fail(IS_BIGINT(b), NULL);

	BigIntClass *klass = BIGINT_GET_CLASS(a);

	return_val_if_fail(klass->add != NULL, NULL);
	return klass->add(a, b);
}

BigInt* bi_sub(const BigInt *a, const BigInt *b)
{
	return_val_if_fail(IS_BIGINT(a), NULL);
	return_val_if_fail(IS_BIGINT(b), NULL);

	BigIntClass *klass = BIGINT_GET_CLASS(a);

	return_val_if_fail(klass->sub != NULL, NULL);
	return klass->sub(a, b);
}

BigInt* bi_mul(const BigInt *a, const BigInt *b)
{
	return_val_if_fail(IS_BIGINT(a), NULL);
	return_val_if_fail(IS_BIGINT(b), NULL);

	BigIntClass *klass = BIGINT_GET_CLASS(a);

	return_val_if_fail(klass->mul != NULL, NULL);
	return klass->mul(a, b);
}

void bi_divrem(const BigInt *dividend, const BigInt *divisor, BigInt **quot, BigInt **rem)
{
	return_if_fail(IS_BIGINT(dividend));
	return_if_fail(IS_BIGINT(divisor));

	BigIntClass *klass = BIGINT_GET_CLASS(dividend);

	return_if_fail(klass->divrem != NULL);
	klass->divrem(dividend, divisor, quot, rem);
}

BigInt* bi_div(const BigInt *dividend, const BigInt *divisor)
{
	return_val_if_fail(IS_BIGINT(dividend), NULL);
	return_val_if_fail(IS_BIGINT(divisor), NULL);

	BigIntClass *klass = BIGINT_GET_CLASS(dividend);

	return_val_if_fail(klass->div != NULL, NULL);
	return klass->div(dividend, divisor);
}

BigInt* bi_mod(const BigInt *dividend, const BigInt *divisor)
{
	return_val_if_fail(IS_BIGINT(dividend), NULL);
	return_val_if_fail(IS_BIGINT(divisor), NULL);

	BigIntClass *klass = BIGINT_GET_CLASS(dividend);

	return_val_if_fail(klass->mod != NULL, NULL);
	return klass->mod(dividend, divisor);
}

void bi_output(const BigInt *self)
{
	return_if_fail(IS_BIGINT(self));
	stringer_output((const Stringer*) self);
}

void bi_outputln(const BigInt *self)
{
	return_if_fail(IS_BIGINT(self));
	stringer_outputln((const Stringer*) self);
}

/* }}} */

/* Init {{{ */

static void stringer_interface_init(StringerInterface *iface)
{
	iface->string = BigInt_string;
}

static void bi_class_init(BigIntClass *klass)
{
	OBJECT_CLASS(klass)->ctor = BigInt_ctor;
	OBJECT_CLASS(klass)->dtor = BigInt_dtor;
	OBJECT_CLASS(klass)->cpy = BigInt_cpy;

	klass->lshift = BigInt_lshift;
	klass->rshift = BigInt_rshift;
	klass->add = BigInt_add;
	klass->sub = BigInt_sub;
	klass->mul = BigInt_mul;
	klass->divrem = BigInt_divrem;
	klass->div = BigInt_div;
	klass->mod = BigInt_mod;
}

/* }}} */
