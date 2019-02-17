#include <stdio.h>
#include <stddef.h>
#include "ngx_queue.c"

typedef struct test_node_s test_node_t;

struct test_node_s {
	ngx_queue_t		q;
	int 			num;
};

int 
compTestNode(const ngx_queue_t *a, const ngx_queue_t *b)
{
	test_node_t *n1 = ngx_queue_data(a, test_node_t, q);
	test_node_t *n2 = ngx_queue_data(b, test_node_t, q);

	return n1->num > n2->num;
}

void
print_test_node(ngx_queue_t *queue) 
{
	ngx_queue_t *tmp_q;
	for (tmp_q = ngx_queue_head(queue); tmp_q != ngx_queue_sentinel(queue);
			tmp_q = ngx_queue_next(tmp_q)) {
		test_node_t *a = ngx_queue_data(tmp_q, test_node_t, q);

		printf("%d ", a->num);
	}
}

void main() {
	ngx_queue_t queue;
	ngx_queue_init(&queue);
	
	test_node_t nodes[10];
	int i;
	for (i = 0; i < 10; i++) {
		nodes[i].num = i;
	}

	ngx_queue_insert_tail(&queue, &nodes[0].q);
	ngx_queue_insert_head(&queue, &nodes[1].q);
	ngx_queue_insert_tail(&queue, &nodes[2].q);
	ngx_queue_insert_after(&queue, &nodes[3].q);
	ngx_queue_insert_tail(&queue, &nodes[4].q);

	print_test_node(&queue);

	ngx_queue_sort(&queue, compTestNode);
	printf("\n sorted.\n");

	print_test_node(&queue);
}
