#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "bench-framework.h"
#include "benchstringio.h"

/* Minimum benchmark runtime in us */
static const int kMinBenchRuntime = 1000000;
static int benchmark_count = 0;
static int started = 0;
static bench_info bench_info_list[MAX_BENCHMARKS];
static run_data bench_run_data[MAX_BENCHMARKS];

enum run_model_t { kRunModelRepeated, kRunModelOnce };
static enum run_model_t run_model;

void RegisterBenchmark(char *name, bench_function entry, int param, int time_ref, bench_function setup, bench_function teardown) {
  if (benchmark_count == MAX_BENCHMARKS) {
    ReportStatus("Too many benchmarks. Increase MAX_BENCHMARKS in framework\n");
    exit(1);
  }
  bench_info_list[benchmark_count].name = name;
  bench_info_list[benchmark_count].run = entry;
  bench_info_list[benchmark_count].setup = setup;
  bench_info_list[benchmark_count].teardown = teardown;
  bench_info_list[benchmark_count].time_ref = time_ref;
  bench_info_list[benchmark_count].param = param;
  benchmark_count++;
}

void ClearBenchmarks() {
  for (int i = 0; i < benchmark_count; i++) free(bench_info_list[i].name);
  benchmark_count = 0;
}

static int RunOne(bench_info *bench, run_data *data) {
  struct timeval start, end;
  int diff;
  if (bench->setup) bench->setup(bench->param);
  if (run_model == kRunModelRepeated) {
    /* run one iteration to warm up the cache (if v8 can JIT off the clock,
       then we can do this) */
    bench->run(bench->param);
  }
  gettimeofday(&start, NULL);
  if (run_model == kRunModelRepeated) {
    for (data->runs = 0; data->elapsed < kMinBenchRuntime || data->runs < 16;
         data->runs++) {
      assert(bench->run(bench->param) == 0);
      gettimeofday(&end, NULL);
      diff = (end.tv_sec - start.tv_sec) * 1000000 + 
        (end.tv_usec - start.tv_usec);
      data->elapsed = diff;
    }
  } else {
    assert(bench->run(bench->param) == 0);
    gettimeofday(&end, NULL);
    diff = (end.tv_sec - start.tv_sec) * 1000000 + 
      (end.tv_usec - start.tv_usec);
    data->elapsed = diff;
    data->runs = 1;
  }
  if (bench->teardown) bench->teardown(bench->param);
  return 0;
}

static void RunAll() {
  int i;
  for (i = 0; i < benchmark_count; i++) {
    run_data *rd = &bench_run_data[i];
    bench_info *bi = &bench_info_list[i];
    double usec_per_run;
    printf("Running %s\n", bi->name);
    ++started;
    ReportStatus("Running %s (%d/%d)\n", bi->name, started, benchmark_count);
    RunOne(bi, rd);
    usec_per_run = (double)rd->elapsed / (double)rd->runs;
    rd->score = 100.0 * bi->time_ref / usec_per_run;
    printf("usec_per_run %f\n", usec_per_run);
    ReportStatus("%s: %d", bi->name, (int)rd->score);
  }
  started = 0;
}

static void PrintScores() {
  int i;
  for (i = 0; i < benchmark_count; i++) {
    printf("Benchmark %s: usec %d, iters %d, usec/run %d score %.2f\n", 
           bench_info_list[i].name,
           bench_run_data[i].elapsed,
           bench_run_data[i].runs,
           bench_run_data[i].elapsed / bench_run_data[i].runs,
           bench_run_data[i].score);
  }
}

static double GeometricMean() {
  int i;
  double log_total = 0.0;
  for (i = 0; i < benchmark_count; i++) {
    log_total += log(bench_run_data[i].score);
  }
  return pow(M_E, log_total / benchmark_count);
}

void SetupSmallBenchmarks() {
  RegisterBenchmark(strdup("Richards"), run_richards, 10000, 2499257,
                    NULL, NULL);
  RegisterBenchmark(strdup("Deltablue"), run_deltablue, 100, 429919,
                    NULL, NULL);
  RegisterBenchmark(strdup("Fannkuchredux"), run_fannkuch, 10, 64052288,
                    NULL, NULL);
  RegisterBenchmark(strdup("Nbody"), run_nbody, 1000000, 73000000,
                    NULL, NULL);
  RegisterBenchmark(strdup("Spectralnorm"), run_spectralnorm, 350, 150020779,
                    NULL, NULL);
  RegisterBenchmark(strdup("Fasta"), run_fasta, 10000, 51667385, NULL, NULL);
  RegisterBenchmark(strdup("Revcomp"), run_revcomp, 0, 23542857, NULL, NULL);
  RegisterBenchmark(strdup("Binarytrees"), run_binarytrees, 15, 383306452,
                    NULL, NULL);
  RegisterBenchmark(strdup("Knucleotide"), run_knucleotide, 0, 433893130,
                    NULL, NULL);
  RegisterBenchmark(strdup("FFT"), run_fft, 1024, 50000000,
                    setup_fft, teardown_fft);
  RegisterBenchmark(strdup("Pidigits"), run_pidigits, 1000, 406976744,
                    NULL, NULL);
  run_model = kRunModelRepeated;
}

void SetupLargeBenchmarks() {
  RegisterBenchmark(strdup("Richards"), run_richards, 1000000, 2499257,
                    NULL, NULL);
  RegisterBenchmark(strdup("Deltablue"), run_deltablue, 10000, 429919,
                    NULL, NULL);
  RegisterBenchmark(strdup("Fannkuchredux"), run_fannkuch, 11, 64052288,
                    NULL, NULL);
  RegisterBenchmark(strdup("Nbody"), run_nbody, 10000000, 73000000,
                    NULL, NULL);
  RegisterBenchmark(strdup("Spectralnorm"), run_spectralnorm, 5500, 150020779,
                    NULL, NULL);
  RegisterBenchmark(strdup("Fasta"), run_fasta, 3000000, 51667385, NULL, NULL);
  RegisterBenchmark(strdup("Revcomp"), run_revcomp, 0, 23542857, NULL, NULL);
  RegisterBenchmark(strdup("Binarytrees"), run_binarytrees, 18, 383306452,
                    NULL, NULL);
  RegisterBenchmark(strdup("Knucleotide"), run_knucleotide, 0, 433893130,
                    NULL, NULL);
  RegisterBenchmark(strdup("FFT"), run_fft, 1024*1024, 50000000,
                    setup_fft, teardown_fft);
  RegisterBenchmark(strdup("Pidigits"), run_pidigits, 5000, 406976744,
                    NULL, NULL);
  run_model = kRunModelOnce;
}


/* must be protected with a mutex */
int framework_main(enum benchmark_size_t size) {
  int score;
  memset(bench_info_list, 0, sizeof(bench_info_list));
  memset(bench_run_data, 0, sizeof(bench_run_data));

#ifdef ARRAYFILE
  arrayfile_stdout = arrayfile_fopen("arrayfile", "w");
  assert(arrayfile_stdout);
#endif

  if (size == kBenchmarkSmall) {
    SetupSmallBenchmarks();
  } else {
    SetupLargeBenchmarks();
  }

  fasta_10k_ref_output_len = strlen(fasta_10k_ref_output);

  printf("%d benchmarks registered\n", benchmark_count);
  RunAll();
  PrintScores();
  score = (int) GeometricMean();
#ifdef ARRAYFILE
  arrayfile_fclose(arrayfile_stdout);
  arrayfile_stdout = NULL;
#endif
  ClearBenchmarks();
  printf("Aggregate score: %d\n", score);
  return score;
}
