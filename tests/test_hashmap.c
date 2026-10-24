#define _POSIX_C_SOURCE 200809L

#include "hashmap.h"

#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

typedef int (*test_function)(void);

typedef struct s_test_case
{
	const char		*name;
	test_function	function;
} test_case;

static int g_use_color;

#define COLOR_GREEN  (g_use_color ? "\033[32m" : "")
#define COLOR_RED    (g_use_color ? "\033[31m" : "")
#define COLOR_YELLOW (g_use_color ? "\033[33m" : "")
#define COLOR_BOLD   (g_use_color ? "\033[1m" : "")
#define COLOR_DIM    (g_use_color ? "\033[2m" : "")
#define COLOR_RESET  (g_use_color ? "\033[0m" : "")

#define EXPECT_TRUE(condition)                                                \
	do                                                                        \
	{                                                                         \
		if (!(condition))                                                       \
		{                                                                     \
			fprintf(stderr, "%s:%d: attendu: %s\n",                         \
				__FILE__, __LINE__, #condition);                                  \
			return 0;                                                           \
		}                                                                     \
	} while (0)

#define EXPECT_INT(actual, expected)                                         \
	do                                                                        \
	{                                                                         \
		int test_actual = (actual);                                            \
		int test_expected = (expected);                                        \
		if (test_actual != test_expected)                                      \
		{                                                                     \
			fprintf(stderr, "%s:%d: attendu %d, obtenu %d (%s)\n",          \
				__FILE__, __LINE__, test_expected, test_actual, #actual);         \
			return 0;                                                           \
		}                                                                     \
	} while (0)

#define EXPECT_SIZE(actual, expected)                                        \
	do                                                                        \
	{                                                                         \
		size_t test_actual = (actual);                                         \
		size_t test_expected = (expected);                                     \
		if (test_actual != test_expected)                                      \
		{                                                                     \
			fprintf(stderr, "%s:%d: attendu %zu, obtenu %zu (%s)\n",       \
				__FILE__, __LINE__, test_expected, test_actual, #actual);         \
			return 0;                                                           \
		}                                                                     \
	} while (0)

#define EXPECT_PTR(actual, expected)                                         \
	do                                                                        \
	{                                                                         \
		const void *test_actual = (actual);                                    \
		const void *test_expected = (expected);                                \
		if (test_actual != test_expected)                                      \
		{                                                                     \
			fprintf(stderr, "%s:%d: pointeur attendu %p, obtenu %p (%s)\n", \
				__FILE__, __LINE__, test_expected, test_actual, #actual);         \
			return 0;                                                           \
		}                                                                     \
	} while (0)

static uint64_t	reference_hash(const char *key)
{
	uint64_t hash;

	hash = UINT64_C(14695981039346656037);
	while (*key != '\0')
	{
		hash ^= (unsigned char)*key++;
		hash *= UINT64_C(1099511628211);
	}
	return hash;
}

static int	find_collisions(char keys[3][32], size_t capacity)
{
	size_t bucket;
	size_t found;
	unsigned int candidate;

	bucket = 0;
	found = 0;
	candidate = 0;
	while (candidate < 10000 && found < 3)
	{
		snprintf(keys[found], 32, "collision-%u", candidate);
		if (found == 0)
			bucket = (size_t)(reference_hash(keys[found]) % capacity);
		if ((size_t)(reference_hash(keys[found]) % capacity) == bucket)
			++found;
		++candidate;
	}
	return found == 3;
}

static int	test_create_minimum_capacity(void)
{
	hashmap *map;
	size_t i;

	map = hm_create(0);
	EXPECT_TRUE(map != NULL);
	EXPECT_TRUE(map->entries != NULL);
	EXPECT_SIZE(map->capacity, 8);
	EXPECT_SIZE(map->size, 0);
	EXPECT_SIZE(map->deleted, 0);
	i = 0;
	while (i < map->capacity)
	{
		EXPECT_INT(map->entries[i].state, HM_EMPTY);
		++i;
	}
	hm_destroy(map);
	return 1;
}

static int	test_create_requested_capacity(void)
{
	hashmap *map;

	map = hm_create(37);
	EXPECT_TRUE(map != NULL);
	EXPECT_SIZE(map->capacity, 37);
	EXPECT_SIZE(map->size, 0);
	hm_destroy(map);
	return 1;
}

static int	test_insert_and_get(void)
{
	hashmap *map;
	int alpha;
	int beta;

	alpha = 11;
	beta = 22;
	map = hm_create(8);
	EXPECT_TRUE(map != NULL);
	EXPECT_INT(hm_set(map, "alpha", &alpha), 0);
	EXPECT_INT(hm_set(map, "beta", &beta), 0);
	EXPECT_PTR(hm_get(map, "alpha"), &alpha);
	EXPECT_PTR(hm_get(map, "beta"), &beta);
	EXPECT_PTR(hm_get(map, "absente"), NULL);
	EXPECT_SIZE(map->size, 2);
	hm_destroy(map);
	return 1;
}

static int	test_empty_key(void)
{
	hashmap *map;
	int value;

	value = 7;
	map = hm_create(8);
	EXPECT_TRUE(map != NULL);
	EXPECT_INT(hm_set(map, "", &value), 0);
	EXPECT_PTR(hm_get(map, ""), &value);
	EXPECT_SIZE(map->size, 1);
	hm_destroy(map);
	return 1;
}

static int	test_update_existing_key(void)
{
	hashmap *map;
	int before;
	int after;

	before = 1;
	after = 2;
	map = hm_create(8);
	EXPECT_TRUE(map != NULL);
	EXPECT_INT(hm_set(map, "key", &before), 0);
	EXPECT_INT(hm_set(map, "key", &after), 0);
	EXPECT_PTR(hm_get(map, "key"), &after);
	EXPECT_SIZE(map->size, 1);
	hm_destroy(map);
	return 1;
}

static int	test_collision_lookup(void)
{
	hashmap *map;
	char keys[3][32];
	int values[3];
	size_t i;

	values[0] = 10;
	values[1] = 20;
	values[2] = 30;
	map = hm_create(8);
	EXPECT_TRUE(map != NULL);
	EXPECT_TRUE(find_collisions(keys, map->capacity));
	i = 0;
	while (i < 3)
	{
		EXPECT_INT(hm_set(map, keys[i], &values[i]), 0);
		++i;
	}
	i = 0;
	while (i < 3)
	{
		EXPECT_PTR(hm_get(map, keys[i]), &values[i]);
		++i;
	}
	hm_destroy(map);
	return 1;
}

static int	test_resize_preserves_entries(void)
{
	hashmap *map;
	char keys[16][24];
	int values[16];
	size_t i;

	map = hm_create(8);
	EXPECT_TRUE(map != NULL);
	i = 0;
	while (i < 16)
	{
		snprintf(keys[i], sizeof(keys[i]), "resize-%zu", i);
		values[i] = (int)(i * 3);
		EXPECT_INT(hm_set(map, keys[i], &values[i]), 0);
		++i;
	}
	EXPECT_TRUE(map->capacity > 8);
	EXPECT_SIZE(map->size, 16);
	i = 0;
	while (i < 16)
	{
		EXPECT_PTR(hm_get(map, keys[i]), &values[i]);
		++i;
	}
	hm_destroy(map);
	return 1;
}

static int	test_many_entries(void)
{
	hashmap *map;
	char keys[256][24];
	int values[256];
	size_t i;

	map = hm_create(8);
	EXPECT_TRUE(map != NULL);
	i = 0;
	while (i < 256)
	{
		snprintf(keys[i], sizeof(keys[i]), "entry-%zu", i);
		values[i] = (int)i;
		EXPECT_INT(hm_set(map, keys[i], &values[i]), 0);
		++i;
	}
	EXPECT_SIZE(map->size, 256);
	i = 0;
	while (i < 256)
	{
		EXPECT_PTR(hm_get(map, keys[i]), &values[i]);
		++i;
	}
	hm_destroy(map);
	return 1;
}

static int	test_remove_existing(void)
{
	hashmap *map;

	map = hm_create(8);
	EXPECT_TRUE(map != NULL);
	EXPECT_INT(hm_set(map, "remove-me", NULL), 0);
	EXPECT_INT(hm_remove(map, "remove-me"), 0);
	EXPECT_PTR(hm_get(map, "remove-me"), NULL);
	EXPECT_SIZE(map->size, 0);
	EXPECT_SIZE(map->deleted, 1);
	hm_destroy(map);
	return 1;
}

static int	test_remove_missing(void)
{
	hashmap *map;

	map = hm_create(8);
	EXPECT_TRUE(map != NULL);
	EXPECT_INT(hm_remove(map, "missing"), -1);
	EXPECT_SIZE(map->size, 0);
	EXPECT_SIZE(map->deleted, 0);
	hm_destroy(map);
	return 1;
}

static int	test_remove_keeps_probe_chain(void)
{
	hashmap *map;
	char keys[3][32];
	int second;
	int third;

	second = 2;
	third = 3;
	map = hm_create(8);
	EXPECT_TRUE(map != NULL);
	EXPECT_TRUE(find_collisions(keys, map->capacity));
	EXPECT_INT(hm_set(map, keys[0], NULL), 0);
	EXPECT_INT(hm_set(map, keys[1], &second), 0);
	EXPECT_INT(hm_set(map, keys[2], &third), 0);
	EXPECT_INT(hm_remove(map, keys[0]), 0);
	EXPECT_PTR(hm_get(map, keys[1]), &second);
	EXPECT_PTR(hm_get(map, keys[2]), &third);
	hm_destroy(map);
	return 1;
}

static int	test_reuse_tombstone(void)
{
	hashmap *map;
	char keys[3][32];
	int value;

	value = 42;
	map = hm_create(8);
	EXPECT_TRUE(map != NULL);
	EXPECT_TRUE(find_collisions(keys, map->capacity));
	EXPECT_INT(hm_set(map, keys[0], NULL), 0);
	EXPECT_INT(hm_remove(map, keys[0]), 0);
	EXPECT_SIZE(map->deleted, 1);
	EXPECT_INT(hm_set(map, keys[1], &value), 0);
	EXPECT_PTR(hm_get(map, keys[1]), &value);
	EXPECT_SIZE(map->size, 1);
	EXPECT_SIZE(map->deleted, 0);
	hm_destroy(map);
	return 1;
}

static int	test_null_arguments(void)
{
	hashmap *map;
	int value;

	value = 12;
	map = hm_create(8);
	EXPECT_TRUE(map != NULL);
	EXPECT_PTR(hm_get(NULL, "key"), NULL);
	EXPECT_PTR(hm_get(map, NULL), NULL);
	EXPECT_INT(hm_set(NULL, "key", &value), -1);
	EXPECT_INT(hm_set(map, NULL, &value), -1);
	EXPECT_INT(hm_remove(NULL, "key"), -1);
	EXPECT_INT(hm_remove(map, NULL), -1);
	hm_destroy(map);
	hm_destroy(NULL);
	return 1;
}

static const test_case g_tests[] = {
	{"création avec capacité minimale", test_create_minimum_capacity},
	{"création avec capacité demandée", test_create_requested_capacity},
	{"insertion et lecture", test_insert_and_get},
	{"clé vide", test_empty_key},
	{"mise à jour d'une clé", test_update_existing_key},
	{"résolution des collisions", test_collision_lookup},
	{"redimensionnement et conservation", test_resize_preserves_entries},
	{"charge de 256 entrées", test_many_entries},
	{"suppression d'une entrée", test_remove_existing},
	{"suppression d'une clé absente", test_remove_missing},
	{"suppression dans une chaîne de sondage", test_remove_keeps_probe_chain},
	{"réutilisation d'un tombstone", test_reuse_tombstone},
	{"arguments NULL", test_null_arguments},
};

static double	elapsed_ms(const struct timespec *start,
	const struct timespec *end)
{
	return (double)(end->tv_sec - start->tv_sec) * 1000.0
		+ (double)(end->tv_nsec - start->tv_nsec) / 1000000.0;
}

static const char	*signal_name(int signal_number)
{
	if (signal_number == SIGSEGV)
		return "segmentation fault";
	if (signal_number == SIGABRT)
		return "abort";
	if (signal_number == SIGBUS)
		return "bus error";
	if (signal_number == SIGFPE)
		return "floating-point exception";
	return "signal inconnu";
}

static int	run_one_test(const test_case *test, double *duration,
	int *signal_number)
{
	struct timespec start;
	struct timespec end;
	pid_t pid;
	int status;
	int success;

	fflush(NULL);
	clock_gettime(CLOCK_MONOTONIC, &start);
	if (getenv("HASHMAP_TEST_NO_FORK") != NULL)
	{
		success = test->function();
		clock_gettime(CLOCK_MONOTONIC, &end);
		*duration = elapsed_ms(&start, &end);
		return success;
	}
	pid = fork();
	if (pid == -1)
	{
		fprintf(stderr, "fork: %s\n", strerror(errno));
		return 0;
	}
	if (pid == 0)
		exit(test->function() ? EXIT_SUCCESS : EXIT_FAILURE);
	if (waitpid(pid, &status, 0) == -1)
	{
		fprintf(stderr, "waitpid: %s\n", strerror(errno));
		return 0;
	}
	clock_gettime(CLOCK_MONOTONIC, &end);
	*duration = elapsed_ms(&start, &end);
	if (WIFSIGNALED(status))
	{
		*signal_number = WTERMSIG(status);
		return 0;
	}
	return WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS;
}

static int	colors_enabled(int argc, char **argv)
{
	int i;

	if (getenv("NO_COLOR") != NULL || getenv("TERM") == NULL
		|| strcmp(getenv("TERM"), "dumb") == 0)
		return 0;
	i = 1;
	while (i < argc)
	{
		if (strcmp(argv[i], "--no-color") == 0)
			return 0;
		++i;
	}
	return isatty(STDOUT_FILENO) || getenv("FORCE_COLOR") != NULL;
}

static const char	*read_filter(int argc, char **argv)
{
	int i;

	i = 1;
	while (i + 1 < argc)
	{
		if (strcmp(argv[i], "--filter") == 0)
			return argv[i + 1];
		++i;
	}
	return NULL;
}

static int	print_special_command(int argc, char **argv)
{
	size_t i;

	if (argc == 2 && strcmp(argv[1], "--list") == 0)
	{
		i = 0;
		while (i < sizeof(g_tests) / sizeof(g_tests[0]))
		{
			printf("%s\n", g_tests[i].name);
			++i;
		}
		return 1;
	}
	if (argc == 2 && strcmp(argv[1], "--help") == 0)
	{
		printf("Usage: %s [--no-color] [--list] [--filter texte]\n", argv[0]);
		return 1;
	}
	return 0;
}

int	main(int argc, char **argv)
{
	const char *filter;
	size_t passed;
	size_t failed;
	size_t selected;
	size_t i;

	if (print_special_command(argc, argv))
		return EXIT_SUCCESS;
	g_use_color = colors_enabled(argc, argv);
	filter = read_filter(argc, argv);
	passed = 0;
	failed = 0;
	selected = 0;
	printf("\n%sHashmap — batterie de tests%s\n", COLOR_BOLD, COLOR_RESET);
	printf("%s───────────────────────────%s\n", COLOR_DIM, COLOR_RESET);
	i = 0;
	while (i < sizeof(g_tests) / sizeof(g_tests[0]))
	{
		double duration;
		int signal_number;
		int success;

		if (filter != NULL && strstr(g_tests[i].name, filter) == NULL)
		{
			++i;
			continue;
		}
		++selected;
		signal_number = 0;
		success = run_one_test(&g_tests[i], &duration, &signal_number);
		if (success)
		{
			printf("  %s✓%s %-43s %s%7.2f ms%s\n", COLOR_GREEN,
				COLOR_RESET, g_tests[i].name, COLOR_DIM, duration, COLOR_RESET);
			++passed;
		}
		else
		{
			printf("  %s✗%s %-43s %s%7.2f ms%s", COLOR_RED,
				COLOR_RESET, g_tests[i].name, COLOR_DIM, duration, COLOR_RESET);
			if (signal_number != 0)
				printf(" %s(%s)%s", COLOR_YELLOW,
					signal_name(signal_number), COLOR_RESET);
			printf("\n");
			if (getenv("GITHUB_ACTIONS") != NULL)
				printf("::error title=Test hashmap échoué::%s\n", g_tests[i].name);
			++failed;
		}
		++i;
	}
	printf("%s───────────────────────────%s\n", COLOR_DIM, COLOR_RESET);
	if (selected == 0)
	{
		printf("%sAucun test ne correspond au filtre.%s\n\n",
			COLOR_YELLOW, COLOR_RESET);
		return EXIT_FAILURE;
	}
	printf("%s%zu réussi%s, %s%zu échoué%s, %zu total\n\n",
		COLOR_GREEN, passed, COLOR_RESET, failed == 0 ? COLOR_GREEN : COLOR_RED,
		failed, COLOR_RESET, selected);
	return failed == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
