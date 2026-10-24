#include <stdint.h>

uint64_t	hm_hash(const char *key)
{
	uint64_t hash = UINT64_C(14695981039346656037);

	while (*key)
	{
		hash ^= (unsigned char)*key++;
		hash *= UINT64_C(1099511628211);
	}

	return hash;
}
