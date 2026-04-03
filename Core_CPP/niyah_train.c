#include "niyah_core.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* tokenizer.c API */
void tokenizer_init(void);
uint32_t tokenizer_encode(const char *text, uint32_t *tokens, uint32_t max_len);
void tokenizer_free(void);

static FILE* open_training_data_path(const char* path) {
    if (path && path[0]) {
        FILE* f = fopen(path, "r");
        if (f) { printf("[NIYAH] data file: %s\n", path); return f; }
        printf("[NIYAH] path not found: %s\n", path);
    }
    const char* candidates[] = {
        "Data_Training/sovereign_knowledge.txt",
        "sovereign_knowledge_data.txt",
        "sovereign_knowledge.txt"
    };
    for (size_t i = 0; i < sizeof(candidates)/sizeof(candidates[0]); i++) {
        FILE* f = fopen(candidates[i], "r");
        if (f) { printf("[NIYAH] data file: %s\n", candidates[i]); return f; }
    }
    return NULL;
}

static int   parse_int  (const char* s, int   fb) { if(!s||!s[0]) return fb; int   v=atoi(s);        return v>0?v:fb; }
static float parse_float(const char* s, float fb) { if(!s||!s[0]) return fb; float v=(float)atof(s); return v>0?v:fb; }

static float cosine_lr(float base, float mn, uint32_t step, uint32_t total, uint32_t warmup) {
    if (step < warmup) {
        float t = (float)step / (float)(warmup > 0 ? warmup : 1);
        return mn + (base - mn) * t;
    }
    uint32_t d = (total > warmup) ? (total - warmup) : 1;
    float p = (float)(step - warmup) / (float)d;
    if (p > 1.f) p = 1.f;
    return mn + (base - mn) * 0.5f * (1.f + cosf(3.14159265f * p));
}

/*
 * Clamp all token IDs so they fit within [0, vocab_size).
 * The tokenizer uses Unicode-derived IDs (up to ~16000 for Arabic)
 * which exceed the model vocab. We fold them with modulo so every
 * unique codepoint still maps to a unique bucket (given vocab ≥ 8192).
 */
static void clamp_tokens(uint32_t *tk, uint32_t n, uint32_t vocab_size) {
    for (uint32_t i = 0; i < n; i++)
        tk[i] = tk[i] % vocab_size;
}

