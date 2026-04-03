#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <random>
#include <cstring>

// =========================================================
// Config & Utils
// =========================================================
struct ModelConfig {
    int vocab_size = 32000;   // حجم المفردات
    int ctx_len = 512;
    int embed_dim = 128;
    int num_layers = 4;
    int num_heads = 8;
    int ffn_dim = 512;
    float lr = 0.001f;
    int epochs = 5;           // مرات التكرار على البيانات
};

// =========================================================
// Helper: Softmax + Cross-Entropy
// =========================================================
void softmax(float* x, int n) {
    float max_val = x[0];
    for (int i = 1; i < n; ++i) if (x[i] > max_val) max_val = x[i];
    float sum = 0.0f;
    for (int i = 0; i < n; ++i) {
        x[i] = std::exp(x[i] - max_val);
        sum += x[i];
    }
    for (int i = 0; i < n; ++i) x[i] /= sum;
}

float cross_entropy(const float* logits, int target, int n) {
    float max_logit = logits[0];
    for (int i = 1; i < n; ++i) if (logits[i] > max_logit) max_logit = logits[i];
    float sum_exp = 0.0f;
    for (int i = 0; i < n; ++i) sum_exp += std::exp(logits[i] - max_logit);
    float log_sum_exp = max_logit + std::log(sum_exp);
    return - (logits[target] - log_sum_exp);
}

// =========================================================
// Tokenizer (نفس الذي أنشأناه)
// =========================================================
#include "../tokenizer.c"   // نضعه في المسار الصحيح
// نضيف تعريف الدوال التي نحتاجها
extern "C" {
    void tokenizer_init();
    uint32_t tokenizer_encode(const char* text, uint32_t* tokens, uint32_t max_len);
    void tokenizer_free();
}

// =========================================================
// Simple Transformer (للتدريب فقط)
// =========================================================
class TinyTransformer {
public:
    ModelConfig cfg;
    std::vector<float> w_embed;       // [vocab_size x embed_dim]
    std::vector<float> w_out;          // [embed_dim x vocab_size]
    std::vector<float> w_ffn1, w_ffn2; // [embed_dim x ffn_dim], [ffn_dim x embed_dim]
    std::vector<float> b_ffn1, b_ffn2;

    TinyTransformer(const ModelConfig& config) : cfg(config) {
        int vs = cfg.vocab_size, ed = cfg.embed_dim, fd = cfg.ffn_dim;
        w_embed.resize(vs * ed);
        w_out.resize(ed * vs);
        w_ffn1.resize(ed * fd);
        w_ffn2.resize(fd * ed);
        b_ffn1.resize(fd);
        b_ffn2.resize(ed);

        // تهيئة عشوائية
        std::mt19937 rng(42);
        std::normal_distribution<float> dist(0.0f, 0.02f);
        for (auto& v : w_embed) v = dist(rng);
        for (auto& v : w_out) v = dist(rng);
        for (auto& v : w_ffn1) v = dist(rng);
        for (auto& v : w_ffn2) v = dist(rng);
        for (auto& v : b_ffn1) v = 0.0f;
        for (auto& v : b_ffn2) v = 0.0f;
    }

    // Forward: input tokens -> logits
    void forward(const std::vector<uint32_t>& tokens, std::vector<float>& logits) {
        int seq_len = tokens.size();
        int ed = cfg.embed_dim, vs = cfg.vocab_size, fd = cfg.ffn_dim;

        // embeddings
        std::vector<float> x(seq_len * ed);
        for (int t = 0; t < seq_len; ++t) {
            uint32_t id = tokens[t];
            if (id >= vs) id = 3; // UNK
            float* emb = &w_embed[id * ed];
            std::copy(emb, emb + ed, &x[t * ed]);
        }

        // بسيط: طبقة FFN واحدة فقط (بدون attention لتسهيل الفهم)
        // x [seq_len x ed] -> y [seq_len x ed] عبر FFN مع ReLU
        std::vector<float> y(seq_len * ed);
        for (int t = 0; t < seq_len; ++t) {
            float* in = &x[t * ed];
            float* out = &y[t * ed];
            // أول طبقة FFN1 + bias
            std::vector<float> h(fd);
            for (int i = 0; i < fd; ++i) {
                float sum = b_ffn1[i];
                for (int j = 0; j < ed; ++j) sum += in[j] * w_ffn1[j * fd + i];
                h[i] = std::max(0.0f, sum); // ReLU
            }
            // طبقة FFN2
            for (int i = 0; i < ed; ++i) {
                float sum = b_ffn2[i];
                for (int j = 0; j < fd; ++j) sum += h[j] * w_ffn2[j * ed + i];
                out[i] = sum;
            }
        }

        // آخر رمز فقط (نستخدمه للتنبؤ بالرمز التالي)
        // نحتاج logits للرمز الأخير
        float* last = &y[(seq_len - 1) * ed];
        logits.resize(vs);
        for (int i = 0; i < vs; ++i) {
            float sum = 0.0f;
            for (int j = 0; j < ed; ++j) sum += last[j] * w_out[j * vs + i];
            logits[i] = sum;
        }
    }

