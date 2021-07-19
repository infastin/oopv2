#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "Base.h"
#include "DataStructs/BigInt.h"
#include "DataStructs/Array.h"
#include "Interfaces/StringerInterface.h"
#include "Utils/Stuff.h"

/* Predefinitions {{{ */

typedef unsigned int		word_t;
typedef unsigned long long	lword_t;
typedef long long			slword_t;

struct _BigInt
{
	Object	parent;
	size_t	capacity;
	size_t	length;
	word_t *words;
	int		sign;
};

static void stringer_interface_init(StringerInterface *iface);

DEFINE_TYPE_WITH_IFACES(BigInt, bi, object, 1, 
		USE_INTERFACE(STRINGER_INTERFACE_TYPE, stringer_interface_init));

#define WORD_BIT 32 // Bits per word
#define WORD_MASK ((1ULL << WORD_BIT) - 1) // Word mask 
#define WORD_BASE (1ULL << WORD_BIT) // Word base
#define BI_ZERO ((BigInt*)object_new(BIGINT_TYPE, BI_INIT_INT, 0))

/* }}} */

/* Private methods {{{ */

static BigInt* _BigInt_growcap(BigInt *self, size_t add)
{
	if (add == 0)
		return self;

	if (self->length + add <= self->capacity)
		return self;

	size_t mincap = self->capacity + add;
	size_t new_allocated = (mincap >> 3) + (mincap < 9 ? 3 : 6);

	if (mincap > SIZE_MAX - new_allocated)
	{
		msg_error("bigint capacity overflow!");
		return NULL;
	}

	mincap += new_allocated;

	word_t *words = (word_t*)realloc(self->words, mincap * sizeof(word_t));

	if (words == NULL)
	{
		msg_error("couldn't reallocate memory for bigint!");
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

	word_t carry = 0;
	size_t i = 0;

	for (; i < lo->length; ++i) 
	{
		word_t hi_word = hi->words[i];
		word_t lo_word = lo->words[i];

		word_t new_word = hi_word + lo_word + carry;

		if (hi_word > new_word || lo_word > new_word)
			carry = 0x1;
		else
			carry = 0;

		result->words[i] = new_word;
	}

	for (; i < hi->length; ++i)
	{
		word_t hi_word = hi->words[i];
		word_t new_word = hi_word + carry;

		if (hi_word > new_word)
			carry = 0x1;
		else
			carry = 0;

		result->words[i] = new_word;
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

	word_t carry = 0;
	size_t i = 0;

	for (; i < lo->length; ++i)
	{
		word_t hi_word = hi->words[i];
		word_t lo_word = lo->words[i];

		word_t new_word = hi_word - lo_word - carry;

		if (new_word > hi_word && new_word > lo_word)
			carry = 0x1;
		else
			carry = 0;

		result->words[i] = new_word;
	}

	for (; i < hi->length; ++i)
	{
		word_t hi_word = hi->words[i];
		word_t new_word = hi_word - carry;

		if (new_word > hi_word)
			carry = 0x1;
		else
			carry = 0;

		result->words[i] = new_word;
	}

	result->length = hi->length;

	return result;
}

static int _BigInt_divmnu(word_t *q, word_t *r, word_t *u, word_t *v, size_t m, size_t n)
{
	lword_t b = WORD_BASE; // Number base
	lword_t mask = WORD_MASK; // Number mask b - 1

	word_t *un, *vn; // Normalized form of u, v
	lword_t qhat; // Estimated quotient digit
	lword_t rhat; // A remainder
	lword_t p; // Product of two digits

	slword_t t, k;

	if (n == 1)
	{
		k = 0;

		for (size_t j = m - 1; j >= 0; --j)
		{
			q[j] = (k * b + u[j]) / v[0];
			k = (k * b + u[j]) - (q[j] * v[0]);
		}

		r[0] = k;

		return 0;
	}

	// Normalize by shifting v left just enough so that
	// its high-order bit is on, and shift u left the
	// same amount.  We may have to append a high-order
	// digit on the dividend; we do that unconditionally.

	int s = WORD_BIT - ((UINT_BIT - __builtin_clz(v[n - 1])) + 1);
	vn = (word_t*)calloc(n, sizeof(word_t));
	return_val_if_fail(vn != NULL, 1);

	for (size_t i = n - 1; i > 0; --i)
		vn[i] = ((v[i] << s) & mask) | (((lword_t) v[i - 1] >> (WORD_BIT - s)) & mask);

	vn[0] = (v[0] << s) & mask;

	un = (word_t*)calloc(m + 1, sizeof(word_t));

	if (un == NULL)
	{
		free(vn);
		return_val_if_fail(un != NULL, 2);
	}

	un[m] = (u[m - 1] >> (WORD_BIT - s)) & mask;

	for (size_t i = m - 1; i > 0; --i)
		un[i] = ((u[i] << s) & mask) | (((lword_t) u[i - 1] >> (WORD_BIT - s)) & mask);

	un[0] = (u[0] << s) & mask;

	for (size_t j = m - n; j >= 0; --j) // Main loop
	{
		// Compute estimate qhat of q[j]
		qhat = ((un[j + n] * b) + un[j + n - 1]) / (vn[n - 1]);
		rhat = ((un[j + n] * b) + un[j + n - 1]) % vn[n - 1];

		while (1) 
		{
			if (qhat >= b || ((word_t) qhat * (lword_t) vn[n - 2]) > (b * rhat + un[j + n - 2]))
			{
				qhat -= 1;
				rhat += vn[n-1];

				if (rhat < b)
					continue;
			}

			break;
		}

		// Multiply and subtract
		k = 0;
		for (size_t i = 0; i < n; ++i)
		{
			p = (word_t) qhat * (lword_t) vn[i];
			t = un[i + j] - k - (p & mask);
			un[i + j] = t & mask;
			k = (p / b) - (t / b);
		}

		t = un[j + n] - k;
		un[j + n] = t;

		q[j] = qhat; // Store quotient digit

		if (t < 0) // If we subtracted too
		{
			q[j] -= 1; // ...much, add back
			k = 0;

			for (size_t i = 0; i < n; ++i)
			{
				t = (lword_t) un[i + j] + vn[i] + k;
				un[i + j] = t & mask;
				k = t / b;
			}

			un[j + n] = un[j + n] + k;
		}

		if (j == 0)
			break;
	}

	// If the caller wants the remainder, unnormalize
	// it and pass it back.

	for (size_t i = 0; i < n; i++)
		r[i] = ((un[i] >> s) & mask) | (((lword_t) un[i + 1] << (WORD_BIT - s)) & mask);

	r[n - 1] = (un[n - 1] >> s) & mask;

	return 0;
}

static word_t _BigInt_divrem2in1(word_t *u, size_t m, word_t v, word_t *q)
{
	lword_t k = 0;
	lword_t t;

	for (size_t j = m - 1; j >= 0; --j)
	{
		k = (k << WORD_BIT) | u[j];

		if (k >= v)
		{
			t = k / v;
			k -= t * v;
		}
		else
			t = 0;

		q[j] = t;

		if (j == 0)
			break;
	}

	return (word_t) k;
}

static void BigInt_divrem_int(const BigInt *dividend, int divisor, BigInt **ret_quot, BigInt **ret_rem);

static char* _BigInt_string(const BigInt *self)
{
	char *result;

	if (self->length == 0)
	{
		result = strdup_printf("0");
		return_val_if_fail(result != NULL, NULL);

		return result;
	}

	Array *string = array_new(true, true, 1, NULL);
	return_val_if_fail(string != NULL, NULL);

	BigInt *t = (BigInt*)object_copy((const Object*) self);

	int sign = t->sign;

	while (t->length != 0) 
	{
		BigInt *q, *r;
		BigInt_divrem_int(t, 10, &q, &r);

		array_prepend(string, GET_PTR(char, r->words[0] + '0'));

		object_delete((Object*) t);
		object_delete((Object*) r);

		t = q;
	}

	if (sign == 1)
		array_prepend(string, GET_PTR(char, '-'));

	result = array_steal(string, NULL);
	array_delete(string);
	object_delete((Object*) t);

	return result;
}

/* }}} */

/* Public methods {{{ */

static BigInt* BigInt_add_int(const BigInt *a, int b);
static BigInt* BigInt_mul_int(const BigInt *a, int b);

/* Base {{{ */

static Object* BigInt_ctor(Object *_self, va_list *ap)
{
	BigInt *self = BIGINT(OBJECT_CLASS(OBJECT_TYPE)->ctor(_self, ap));

	BigIntInitType type = va_arg(*ap, BigIntInitType);

	if (type == BI_INIT_SIZED)
	{
		size_t numb = va_arg(*ap, size_t);

		if (numb == 0)
			self->capacity = 1;
		else
			self->capacity = numb;
	}
	else
		self->capacity = 1;

	self->length = 0;
	self->words = (word_t*)calloc(self->capacity, sizeof(word_t));
	self->sign = 0;

	if (self->words == NULL)
	{
		object_delete((Object*) self);
		msg_error("couldn't allocate memory for bigint!");
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
	else
	{
		char *numb = va_arg(*ap, char*);
		char *p = numb;

		int sign = 0;

		if (*p == '-')
		{
			sign = 1;
			p++;
		}

		for (; *p != '\0'; ++p)
		{
			if (!isdigit(*p))
			{
				object_delete((Object*) self);
				msg_warn("given string is not a number!");
				return NULL;
			}

			if (self->length != 0)
			{
				BigInt *self_mul = BigInt_mul_int(self, 10);
				object_delete((Object*) self);

				if (self_mul == NULL)
					return_val_if_fail(self_mul != NULL, NULL);

				self = self_mul;
			}

			if (*p - '0' != 0)
			{
				BigInt *self_add = BigInt_add_int(self, *p - '0');
				object_delete((Object*) self);

				if (self_add == NULL)
					return_val_if_fail(self_add != NULL, NULL);

				self = self_add;
			}
		}

		if (p == numb + 1)
		{
			object_delete((Object*) self);
			msg_warn("given string is not a number!");
			return NULL;
		}

		self->sign = sign;
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

static Object* BigInt_cpy(const Object *_self, Object *_object, va_list *ap)
{
	const BigInt *self = BIGINT(_self);
	BigInt *object = BIGINT(OBJECT_CLASS(OBJECT_TYPE)->cpy(_self, _object, ap));

	object->capacity = self->capacity;
	object->length = self->length;
	object->words = (word_t*)calloc(self->capacity, sizeof(word_t));
	object->sign = self->sign;

	if (object->words == NULL)
	{
		object_delete((Object*) object);
		msg_error("couldn't allocate memory for the copy of bi!");
		return NULL;
	}

	memcpy(object->words, self->words, self->length * sizeof(word_t));

	return _object;
}

static Object* BigInt_set(Object *_self, va_list *ap)
{
	BigInt *self = BIGINT(_self);

	BigIntSetType type = va_arg(*ap, BigIntSetType);

	if (type == BI_SET_INT)
	{
		int numb = va_arg(*ap, int);

		memset(self->words, 0, self->length * sizeof(word_t));

		if (numb != 0)
		{
			self->words[0] = numb;
			self->length = 1;

			if (numb < 0)
				self->sign = 1;
		}
	}
	else
	{
		char *numb = va_arg(*ap, char*);
		char *p = numb;

		memset(self->words, 0, self->length * sizeof(word_t));

		int sign = 0;

		if (*p == '-')
		{
			sign = 1;
			p++;
		}

		for (; *p != '\0'; ++p)
		{
			if (!isdigit(*p))
			{
				msg_warn("given string is not a number!");
				return NULL;
			}

			if (self->length != 0)
			{
				BigInt *self_mul = BigInt_mul_int(self, 10);
				object_delete((Object*) self);

				if (self_mul == NULL)
					return_val_if_fail(self_mul != NULL, NULL);

				self = self_mul;
			}

			if (*p - '0' != 0)
			{
				BigInt *self_add = BigInt_add_int(self, *p - '0');
				object_delete((Object*) self);

				if (self_add == NULL)
					return_val_if_fail(self_add != NULL, NULL);

				self = self_add;
			}
		}

		if (p == numb + 1)
		{
			msg_warn("given string is not a number!");
			return NULL;
		}

		self->sign = sign;
	}

	return (Object*) self;
}

static void BigInt_get(const Object* _self, va_list *ap)
{
	const BigInt *self = BIGINT(_self);

	char **ret = va_arg(*ap, char**);
	return_if_fail(ret != NULL);

	char *str = _BigInt_string(self);
	return_if_fail(str != NULL);

	*ret = str;
}

static void BigInt_string(const Stringer *_self, va_list *ap)
{
	const BigInt *self = BIGINT((const Object*) _self);

	char *str = _BigInt_string(self);
	return_if_fail(str != NULL);

	printf("%s", str);
	free(str);
}

/* }}} */

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

static int BigInt_cmp_int(const BigInt *a, int b)
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

static BigInt* BigInt_add_int(const BigInt *a, int b)
{
	BigInt *result;

	if (b == 0)
	{
		result = (BigInt*)object_copy((const Object*) a);
		return_val_if_fail(result != NULL, NULL);

		return result;
	}

	result = BigInt_add(a, BI_INT(b));
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

static BigInt* BigInt_sub_int(const BigInt *a, int b)
{
	BigInt *result = BigInt_sub(a, BI_INT(b));
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
		memmove(self->words + wlshift, self->words, self->length * sizeof(word_t));
		self->length += wlshift;
	}

	if (lshift == 0)
		return self;

	size_t sh = WORD_BIT - lshift;
	size_t i = wlshift;

	lword_t mask = (1ULL << lshift) - 1;
	lword_t r = 0;

	for (; i < self->length; ++i)
	{
		lword_t self_word = self->words[i]; 
		lword_t rr = (self_word >> sh) & mask;
		lword_t new_word = (self_word << lshift) | r;

		self->words[i] = (word_t) new_word;
		r = rr;
	}

	if (r != 0)
	{
		self->words[i] = (word_t) r;
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
			memset(self->words, 0, self->length * sizeof(word_t));
			self->length = 0;

			return self;
		}

		memmove(self->words, self->words + wrshift, (self->length - wrshift) * sizeof(word_t));
	}

	if (rshift == 0)
		return self;

	size_t sh = WORD_BIT - rshift;
	size_t i = self->length - wrshift - 1;

	lword_t mask = (1ULL << rshift) - 1;
	lword_t r = 0;

	for (; i >= 0; --i) 
	{
		lword_t self_word = self->words[i];
		lword_t rr = self_word & mask;
		lword_t new_word = (self_word >> rshift) | (r << sh);

		self->words[i] = (word_t) new_word;
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
		lword_t carry = 0;

		for (size_t j = 0; j < lo->length; ++j)
		{
			lword_t result_word = result->words[i + j];
			lword_t hi_word = hi->words[i];
			lword_t lo_word = lo->words[j];

			lword_t new_word = result_word + (hi_word * lo_word) + carry;

			carry = new_word >> WORD_BIT;
			result->words[i + j] = (word_t) new_word;
		}

		result->words[i + lo->length] = (word_t) carry;
	}

	_BigInt_clamp(result);

	result->sign = (a->sign + b->sign) % 2;

	return result;
}

static BigInt* BigInt_mul_int(const BigInt *a, int b)
{
	BigInt *result;

	if (a->length == 0 || b == 0)
	{
		result = BI_ZERO;
		return_val_if_fail(result != NULL, NULL);

		return result;
	}

	result = (BigInt*)object_new(BIGINT_TYPE, BI_INIT_SIZED, a->length + 1);
	return_val_if_fail(result != NULL, NULL);

	lword_t carry = 0;
	size_t i = 0;

	for (; i < a->length; ++i)
	{
		lword_t self_word = a->words[i];
		lword_t new_word = self_word * b + carry;

		carry = new_word >> WORD_BIT;
		result->words[i] = (word_t) new_word;
	}

	if (carry != 0)
	{
		result->length = a->length + 1;
		result->words[i] = carry;
	}
	else
		result->length = a->length;

	result->sign = (a->sign + ((b < 0) ? 1 : 0)) % 2;

	return result;
}

/* }}} */

/* Division {{{ */

static void BigInt_divrem(const BigInt *dividend, const BigInt *divisor, BigInt **ret_quot, BigInt **ret_rem)
{
	BigInt *quot, *rem;

	return_if_fail(divisor->length != 0);

	if (dividend->length == 0)
	{
		if (ret_quot != NULL)
		{
			quot = BI_ZERO;
			return_if_fail(quot != NULL);
		}

		if (ret_rem != NULL)
		{
			rem = BI_ZERO;
			if (rem == NULL)
			{
				if (ret_quot != NULL)
					object_delete((Object*) quot);
				return_if_fail(rem != NULL);
			}
		}

		if (ret_quot != NULL)
			*ret_quot = quot;

		if (ret_rem != NULL)
			*ret_rem = rem;

		return;
	}

	if (divisor->length == 1)
	{
		int dvsr = ((divisor->sign) ? -1 : 1) * divisor->words[0];
		BigInt_divrem_int(dividend, dvsr, ret_quot, ret_rem);
		return;
	}

	if (BigInt_cmp(dividend, divisor) < 0)
	{
		if (ret_quot != NULL)
		{
			quot = BI_ZERO;
			return_if_fail(quot != NULL);
		}

		if (ret_rem != NULL)
		{
			rem = (BigInt*)object_copy((const Object*) dividend);
			if (rem == NULL)
			{
				if (ret_quot != NULL)
					object_delete((Object*) quot);
				return_if_fail(rem != NULL);
			}
		}

		if (ret_quot != NULL)
			*ret_quot = quot;

		if (ret_rem != NULL)
			*ret_rem = rem;

		return;
	}

	word_t *q;
	word_t *r;
	word_t *u;
	word_t *v;

	size_t m = dividend->length;
	size_t n = divisor->length;

	quot = (BigInt*)object_new(BIGINT_TYPE, BI_INIT_SIZED, m);
	return_if_fail(quot != NULL);

	rem = (BigInt*)object_new(BIGINT_TYPE, BI_INIT_SIZED, m);
	if (rem == NULL)
	{
		object_delete((Object*) quot);
		return_if_fail(rem != NULL);
	}

	quot->length = m;
	rem->length = m;

	q = quot->words;
	r = rem->words;
	u = dividend->words;
	v = divisor->words;

	int err = _BigInt_divmnu(q, r, u, v, m, n);

	if (err != 0)
	{
		object_delete((Object*) quot);
		object_delete((Object*) rem);

		return_if_fail(err == 0);
	}

	if (ret_quot != NULL)
	{
		quot->sign = (divisor->sign + dividend->sign) % 2; 
		_BigInt_clamp(quot);
		*ret_quot = quot;
	}
	else
		object_delete((Object*) quot);

	if (ret_rem != NULL)
	{
		rem->sign = dividend->sign;
		_BigInt_clamp(rem);
		*ret_rem = rem;
	}
	else
		object_delete((Object*) rem);
}

static void BigInt_divrem_int(const BigInt *dividend, int divisor, BigInt **ret_quot, BigInt **ret_rem)
{
	BigInt *quot, *rem;

	return_if_fail(divisor != 0);

	if (dividend->length == 0)
	{
		if (ret_quot != NULL)
		{
			quot = BI_ZERO;
			return_if_fail(quot != NULL);
		}

		if (ret_rem != NULL)
		{
			rem = BI_ZERO;
			if (rem == NULL)
			{
				if (ret_quot != NULL)
					object_delete((Object*) quot);
				return_if_fail(rem != NULL);
			}
		}

		if (ret_quot != NULL)
			*ret_quot = quot;

		if (ret_rem != NULL)
			*ret_rem = rem;

		return;
	}

	int dsign = (divisor < 0) ? 1 : 0;

	if (dsign) 
		divisor *= -1;

	if (dividend->length == 1 && dividend->words[0] < divisor)
	{
		if (ret_quot != NULL)
		{
			quot = BI_ZERO;
			return_if_fail(quot != NULL);
		}

		if (ret_rem != NULL)
		{
			rem = (BigInt*)object_copy((const Object*) dividend);
			if (rem == NULL)
			{
				if (ret_quot != NULL)
					object_delete((Object*) quot);
				return_if_fail(rem != NULL);
			}
		}

		if (ret_quot != NULL)
			*ret_quot = quot;

		if (ret_rem != NULL)
			*ret_rem = rem;

		return;
	}

	word_t *q;
	word_t *u;
	word_t v;

	size_t m = dividend->length;

	quot = (BigInt*)object_new(BIGINT_TYPE, BI_INIT_SIZED, m);
	return_if_fail(quot != NULL);

	rem = (BigInt*)object_new(BIGINT_TYPE, BI_INIT_SIZED, 1);
	if (rem == NULL)
	{
		object_delete((Object*) quot);
		return_if_fail(rem != NULL);
	}

	quot->length = m;
	rem->length = 1;

	q = quot->words;
	u = dividend->words;
	v = divisor;

	rem->words[0] = _BigInt_divrem2in1(u, m, v, q);

	if (ret_quot != NULL)
	{
		quot->sign = (dsign + dividend->sign) % 2; 
		_BigInt_clamp(quot);
		*ret_quot = quot;
	}
	else
		object_delete((Object*) quot);

	if (ret_rem != NULL)
	{
		rem->sign = dividend->sign;
		_BigInt_clamp(rem);
		*ret_rem = rem;
	}
	else
		object_delete((Object*) rem);
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

BigInt* bi_new_sized(size_t size)
{
	return (BigInt*)object_new(BIGINT_TYPE, BI_INIT_SIZED, size);
}

BigInt* bi_copy(const BigInt *self)
{
	return_val_if_fail(IS_BIGINT(self), NULL);
	return (BigInt*)object_copy((const Object*) self);
}

BigInt* bi_set_int(BigInt *self, int value)
{
	return_val_if_fail(IS_BIGINT(self), NULL);
	return (BigInt*)object_set((Object*) self, BI_SET_INT, value);
}

BigInt* bi_set_str(BigInt *self, char *value)
{
	return_val_if_fail(IS_BIGINT(self), NULL);
	return (BigInt*)object_set((Object*) self, BI_SET_STR, value);
}

char* bi_get(const BigInt *self)
{
	return_val_if_fail(IS_BIGINT(self), NULL);

	char *ret = NULL;
	object_get((Object*) self, &ret);
	return_val_if_fail(ret != NULL, NULL);

	return ret;
}

void bi_delete(BigInt *self)
{
	return_if_fail(IS_BIGINT(self));
	object_delete((Object*) self);
}

BigInt* bi_lshift(BigInt *self, size_t shift)
{
	return_val_if_fail(IS_BIGINT(self), NULL);
	return BigInt_lshift(self, shift);
}

BigInt* bi_rshift(BigInt *self, size_t shift)
{
	return_val_if_fail(IS_BIGINT(self), NULL);
	return BigInt_rshift(self, shift);
}

int bi_cmp(const BigInt *a, const BigInt *b)
{
	return_val_if_fail(IS_BIGINT(a), -2);
	return_val_if_fail(IS_BIGINT(b), -2);
	return BigInt_cmp(a, b);
}

int bi_cmp_int(const BigInt *a, int b)
{
	return_val_if_fail(IS_BIGINT(a), -2);
	return BigInt_cmp_int(a, b);
}

BigInt* bi_add(const BigInt *a, const BigInt *b)
{
	return_val_if_fail(IS_BIGINT(a), NULL);
	return_val_if_fail(IS_BIGINT(b), NULL);
	return BigInt_add(a, b);
}

BigInt* bi_add_int(const BigInt *a, int b)
{
	return_val_if_fail(IS_BIGINT(a), NULL);
	return BigInt_add_int(a, b);
}

BigInt* bi_sub(const BigInt *a, const BigInt *b)
{
	return_val_if_fail(IS_BIGINT(a), NULL);
	return_val_if_fail(IS_BIGINT(b), NULL);
	return BigInt_sub(a, b);
}

BigInt* bi_sub_int(const BigInt *a, int b)
{
	return_val_if_fail(IS_BIGINT(a), NULL);
	return BigInt_sub_int(a, b);
}

BigInt* bi_mul(const BigInt *a, const BigInt *b)
{
	return_val_if_fail(IS_BIGINT(a), NULL);
	return_val_if_fail(IS_BIGINT(b), NULL);
	return BigInt_mul(a, b);
}

BigInt* bi_mul_int(const BigInt *a, int b)
{
	return_val_if_fail(IS_BIGINT(a), NULL);
	return BigInt_mul_int(a, b);
}

void bi_divrem(const BigInt *dividend, const BigInt *divisor, BigInt **quot, BigInt **rem)
{
	return_if_fail(IS_BIGINT(dividend));
	return_if_fail(IS_BIGINT(divisor));
	BigInt_divrem(dividend, divisor, quot, rem);
}

void bi_divrem_int(const BigInt *dividend, int divisor, BigInt **quot, BigInt **rem)
{
	return_if_fail(IS_BIGINT(dividend));
	BigInt_divrem_int(dividend, divisor, quot, rem);
}

BigInt* bi_div(const BigInt *dividend, const BigInt *divisor)
{
	return_val_if_fail(IS_BIGINT(dividend), NULL);
	return_val_if_fail(IS_BIGINT(divisor), NULL);

	BigInt *quot = NULL; 

	BigInt_divrem(dividend, divisor, &quot, NULL);
	return_val_if_fail(quot != NULL, NULL);

	return quot;
}

BigInt* bi_div_int(const BigInt *dividend, int divisor)
{
	return_val_if_fail(IS_BIGINT(dividend), NULL);

	BigInt *quot = NULL; 

	BigInt_divrem_int(dividend, divisor, &quot, NULL);
	return_val_if_fail(quot != NULL, NULL);

	return quot;
}

BigInt* bi_mod(const BigInt *dividend, const BigInt *divisor)
{
	return_val_if_fail(IS_BIGINT(dividend), NULL);
	return_val_if_fail(IS_BIGINT(divisor), NULL);

	BigInt *rem = NULL;

	BigInt_divrem(dividend, divisor, NULL, &rem);
	return_val_if_fail(rem != NULL, NULL);

	return rem;
}

BigInt* bi_mod_int(const BigInt *dividend, int divisor)
{
	return_val_if_fail(IS_BIGINT(dividend), NULL);

	BigInt *rem = NULL; 

	BigInt_divrem_int(dividend, divisor, NULL, &rem);
	return_val_if_fail(rem != NULL, NULL);

	return rem;
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
	OBJECT_CLASS(klass)->set = BigInt_set;
	OBJECT_CLASS(klass)->get = BigInt_get;
}

/* }}} */
