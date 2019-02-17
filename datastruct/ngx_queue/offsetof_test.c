#include <stddef.h>
#include <stdio.h>

typedef struct test_node_s test_node_t;

struct test_node_s {
	char 			*str;
	int 			num;
};

void main() {
	int i;
	i = offsetof(test_node_t, num);
	printf("the offset is: %d.\n", i);

	i = offsetof(test_node_t, str);
	printf("the offset is: %d.\n", i);
}

