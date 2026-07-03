#include "casper_ffi.h"
#include "niyah_core.h"
#include "rule_parser.h"
#include "hybrid_reasoner.h"
#include "khz_q_svd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static NiyahModel* G_MODEL = NULL;
static NiyahRuleKB* G_RULES = NULL;

int casper_init(const char* config_json) {
    if (G_MODEL) { return 0; }
    (void)config_json;
    
    NiyahConfig cfg = {
        .magic = 0x4E594148, .version = 1,
        .embed_dim = 64, .n_heads = 4, .n_kv_heads = 4,
        .n_layers = 2, .ffn_mult = 4, .vocab_size = 256,
        .ctx_len = 32, .rope_theta = 10000.f, .rms_eps = 1e-5f,
    };
    
    G_MODEL = niyah_alloc(&cfg);
    if (!G_MODEL) return -1;
    return 0;
}

int casper_judge_evaluate(const char* req_json, char* out_buf, int out_len) {
    if (!G_MODEL) { return -1; }
    
    const char* prompt = "Analyze this security event";
    KHZQ_Result khz = khz_q_verify_output(req_json, 0.85f);
    const char* rule_verdict = niyah_rule_check(G_RULES, prompt, req_json);
    
    int n = snprintf(out_buf, out_len,
        "{\"allowed\": %s, \"reason\": \"%s\", \"khz_energy\": %.4f, \"rule_verdict\": \"%s\"}",
        (khz.is_coherent && !rule_verdict) ? "true" : "false",
        "Casper evaluation complete.",
        khz.energy_preserved,
        rule_verdict ? rule_verdict : "pass"
    );
    
    if (n >= out_len) { return -2; }
    return n;
}

void casper_shutdown() {
    if (G_MODEL) { niyah_free(G_MODEL); G_MODEL = NULL; }
    if (G_RULES) { niyah_rule_free(G_RULES); G_RULES = NULL; }
    fprintf(stderr, "[casper_ffi] Engine shut down.\n");
}
