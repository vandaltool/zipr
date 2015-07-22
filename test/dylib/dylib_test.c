#include <stdio.h>
#include "dylib.h"

int main() {
	int ret = 0;
	ret += dynamic_function_a();
	ret += dynamic_function_b("b", 2, "a", "b");
	ret += dynamic_function_c(3);
	return ret;
}
