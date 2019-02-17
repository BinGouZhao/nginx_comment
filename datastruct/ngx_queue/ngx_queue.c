#include "ngx_queue.h"

ngx_queue_t *
ngx_queue_middle(ngx_queue_t *queue)
{
	ngx_queue_t *middle, *next;

	middle = ngx_queue_head(queue);

	if (middle == ngx_queue_last(queue)) {
		return middle;
	}

	next = ngx_queue_head(queue);

	// middle 减少一次, next 减少两次, 当next到last时，middle就到中间了
	// n/2 + 1
	for ( ;; ) {
		middle = ngx_queue_next(middle);
		
		next = ngx_queue_next(next);

		if (next == ngx_queue_last(queue)) {
			return middle;
		}

		next = ngx_queue_next(next);

		if (next == ngx_queue_last(queue)) {
			return middle;
		}
	}
}


/* the stable insertion sort */
// 稳定的插入排序
// 即从第二个元素开始, 进行插入排序

void 
ngx_queue_sort(ngx_queue_t *queue, 
	int (*cmp)(const ngx_queue_t *, const ngx_queue_t *))
{
	ngx_queue_t *q, *next, *prev;

	q = ngx_queue_head(queue);

	if (q == ngx_queue_last(queue)) {
		return;
	}

	for ( q = ngx_queue_next(q); q != ngx_queue_sentinel(queue); q = next) {
		prev = ngx_queue_prev(q);
		next = ngx_queue_next(q);

		ngx_queue_remove(q);

		do {
			if (cmp(prev, q) <= 0) {
				break;
			}

			prev = ngx_queue_prev(prev);
		} while (prev != ngx_queue_sentinel(queue));

		ngx_queue_insert_after(prev, q);
	}
}

