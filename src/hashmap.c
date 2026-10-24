#include "hashmap.h"

#include <stdlib.h>
#include <string.h>

uint64_t	hm_hash(const char *key);

static hm_entry *hm_find(
	const hashmap *map,
	const char *key,
	uint64_t hash
)
{
	size_t index = hash % map->capacity;

	for (size_t i = 0; i < map->capacity; i++)
	{
		hm_entry *entry = &map->entries[index];

		if (entry->state == HM_EMPTY)
			return NULL;

		if (entry->state == HM_OCCUPIED &&
			entry->hash == hash &&
			strcmp(entry->key, key) == 0)
		{
			return entry;
		}

		index = (index + 1) % map->capacity;
	}

	return NULL;
}

static int hm_insert(
	hashmap *map,
	char *key,
	void *value,
	uint64_t hash
)
{
	size_t index = hash % map->capacity;
	hm_entry *first_deleted = NULL;

	for (size_t i = 0; i < map->capacity; i++)
	{
		hm_entry *entry = &map->entries[index];

		if (entry->state == HM_OCCUPIED)
		{
			if (entry->hash == hash &&
				strcmp(entry->key, key) == 0)
			{
				entry->value = value;
				return 0;
			}
		}

		else if (entry->state == HM_DELETED)
		{
			if(!first_deleted)
				first_deleted = entry;
		}

		else
		{
			if (first_deleted)
				entry = first_deleted;

			if (entry->state == HM_DELETED)
				map->deleted--;

			entry->key = key;
			entry->value = value;
			entry->hash = hash;
			entry->state = HM_OCCUPIED;

			map->size++;

			return 0;
		}

		index = (index + 1) % map->capacity;
	}

	if (first_deleted)
	{
		first_deleted->key = key;
		first_deleted->value = value;
		first_deleted->hash = hash;
		first_deleted->state = HM_OCCUPIED;

		map->deleted--;
		map->size++;

		return 0;
	}

	return -1;
}

hashmap	*hm_create(size_t capacity)
{
	if (capacity < 8)
		capacity = 8;

	hashmap *map = malloc(sizeof(*map));

	if (!map)
		return NULL;

	map->entries = calloc(capacity, sizeof(hm_entry));

	if(!map->entries)
	{
		free(map);
		return NULL;
	}

	map->capacity = capacity;
	map->size = 0;
	map->deleted = 0;

	return map;
}

static int check_occupancy(hashmap *map)
{
	double occupancy_factor;
	size_t old_capacity;
	size_t new_capacity;
	hm_entry *old_entries;
	hm_entry *new_entries;

	occupancy_factor = (double)(map->size + map->deleted)
		/ (double)map->capacity;
	if (occupancy_factor < 0.70)
		return 0;
	if (map->capacity > SIZE_MAX / 2)
		return -1;
	old_capacity = map->capacity;
	new_capacity = old_capacity * 2;
	old_entries = map->entries;
	new_entries = calloc(new_capacity, sizeof(*new_entries));
	if (!new_entries)
		return -1;
	map->capacity = new_capacity;
	map->entries = new_entries;
	map->deleted = 0;
	map->size = 0;
	for (size_t i = 0; i < old_capacity; i++)
	{
		hm_entry entry = old_entries[i];

		if (entry.state == HM_OCCUPIED)
			hm_insert(map, entry.key, entry.value, entry.hash);
	}
	free(old_entries);
	return 0;
}

int		hm_set(hashmap *map, char *key, void *value)
{
	uint64_t hash;

	if (!map || !key)
		return -1;
	if (check_occupancy(map) == -1)
		return -1;
	hash = hm_hash(key);
	return hm_insert(map, key, value, hash);
}

void 	*hm_get(hashmap *map, char *key)
{
	if (!map || !key)
		return NULL;

	uint64_t hash = hm_hash(key);
	hm_entry *entry = hm_find(map, key, hash);

	if (!entry)
		return NULL;

	return entry->value;
}

int		hm_remove(hashmap *map, char *key)
{
	uint64_t hash;
	hm_entry *entry;

	if(!map || !key)
		return -1;
	hash = hm_hash(key);
	entry = hm_find(map, key, hash);
	if (!entry)
		return -1;
	entry->key = NULL;
	entry->value = NULL;
	entry->hash = 0;
	entry->state = HM_DELETED;
	map->size--;
	map->deleted++;
	return 0;
}

void	hm_destroy(hashmap *map)
{
	if (!map)
		return;
	free(map->entries);
	free(map);
}