    // تحديث الأوزان باستخدام التدرجات المحسوبة يدويًا (هنا نبسط)
    // سنقوم بتخزين التدرجات في متغيرات منفصلة
    std::vector<float> grad_w_embed, grad_w_out, grad_w_ffn1, grad_w_ffn2, grad_b_ffn1, grad_b_ffn2;

    void zero_grad() {
        grad_w_embed.assign(w_embed.size(), 0.0f);
        grad_w_out.assign(w_out.size(), 0.0f);
        grad_w_ffn1.assign(w_ffn1.size(), 0.0f);
        grad_w_ffn2.assign(w_ffn2.size(), 0.0f);
        grad_b_ffn1.assign(b_ffn1.size(), 0.0f);
        grad_b_ffn2.assign(b_ffn2.size(), 0.0f);
    }

    // تحديث الوزن باستخدام SGD
    void update() {
        float lr = cfg.lr;
        for (size_t i = 0; i < w_embed.size(); ++i) w_embed[i] -= lr * grad_w_embed[i];
        for (size_t i = 0; i < w_out.size(); ++i) w_out[i] -= lr * grad_w_out[i];
        for (size_t i = 0; i < w_ffn1.size(); ++i) w_ffn1[i] -= lr * grad_w_ffn1[i];
        for (size_t i = 0; i < w_ffn2.size(); ++i) w_ffn2[i] -= lr * grad_w_ffn2[i];
        for (size_t i = 0; i < b_ffn1.size(); ++i) b_ffn1[i] -= lr * grad_b_ffn1[i];
        for (size_t i = 0; i < b_ffn2.size(); ++i) b_ffn2[i] -= lr * grad_b_ffn2[i];
    }

    // حفظ الأوزان في ملف بتنسيق متوافق مع niyah_core (يمكن تطويره لاحقًا)
    void save(const std::string& path) {
        std::ofstream f(path, std::ios::binary);
        // نكتب رأس وهمي
        uint32_t magic = 0x4E595148;
        uint32_t ver = 0x0003;
        f.write((char*)&magic, 4);
        f.write((char*)&ver, 4);
        // نكتب التكوين (يجب أن يتطابق مع NiyahConfig)
        // تبسيط: نكتب جميع الأوزان في الملف
        size_t sz = w_embed.size();
        f.write((char*)&sz, sizeof(sz));
        f.write((char*)w_embed.data(), sz * sizeof(float));
        sz = w_out.size();
        f.write((char*)&sz, sizeof(sz));
        f.write((char*)w_out.data(), sz * sizeof(float));
        // ... وهكذا
        f.close();
        std::cout << "Model saved to " << path << "\n";
    }
};

// =========================================================
// قراءة مجموعة البيانات وتحويلها إلى رموز
// =========================================================
std::vector<std::vector<uint32_t>> load_and_tokenize(const std::string& filename, int max_seq_len) {
    std::vector<std::vector<uint32_t>> sequences;
    std::ifstream file(filename);
    std::string line;
    tokenizer_init();

    uint32_t tokens[1024];
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        uint32_t n = tokenizer_encode(line.c_str(), tokens, 1024);
        if (n < 2) continue; // BOS + EOS
        // نقص الطول إذا طال
        if (n > max_seq_len) n = max_seq_len;
        std::vector<uint32_t> seq(tokens, tokens + n);
        sequences.push_back(seq);
    }
    tokenizer_free();
    return sequences;
}

