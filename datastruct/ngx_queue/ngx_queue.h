#ifndef _NGX_QUEUE_H_INCLUDE_
#define	_NGX_QUEUE_H_INCLUDE_

typedef struct ngx_queue_s ngx_queue_t;
typedef unsigned char u_char;

struct ngx_queue_s {
	ngx_queue_t *prev;
	ngx_queue_t *next;
};

#define ngx_queue_init(q)		\
	(q)->next = q;				\
	(q)->prev = q

// 判断是否为空
#define ngx_queue_empty(h)	(h == (h)->prev)

// h 表示的是链表容器, h->next 才是头元素
#define ngx_queue_insert_head(h, x)		\
	(x)->next = (h)->next;				\
	(x)->next->prev = x;				\
	(x)->prev = h;						\
	(h)->next = x

#define ngx_queue_insert_after ngx_queue_insert_head

#define ngx_queue_insert_tail(h, x)		\
	(x)->prev = (h)->prev;				\
	(x)->prev->next = x;				\
	(x)->next = h;						\
	(h)->prev = x

#define ngx_queue_head(h)	(h)->next

#define ngx_queue_last(h)	(h)->prev

#define ngx_queue_sentinel(h) (h)

#define ngx_queue_next(q)	(q)->next

#define ngx_queue_prev(q)	(q)->prev

#define ngx_queue_remove(x) 			\
	x->prev->next = x->next;			\
	x->next->prev = x->prev;

// 将h以 q 为分界切割成h, n两部分, q是n的首节点
#define ngx_queue_split(h, q, n)		\
	(n)->prev = (h)->prev;				\
	(n)->prev->next = n;				\
	(n)->next = q;						\
	(h)->prev = (q)->prev;				\
	(h)->prev->next = h;				\
	(q)->prev = n;

// 将h, n两个链表连在一起
#define ngx_queue_add(h, n)				\
	(h)->prev->next = (n)->next;		\
	(n)->next->prev = (h)->prev;		\
	(h)->prev =	(n)->prev;				\
	(h)->prev->next = h;

// 通过 ngx_queue 的地址反推原结构体的地址
// 666, 厉害厉害
#define ngx_queue_data(q, type, link)   \
    (type *) ((u_char *) q - offsetof(type, link))

ngx_queue_t *ngx_queue_middle(ngx_queue_t *queue);
void ngx_queue_sort(ngx_queue_t *queue,
	int (*cmp)(const ngx_queue_t *, const ngx_queue_t *));

#endif

