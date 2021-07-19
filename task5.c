#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "Base.h"
#include "DataStructs/BigInt.h"

int main(int argc, char *argv[])
{
	srand(time(0));

	BigInt *bi1 = bi_new_str("1231654981236789");
	BigInt *bi2 = bi_new_int(rand());

	printf("bi1:\n");
	bi_outputln(bi1);

	printf("bi2:\n");
	bi_outputln(bi2);

	printf("bi1 + bi2:\n");
	BigInt *add = bi_add(bi1, bi2);
	bi_outputln(add);

	printf("bi1 * bi2:\n");
	BigInt *mul = bi_mul(bi1, bi2);
	bi_outputln(mul);

	printf("bi1 - bi2:\n");
	BigInt *sub = bi_sub(bi1, bi2);
	bi_outputln(sub);

	printf("bi1 / bi2:\n");
	BigInt *div = bi_div(bi1, bi2);
	bi_outputln(div);

	printf("bi1 %% bi2:\n");
	BigInt *mod = bi_div(bi1, bi2);
	bi_outputln(mod);

	bi_delete(bi1);
	bi_delete(bi2);
	bi_delete(add);
	bi_delete(mul);
	bi_delete(sub);
	bi_delete(div);
	bi_delete(mod);

	return 0;
}
