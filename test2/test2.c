// Quality test for the traces
//
// We are looking in details into the distribution of the latency of
// trace calls.

#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/spdr/spdr.h"

static struct SPDR_Context * g_spdr_context;

#define trace(cat, name) SPDR_EVENT(g_spdr_context, cat, name)

// Xorshift RNGs, George Marsaglia
static inline uint32_t xorshift32(uint32_t *state) {
    // Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs"
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return *state = x;
}

static inline void record_overhead(uint64_t cycles);
    
typedef struct {
    const char** strings;
    uint32_t count;
    uint32_t mask; // For fast power-of-two indexing
} TraceDictionary;

static inline void recorded_trace_call(const char* cat, const char* name);

// The "Work" function that simulates your handwritten "Call Tree"
void simulate_work_tree(uint32_t depth, uint32_t max_depth, 
                        TraceDictionary* dict, uint32_t* rng_state) {
    if (depth >= max_depth) return;

    // Pick a "Category" and "Name" from our test dictionary
    const char* cat = dict->strings[xorshift32(rng_state) & dict->mask];
    const char* name = dict->strings[xorshift32(rng_state) & dict->mask];

    recorded_trace_call(cat, name);

    // Simulate "Work Chunk"
    volatile int dummy = 0;
    for(int i = 0; i < 100; ++i) dummy += i;

    // Recurse to simulate the "Down call trees" from your notes
    simulate_work_tree(depth + 1, max_depth, dict, rng_state);
}


// Linus: The "L3 Stomper"
void* chaos_thread_entry(void* arg) {
    size_t l3_size = (size_t)arg; 
    size_t buffer_size = l3_size * 2;
    uint8_t* poison = malloc(buffer_size);
    
    // Large prime stride to defeat hardware prefetchers
    size_t stride = 503; 
    size_t addr = 0;

    while (1) {
        // Force a read and a write to every cache line
        poison[addr] += 1;
        addr = (addr + stride) % buffer_size;
        
        // Occasional compiler barrier to prevent loop optimization
        __asm__ volatile("" : : : "memory");
    }
    return NULL;
}


#define HISTOGRAM_BUCKETS 64
static _Atomic uint64_t g_latency_hist[HISTOGRAM_BUCKETS];

static inline void record_overhead(uint64_t cycles) {
    // Per: Find the highest bit set to get a logarithmic bucket
    // This lets us see 10 cycles and 10,000,000 cycles in one table
    uint32_t bucket = (cycles > 0) ? (63 - __builtin_clzll(cycles)) : 0;
    if (bucket >= HISTOGRAM_BUCKETS) bucket = HISTOGRAM_BUCKETS - 1;
    
    atomic_fetch_add_explicit(&g_latency_hist[bucket], 1, memory_order_relaxed);
}

// Casey: Track the "worst" call to see if it's deterministic
static uint64_t g_max_cycles = 0;
static uint32_t g_max_call_index = 0;
static uint32_t g_call_counter = 0;

static inline void recorded_trace_call(const char* cat, const char* name) {  
    uint32_t idx = g_call_counter++;

    __asm__ volatile("" : : : "memory");    
    uint64_t start = __builtin_readcyclecounter();
    trace(cat, name);
    
    uint64_t end = __builtin_readcyclecounter();
    __asm__ volatile("" : : : "memory");    
    
    uint64_t diff = end - start;
    record_overhead(diff);
    
    if (diff > g_max_cycles) {
        g_max_cycles = diff;
        g_max_call_index = idx;
    }
}

static void reset_latency_hist() {
    memset(g_latency_hist, 0, sizeof(g_latency_hist));
}


static void print_percentiles() {
    uint32_t total_calls = g_call_counter;
    uint32_t target_p999 = (uint32_t)(total_calls * 0.999);
    uint32_t target_p95 = (uint32_t)(total_calls * 0.95);
    uint32_t cumulative = 0;
    
    int p95_bucket = -1;
    int p999_bucket = -1;

    for (int i = 0; i < HISTOGRAM_BUCKETS; i++) {
        cumulative += g_latency_hist[i];
        
        if (p95_bucket == -1 && cumulative >= target_p95) {
            p95_bucket = i;
        }
        if (p999_bucket == -1 && cumulative >= target_p999) {
            p999_bucket = i;
            break; 
        }
    }
    printf("P95: Bucket %d (%llu - %llu cycles)\n", 
           p95_bucket, 1ULL << p95_bucket, (1ULL << (p95_bucket + 1)) - 1);
    printf("P99.9: Bucket %d (%llu - %llu cycles)\n", 
           p999_bucket, 1ULL << p999_bucket, (1ULL << (p999_bucket + 1)) - 1);
}

