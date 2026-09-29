/* 삽입 정렬, 퀵 정렬, 힙 정렬의 성능을 비교한다. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
    int key;
    int original;
} Record;

typedef struct {
    unsigned long long compares;
    unsigned long long moves; /* 원소 대입 1회, 교환 3회 */
    size_t maxDepth;
} Stats;

typedef void (*SortFunction)(Record *, size_t, Stats *);

typedef struct {
    const char *name;
    SortFunction sort;
} Algorithm;

static int compare(const Record *a, const Record *b, Stats *s) {
    s->compares++;
    return (a->key > b->key) - (a->key < b->key);
}

static void swap(Record *a, Record *b, Stats *s) {
    if (a == b) return;
    Record temp = *a;
    *a = *b;
    *b = temp;
    s->moves += 3;
}

static void insertionSort(Record *a, size_t n, Stats *s) {
    s->maxDepth = 1;
    for (size_t i = 1; i < n; i++) {
        /* 앞 원소가 더 작으면 이미 올바른 위치에 있다. */
        if (compare(&a[i - 1], &a[i], s) <= 0) continue;
        Record value = a[i];
        s->moves++;
        size_t j = i;
        while (j > 0 && compare(&a[j - 1], &value, s) > 0) {
            a[j] = a[j - 1];
            s->moves++;
            j--;
        }
        a[j] = value;
        s->moves++;
    }
}

/* 마지막 원소를 피벗으로 선택한다. 범위는 [lo, hi)이다. */
static size_t partition(Record *a, size_t lo, size_t hi, Stats *s) {
    Record pivot = a[hi - 1];
    s->moves++;
    size_t i = lo;
    for (size_t j = lo; j < hi - 1; j++) {
        if (compare(&a[j], &pivot, s) <= 0) {
            swap(&a[i], &a[j], s);
            i++;
        }
    }
    swap(&a[i], &a[hi - 1], s);
    return i;
}

static void quickRange(Record *a, size_t lo, size_t hi, size_t depth, Stats *s) {
    if (depth > s->maxDepth) s->maxDepth = depth;
    if (hi - lo < 2) return;
    size_t p = partition(a, lo, hi, s);
    quickRange(a, lo, p, depth + 1, s);
    quickRange(a, p + 1, hi, depth + 1, s);
}

static void quickSort(Record *a, size_t n, Stats *s) {
    quickRange(a, 0, n, 1, s);
}

static void siftDown(Record *a, size_t root, size_t n, Stats *s) {
    while (root < n / 2) {
        size_t child = root * 2 + 1;
        if (child + 1 < n && compare(&a[child], &a[child + 1], s) < 0)
            child++;
        if (compare(&a[root], &a[child], s) >= 0) break;
        swap(&a[root], &a[child], s);
        root = child;
    }
}

static void heapSort(Record *a, size_t n, Stats *s) {
    s->maxDepth = 1; /* siftDown은 반복문으로 구현했다. */
    for (size_t i = n / 2; i > 0; i--) siftDown(a, i - 1, n, s);
    for (size_t end = n; end > 1; end--) {
        swap(&a[0], &a[end - 1], s);
        siftDown(a, 0, end - 1, s);
    }
}

enum InputKind { RANDOM, SORTED, REVERSED, DUPLICATES };

static const char *inputName(enum InputKind kind) {
    static const char *names[] = {"random", "sorted", "reversed", "duplicates"};
    return names[kind];
}

/* 입력을 재현할 수 있도록 시드를 고정한 난수를 사용한다. */
static unsigned nextRandom(unsigned *state) {
    *state = *state * 1664525u + 1013904223u;
    return *state;
}

static void makeInput(Record *a, size_t n, enum InputKind kind) {
    unsigned state = 20260929u;
    for (size_t i = 0; i < n; i++) {
        if (kind == RANDOM) a[i].key = (int)(nextRandom(&state) & 0x7fffffffu);
        else if (kind == SORTED) a[i].key = (int)i;
        else if (kind == REVERSED) a[i].key = (int)(n - i);
        else a[i].key = (int)(nextRandom(&state) % 8u);
        a[i].original = (int)i;
    }
}

static int isSorted(const Record *a, size_t n) {
    for (size_t i = 1; i < n; i++)
        if (a[i - 1].key > a[i].key) return 0;
    return 1;
}

static int isStable(const Record *a, size_t n) {
    for (size_t i = 1; i < n; i++)
        if (a[i - 1].key == a[i].key && a[i - 1].original > a[i].original)
            return 0;
    return 1;
}

int main(int argc, char **argv) {
    const Algorithm algorithms[] = {
        {"insertionSort", insertionSort},
        {"quickSort", quickSort},
        {"heapSort", heapSort}
    };
    const size_t sizes[] = {1000, 2000, 4000, 8000};
    const int repeats = 3;
    int csv = argc > 1 && strcmp(argv[1], "--csv") == 0;

    if (csv)
        puts("input,n,algorithm,millis,compares,moves,maxDepth,sorted,stable");
    else
        puts("input       n     algorithm       ms        compares      moves       depth sorted stable");

    for (int kind = RANDOM; kind <= DUPLICATES; kind++) {
        for (size_t si = 0; si < sizeof(sizes) / sizeof(sizes[0]); si++) {
            size_t n = sizes[si];
            Record *input = malloc(n * sizeof(*input));
            Record *work = malloc(n * sizeof(*work));
            if (input == NULL || work == NULL) {
                fprintf(stderr, "memory allocation failed\n");
                free(input);
                free(work);
                return 1;
            }
            makeInput(input, n, (enum InputKind)kind);
            for (size_t ai = 0; ai < sizeof(algorithms) / sizeof(algorithms[0]); ai++) {
                Stats stats = {0, 0, 0};
                clock_t ticks = 0;
                for (int rep = 0; rep < repeats; rep++) {
                    memcpy(work, input, n * sizeof(*work));
                    stats = (Stats){0, 0, 0};
                    clock_t start = clock();
                    algorithms[ai].sort(work, n, &stats);
                    ticks += clock() - start;
                }
                int sorted = isSorted(work, n);
                int stable = isStable(work, n);
                double ms = (double)ticks * 1000.0 / CLOCKS_PER_SEC / repeats;
                if (csv)
                    printf("%s,%zu,%s,%.3f,%llu,%llu,%zu,%d,%d\n",
                           inputName((enum InputKind)kind), n, algorithms[ai].name,
                           ms, stats.compares, stats.moves, stats.maxDepth,
                           sorted, stable);
                else
                    printf("%-11s %5zu %-14s %8.3f %14llu %10llu %11zu %6d %6d\n",
                           inputName((enum InputKind)kind), n, algorithms[ai].name,
                           ms, stats.compares, stats.moves, stats.maxDepth,
                           sorted, stable);
                if (!sorted || (ai == 0 && !stable)) {
                    fprintf(stderr, "sort check failed: %s\n", algorithms[ai].name);
                    free(input);
                    free(work);
                    return 1;
                }
            }
            free(input);
            free(work);
        }
    }
    return 0;
}
