#include "Core_CPP/niyah_core.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    NiyahConfig cfg = {
        .magic      = NIYAH_MAGIC,
        .version    = NIYAH_VER,
        .vocab_size = 32000,
        .ctx_len    = 512,
        .embed_dim  = 128,
        .n_layers   = 4,
        .n_heads    = 8,
        .n_kv_heads = 8,
        .ffn_mult   = 4,        /* ffn_hidden = embed_dim * ffn_mult = 512 */
        .rope_theta = 10000.0f,
        .rms_eps    = 1e-5f,
        .flags      = 0
    };

    NiyahModel *model = niyah_alloc(&cfg);
    if (!model) {
        fprintf(stderr, "ERROR: niyah_alloc failed\n");
        return 1;
    }

    printf("SIMD backend : %s\n",   niyah_simd_name());
    printf("Total params : %zu\n",  niyah_param_count(model));
    printf("embed_dim    : %u\n",   model->cfg.embed_dim);
    printf("n_layers     : %u\n",   model->cfg.n_layers);
    printf("head_dim     : %u\n",   model->head_dim);
    printf("ffn_dim      : %u\n\n", model->ffn_dim);

    /* توليد 10 رموز عشوائية */
    uint32_t token = 0;   /* <BOS> */
    for (int pos = 0; pos < 10; pos++) {
        float *logits = niyah_forward(model, token, pos);
        uint32_t next = 0;
        float mx = logits[0];
        for (uint32_t i = 1; i < cfg.vocab_size; i++) {
            if (logits[i] > mx) { mx = logits[i]; next = i; }
        }
        printf("pos %2d: token %-6u  logit_max = %.4f\n", pos, next, mx);
        token = next;
    }

    niyah_free(model);
    return 0;
}