static void print_latency_hist() {
    printf("Latency:\n");

    // Find last bucket with some data, to avoid printing irrelevant buckets.
    size_t last_bucket = HISTOGRAM_BUCKETS;
    for (size_t ri = HISTOGRAM_BUCKETS; ri > 0; ri--) {
      if (g_latency_hist[ri - 1] == 0) {
        last_bucket--;
        continue;
      }
      break;
    }
    for (size_t i = 0, n = last_bucket; i < n; i++) {
      printf("%zu([%zu..%zu]): %llu\n", i, 1ull<<i, (1ull<<(i+1)) - 1, (unsigned long long) g_latency_hist[i]);
    }

    printf("Outliers:\n");
    printf("g_max_cycles: %zu\n", g_max_cycles);
    printf("g_max_call_index: %u\n", g_max_call_index);
    printf("g_call_counter: %u\n", g_call_counter);

    print_percentiles();
}


// The main harness loop
void run_harness_iteration(TraceDictionary* dict, uint32_t* seed) {
    // burn in, let's initialize the trace library
    for (int i = 0; i < 100; i++) {
        const char* cat = dict->strings[0];
        const char* name = dict->strings[0];
        trace(cat, name); // NOT measured
    }

    g_max_cycles = 0;
    g_max_call_index = 0;
    g_call_counter = 0;
       
    uint32_t work_iters = 100*1000;
    for (uint32_t i = 0; i < work_iters; i++) {
        // This is your "Work Chunk" from the drawing
        simulate_work_tree(0, 5, dict, seed); 
    }
}

#if 0
#define _GNU_SOURCE
#include <pthread.h>
#include <sched.h>

void pin_to_core(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    pthread_t current_thread = pthread_self();
    pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}

// Harness Runner: Orchestrates the Workers and the Chaos
void launch_test_suite(int num_workers, size_t dictionary_size, size_t l3_size) {
    // 1. Prepare the Dictionary (Tunable size)
    TraceDictionary dict = prepare_dictionary(dictionary_size);
    
    // 2. Spawn the L3 Stomper on the last core
    pthread_t chaos_tid;
    pthread_create(&chaos_tid, NULL, chaos_thread_entry, (void*)l3_size);
    // Pin to a distant core to focus purely on L3/Memory contention
    // (Assuming a 16-core system for this example)
    cpu_set_t chaos_set;
    CPU_ZERO(&chaos_set);
    CPU_SET(15, &chaos_set);
    pthread_setaffinity_np(chaos_tid, sizeof(cpu_set_t), &chaos_set);

    // 3. Spawn the Workers
    pthread_t workers[num_workers];
    for (int i = 0; i < num_workers; i++) {
        WorkerArgs* args = malloc(sizeof(WorkerArgs));
        args->core_id = i;
        args->dict = &dict;
        pthread_create(&workers[i], NULL, worker_main, args);
    }
    
    // ... wait and then dump the g_latency_hist ...
}
#endif

static inline bool is_power_of_two(size_t x) {
    return (x & (x - 1)) == 0;
}

TraceDictionary prepare_dictionary(size_t dictionary_size) {
    assert(is_power_of_two(dictionary_size));
    size_t string_size = 8;

    char * memory = malloc((string_size + 1) * dictionary_size);

    const char ** strings;
    strings = malloc(dictionary_size * sizeof *strings);

    char * memory_next = memory;
    for (size_t i = 0, n = dictionary_size; i < n; i++) {
      char * str = memory_next;
      size_t len = string_size;
      str[len] = 0;
      memory_next += len + 1;

      strings[i] = str;
    }

    return (TraceDictionary) {
        .strings = strings,
        .count = dictionary_size,
        .mask = dictionary_size - 1
    };
}


