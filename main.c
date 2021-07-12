#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>

#include "Base.h"
#include "BigInt/BigInt.h"

int main(int argc, char *argv[])
{
	srand(time(0));

	unsigned int a = UINT_MAX;
	unsigned int b = UINT_MAX;

	BigInt *bi1 = bi_new_str("12340000000000000");
	BigInt *bi2 = bi_new_str("10000000000000000");

	BigInt *mul = bi_sub(bi1, bi2);

	bi_outputln(mul);
}