// =========================================================
// التدريب
// =========================================================
void train(TinyTransformer& model, const std::vector<std::vector<uint32_t>>& sequences, int epochs) {
    std::cout << "Training started...\n";
    std::mt19937 rng(42);
    for (int epoch = 0; epoch < epochs; ++epoch) {
        float total_loss = 0.0f;
        int steps = 0;
        // خلط البيانات
        auto indices = sequences;
        std::shuffle(indices.begin(), indices.end(), rng);

        for (const auto& seq : indices) {
            if (seq.size() < 2) continue;
            // نستخدم كل تسلسل كمدخل، والهدف هو الرمز التالي بعد كل رمز
            // نأخذ أول tokens[:seq_len-1] كمدخل، والهدف tokens[1:] كتوقع
            std::vector<uint32_t> input(seq.begin(), seq.end() - 1);
            std::vector<uint32_t> target(seq.begin() + 1, seq.end());

            // نحتاج إلى حساب الخسارة لكل موضع
            // لهذا المثال المبسط، سنحسب فقط آخر رمز (لأن الشبكة لا تدعم تسلسل كامل)
            // لتبسيط أكثر: نستخدم آخر رمز كمدخل والهدف هو الرمز الذي يليه
            if (input.size() < 1) continue;
            // خذ آخر رمزين: المدخل = الرمز قبل الأخير، الهدف = الرمز الأخير
            uint32_t last_token = input.back();
            uint32_t next_token = target.back();

            std::vector<uint32_t> single_input = {last_token};
            std::vector<float> logits;
            model.forward(single_input, logits);

            float loss = cross_entropy(logits.data(), next_token, model.cfg.vocab_size);
            total_loss += loss;
            steps++;

            // هنا يجب حساب التدرجات (backpropagation) لضبط الأوزان.
            // لعدم التعقيد، سنقوم بتقريب التدرج باستخدام الفرق المحدود (finite differences)
            // هذه طريقة بطيئة ولكنها تعمل وستعطيك فكرة عن كيفية التحديث.
            // في النموذج الحقيقي، يجب تنفيذ backpropagation يدويًا.

            // ===== Finite differences =====
            // (هذا مجرد مثال تعليمي، سرعته بطيئة لكنه يعمل)
            model.zero_grad();
            float eps = 1e-4;
            auto perturb = [&](auto& weights, auto& grads, float eps) {
                for (size_t i = 0; i < weights.size(); ++i) {
                    float old = weights[i];
                    weights[i] += eps;
                    std::vector<float> logits2;
                    model.forward(single_input, logits2);
                    float loss2 = cross_entropy(logits2.data(), next_token, model.cfg.vocab_size);
                    weights[i] = old;
                    grads[i] = (loss2 - loss) / eps;
                }
            };
            perturb(model.w_embed, model.grad_w_embed, eps);
            perturb(model.w_out, model.grad_w_out, eps);
            perturb(model.w_ffn1, model.grad_w_ffn1, eps);
            perturb(model.w_ffn2, model.grad_w_ffn2, eps);
            perturb(model.b_ffn1, model.grad_b_ffn1, eps);
            perturb(model.b_ffn2, model.grad_b_ffn2, eps);

            model.update();
        }

        std::cout << "Epoch " << epoch+1 << "/" << epochs
                  << " loss = " << total_loss / steps << "\n";
    }
}

// =========================================================
// main
// =========================================================
int main() {
    ModelConfig cfg;
    cfg.epochs = 3;   // يمكن تغييره
    cfg.lr = 0.001f;

    std::cout << "Loading dataset...\n";
    std::string data_file = "../Data_Training/sovereign_knowledge.txt";
    auto sequences = load_and_tokenize(data_file, cfg.ctx_len);
    std::cout << "Loaded " << sequences.size() << " sequences.\n";

    TinyTransformer model(cfg);
    std::cout << "Model initialized.\n";

    train(model, sequences, cfg.epochs);

    // حفظ النموذج
    model.save("trained_model.bin");
    std::cout << "Training finished. Model saved to trained_model.bin\n";
    return 0;
}