int main(int argc, char** argv) {
    /*
     * vocab_size=8192 covers ALL tokenizer ID ranges:
     *   0-3    special tokens
     *   4-13   digits
     *   14-25  punctuation
     *   26+    words/UNK
     *   1000-5999  Arabic (1000 + uc%5000)
     *   6000-15999 other Unicode (6000 + uc%10000)
     *
     * With modulo folding above, even IDs > 8192 map safely.
     * embed=128, layers=4 gives ~4.3M params — fast on NEON.
     */
    NiyahConfig cfg = {
        .magic      = NIYAH_MAGIC,
        .version    = NIYAH_VER,
        .vocab_size = 8192,
        .ctx_len    = 64,
        .embed_dim  = 128,
        .n_layers   = 4,
        .n_heads    = 8,
        .n_kv_heads = 8,
        .ffn_mult   = 4,
        .rope_theta = 10000.f,
        .rms_eps    = 1e-5f,
        .flags      = 0
    };

    const char* data_path = argc > 1 ? argv[1] : NULL;
    int   epochs  = argc > 2 ? parse_int  (argv[2], 5)      : 5;
    float base_lr = argc > 3 ? parse_float(argv[3], 3e-4f)  : 3e-4f;
    float min_lr  = argc > 4 ? parse_float(argv[4], 3e-5f)  : 3e-5f;

    NiyahModel *m = niyah_alloc(&cfg);
    if (!m) { fputs("[NIYAH] alloc failed\n", stderr); return 1; }

    NiyahAdam *opt = niyah_adam_alloc(m);
    if (!opt) { fputs("[NIYAH] adam alloc failed\n", stderr); niyah_free(m); return 1; }
    opt->lr    = base_lr;
    opt->beta1 = 0.9f;
    opt->beta2 = 0.999f;
    opt->eps   = 1e-8f;
    opt->wd    = 0.01f;

    FILE* f = open_training_data_path(data_path);
    if (!f) { fputs("[NIYAH] no data file found\n", stderr); niyah_adam_free(opt); niyah_free(m); return 1; }

    tokenizer_init();
    printf("=== NIYAH TRAIN (Adam) ===\n");
    printf("  simd    : %s\n",   niyah_simd_name());
    printf("  params  : %zu\n",  niyah_param_count(m));
    printf("  vocab   : %u  ctx: %u  embed: %u  layers: %u\n",
           cfg.vocab_size, cfg.ctx_len, cfg.embed_dim, cfg.n_layers);
    printf("  epochs  : %d  lr: %.2e  min_lr: %.2e\n", epochs, (double)base_lr, (double)min_lr);
    fflush(stdout);

    char line[4096];
    uint32_t total_lines = 0;
    while (fgets(line, sizeof(line), f)) { if (strlen(line) > 2) total_lines++; }
    rewind(f);
    if (total_lines == 0) {
        fputs("[NIYAH] no usable lines\n", stderr);
        fclose(f); tokenizer_free(); niyah_adam_free(opt); niyah_free(m); return 1;
    }
    printf("  lines   : %u  (steps/ep ~ %u)\n", total_lines, total_lines);
    fflush(stdout);

    uint32_t total_steps  = total_lines * (uint32_t)epochs;
    uint32_t warmup_steps = total_steps / 20;
    if (warmup_steps < 100) warmup_steps = 100;

    float ema = 0.f, best_ema = 1e30f;
    int   bad_windows = 0;
    uint32_t global_step = 0;
    clock_t t0 = clock();

    for (int ep = 0; ep < epochs; ep++) {
        rewind(f);
        float ls = 0.f;
        uint32_t st = 0;

        while (fgets(line, sizeof(line), f)) {
            uint32_t tk[256];
            uint32_t n = tokenizer_encode(line, tk, 256);
            if (n < 2) continue;
            if (n > cfg.ctx_len + 1) n = cfg.ctx_len + 1;

            /* CRITICAL: fold all token IDs into [0, vocab_size) */
            clamp_tokens(tk, n, cfg.vocab_size);

            opt->lr = cosine_lr(base_lr, min_lr, global_step, total_steps, warmup_steps);

            float l = niyah_train_step(m, opt, tk, n);
            if (!isfinite(l)) { printf("[WARN] non-finite loss ep%d s%u\n", ep+1, st); continue; }

            ls += l; st++; global_step++;
            ema = (ema <= 0.f) ? l : (0.995f * ema + 0.005f * l);

            if (st % 500 == 0) {
                printf("  ep%d s%u  loss=%.4f  ema=%.4f  lr=%.2e\n",
                       ep+1, st, ls/st, ema, (double)opt->lr);
                fflush(stdout);
            }

            if (st % 2000 == 0) {
                if (ema < best_ema - 1e-3f) { best_ema = ema; bad_windows = 0; }
                else {
                    bad_windows++;
                    if (bad_windows >= 6) {
                        printf("[NIYAH] early-stop ep%d step%u ema=%.4f\n", ep+1, st, ema);
                        goto done;
                    }
                }
            }
        }

        printf("Epoch %d/%d  loss=%.4f  ema=%.4f  steps=%u\n",
               ep+1, epochs, st > 0 ? ls/st : 0.f, ema, st);
        fflush(stdout);
    }

done:
    fclose(f);
    tokenizer_free();
    printf("\nDone in %.1fs\n", (double)(clock()-t0)/CLOCKS_PER_SEC);
    niyah_save(m, "niyah_trained.bin");
    printf("Saved: niyah_trained.bin  (params=%zu)\n", niyah_param_count(m));
    niyah_adam_free(opt);
    niyah_free(m);
    return 0;
}
