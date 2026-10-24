#ifndef HASHMAP_H
#define HASHMAP_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
	HM_EMPTY,
	HM_OCCUPIED,
	HM_DELETED
} hm_entry_state;

typedef struct {
	char			*key;
	void			*value;
	uint64_t		hash;
	hm_entry_state	state;
} hm_entry;

typedef struct {
	hm_entry	*entries;
	size_t		capacity;
	size_t		size;
	size_t		deleted;
} hashmap;

hashmap *hm_create(size_t capacity);
void	hm_destroy(hashmap *map);

int		hm_set(hashmap *map, char *key, void *value);
void	*hm_get(hashmap *map, char *key);
int		hm_remove(hashmap *map, char *key);

#endif