#include <string.h>

// Explicitly touch every page to force physical allocation
void prewarm_buffer(void* buffer, size_t size) {
    volatile uint8_t* p = (uint8_t*)buffer;
    size_t page_size = 4096; // Standard OS page
    
    for (size_t i = 0; i < size; i += page_size) {
        // Read-Modify-Write to ensure the page is "Dirty" and resident
        p[i] = p[i]; 
    }
}

// Pull the dictionary strings into the cache hierarchy
void prewarm_dictionary(TraceDictionary* dict) {
    for (uint32_t i = 0; i < dict->count; i++) {
        const char* s = dict->strings[i];
        if (s) {
            // Just read the first byte to trigger the TLB/Cache fetch
            volatile char c = s[0];
            (void)c;
        }
    }
}

void print_harness_baseline() {
    printf("--- Harness Baseline ---\n");
    printf("We print an histogram of the time it takes to record an empty trace call, as the baseline for our test harness."
           " This should tell us the kind of outliers we're ready to receive\n");
    
    g_call_counter = 0;
    g_max_cycles = 0;

    reset_latency_hist();

    for (int i = 0; i < 15000*3; i++) {
        uint32_t idx = g_call_counter++;
        
        // We use dummy strings to ensure the registers are loaded 
        // exactly like a real call would be.
        const char* d1 = "category";
        const char* d2 = "name";

        __asm__ volatile("" : : : "memory");    
        uint64_t start = __builtin_readcyclecounter();
        // trace(d1, d2); // This is the empty macro
        uint64_t end = __builtin_readcyclecounter();
        __asm__ volatile("" : : : "memory");    

        uint64_t diff = end - start;
        record_overhead(diff);
    
        if (diff > g_max_cycles) {
            g_max_cycles = diff;
            g_max_call_index = idx;
        }
    }
    print_latency_hist();
    printf("Max baseline cycles: %llu\n\n", g_max_cycles);
}

void thread_set_realtime_priority();
void thread_pin_to_core(int);

int main() {
    size_t spdr_buffer_size = 10000000;
    char * spdr_buffer = malloc(spdr_buffer_size);
    prewarm_buffer(spdr_buffer, spdr_buffer_size);

    if (spdr_init(&g_spdr_context, spdr_buffer, spdr_buffer_size)) {
        assert(false);
        return 1;
    }

    size_t dictionary_size = 1024;
    uint32_t prngseed = 0xdead;
    TraceDictionary dict = prepare_dictionary(dictionary_size);
    prewarm_dictionary(&dict);

    thread_set_realtime_priority();
    thread_pin_to_core(0);

    print_harness_baseline();
    
    // Enable tracing. We should be testing also without tracing on.
    printf("# With tracing on:\n");
    printf("prngseed: %#x\n", prngseed);
    {
        spdr_enable_trace(g_spdr_context, 1);
        
        reset_latency_hist();
        
        
        run_harness_iteration(&dict, &prngseed);
        
        print_latency_hist();
    }
    
    reset_latency_hist();
    
    printf("# Without tracing on:\n");
    prngseed = 0xdead;
    printf("prngseed: %#x\n", prngseed);
    {
        struct SPDR_Context * save = g_spdr_context;
        spdr_reset(g_spdr_context);    
        spdr_enable_trace(g_spdr_context, 0);
        //g_spdr_context = NULL;
        
        run_harness_iteration(&dict, &prngseed);
        
        print_latency_hist();
        
        g_spdr_context = save;
    }
    
    return 0;

}
#include "../src/allocator.c"
#include "../src/chars.c"
#include "../src/chars_windows.c"
#include "../src/clock.c"
#include "../src/clock_windows.c"
#include "../src/float_windows.c"
#include "../src/spdr2.c"
#include "../src/spdr_windows.c"


#define WIN32_LEAN_AND_MEAN
#include <windows.h>

void thread_set_realtime_priority() {
    BOOL Success;
    Success = SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
    assert(Success);
    Success = SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    assert(Success);
}

void thread_pin_to_core(int core_index) {
    DWORD_PTR PreviousMask = SetThreadAffinityMask(GetCurrentThread(), 1 << core_index);
    assert(PreviousMask != NULL);
}
