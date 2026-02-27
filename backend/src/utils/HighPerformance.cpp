/**
 * HighPerformance 非模板类方法实现
 */

#include "utils/HighPerformance.h"

namespace heartlake::perf {

// ============================================================================
// ACAutomaton
// ============================================================================

ACAutomaton::ACAutomaton() { nodes_.emplace_back(); }

void ACAutomaton::addPattern(std::string_view pattern, uint8_t category, uint8_t level) {
    std::lock_guard<std::mutex> lock(mutex_);
    int cur = 0;
    for (unsigned char c : pattern) {
        if (!nodes_[cur].children[c]) {
            nodes_[cur].children[c] = nodes_.size();
            nodes_.emplace_back();
            nodes_.back().depth = nodes_[cur].depth + 1;
        }
        cur = nodes_[cur].children[c];
    }
    nodes_[cur].isEnd = true;
    nodes_[cur].patternId = patterns_.size();
    nodes_[cur].category = category;
    nodes_[cur].level = level;
    patterns_.emplace_back(pattern);
}

void ACAutomaton::build() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<int> queue;
    queue.reserve(nodes_.size());

    for (int c = 0; c < CHARSET; ++c) {
        if (nodes_[0].children[c]) {
            queue.push_back(nodes_[0].children[c]);
        }
    }

    for (size_t i = 0; i < queue.size(); ++i) {
        int cur = queue[i];
        for (int c = 0; c < CHARSET; ++c) {
            int child = nodes_[cur].children[c];
            if (!child) continue;

            int fail = nodes_[cur].fail;
            while (fail && !nodes_[fail].children[c]) {
                fail = nodes_[fail].fail;
            }
            nodes_[child].fail = nodes_[fail].children[c];
            if (nodes_[child].fail == child) nodes_[child].fail = 0;

            queue.push_back(child);
        }
    }
    built_ = true;
}

bool ACAutomaton::hasMatch(std::string_view text) const {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!built_) return false;

    const size_t nodesSize = nodes_.size();
    int cur = 0;

    for (unsigned char c : text) {
        while (cur && !nodes_[cur].children[c]) {
            cur = nodes_[cur].fail;
        }
        cur = nodes_[cur].children[c];

        if (static_cast<size_t>(cur) >= nodesSize) return false;
        if (nodes_[cur].isEnd) return true;

        for (int t = nodes_[cur].fail; t; t = nodes_[t].fail) {
            if (static_cast<size_t>(t) >= nodesSize) break;
            if (nodes_[t].isEnd) return true;
        }
    }
    return false;
}

std::vector<ACAutomaton::Match> ACAutomaton::match(std::string_view text) const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<Match> results;
    if (!built_) return results;

    const size_t nodesSize = nodes_.size();
    int cur = 0;

    for (size_t i = 0; i < text.size(); ++i) {
        unsigned char c = text[i];
        while (cur && !nodes_[cur].children[c]) {
            cur = nodes_[cur].fail;
        }
        cur = nodes_[cur].children[c];

        if (static_cast<size_t>(cur) >= nodesSize) break;

        for (int t = cur; t; t = nodes_[t].fail) {
            if (static_cast<size_t>(t) >= nodesSize) break;
            if (nodes_[t].isEnd) {
                results.push_back({
                    nodes_[t].patternId,
                    static_cast<int>(i - nodes_[t].depth + 1),
                    nodes_[t].category,
                    nodes_[t].level
                });
            }
        }
    }
    return results;
}

size_t ACAutomaton::patternCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return patterns_.size();
}

std::string ACAutomaton::getPattern(uint16_t id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return id < patterns_.size() ? patterns_[id] : "";
}

void ACAutomaton::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    nodes_.clear();
    nodes_.emplace_back();
    patterns_.clear();
    built_ = false;
}

// ============================================================================
// SIMDString
// ============================================================================

void SIMDString::toLowerSSE2(char* str, size_t len) {
    const __m128i A = _mm_set1_epi8('A');
    const __m128i Z = _mm_set1_epi8('Z');
    const __m128i diff = _mm_set1_epi8('a' - 'A');

    size_t i = 0;
    for (; i + 16 <= len; i += 16) {
        __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(str + i));
        __m128i ge_A = _mm_cmpgt_epi8(chunk, _mm_sub_epi8(A, _mm_set1_epi8(1)));
        __m128i le_Z = _mm_cmplt_epi8(chunk, _mm_add_epi8(Z, _mm_set1_epi8(1)));
        __m128i mask = _mm_and_si128(ge_A, le_Z);
        __m128i toAdd = _mm_and_si128(mask, diff);
        chunk = _mm_add_epi8(chunk, toAdd);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(str + i), chunk);
    }

    for (; i < len; ++i) {
        if (str[i] >= 'A' && str[i] <= 'Z') {
            str[i] += 32;
        }
    }
}

const char* SIMDString::findSSE2(const char* haystack, size_t hlen,
                                  const char* needle, size_t nlen) {
    if (nlen == 0) return haystack;
    if (nlen > hlen) return nullptr;

    __m128i first = _mm_set1_epi8(needle[0]);

    for (size_t i = 0; i + nlen <= hlen; ) {
        if (i + 16 <= hlen) {
            __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(haystack + i));
            __m128i cmp = _mm_cmpeq_epi8(chunk, first);
            int mask = _mm_movemask_epi8(cmp);

            if (mask == 0) {
                i += 16;
                continue;
            }

            int offset = __builtin_ctz(mask);
            i += offset;
        }

        if (i + nlen <= hlen && std::memcmp(haystack + i, needle, nlen) == 0) {
            return haystack + i;
        }
        ++i;
    }

    return nullptr;
}

size_t SIMDString::countCharSSE2(const char* str, size_t len, char c) {
    size_t count = 0;
    __m128i target = _mm_set1_epi8(c);

    size_t i = 0;
    for (; i + 16 <= len; i += 16) {
        __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(str + i));
        __m128i cmp = _mm_cmpeq_epi8(chunk, target);
        int mask = _mm_movemask_epi8(cmp);
        count += __builtin_popcount(mask);
    }

    for (; i < len; ++i) {
        if (str[i] == c) ++count;
    }
    return count;
}

// ============================================================================
// ThreadPool
// ============================================================================

ThreadPool::ThreadPool(size_t threads)
    : stop_(false) {
    for (size_t i = 0; i < threads; ++i) {
        workers_.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    condition_.wait(lock, [this] {
                        return stop_ || !tasks_.empty();
                    });

                    if (stop_ && tasks_.empty()) return;

                    task = std::move(tasks_.front());
                    tasks_.pop();
                }
                task();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_ = true;
    }
    condition_.notify_all();
    for (auto& worker : workers_) {
        worker.join();
    }
}

size_t ThreadPool::queueSize() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return tasks_.size();
}

// ============================================================================
// TokenBucket
// ============================================================================

TokenBucket::TokenBucket(double rate, double burst)
    : rate_(rate), burst_(burst), tokens_(burst),
      lastRefill_(std::chrono::steady_clock::now()) {}

bool TokenBucket::tryAcquire(double tokens) {
    refill();
    double current = tokens_.load(std::memory_order_relaxed);
    do {
        if (current < tokens) return false;
    } while (!tokens_.compare_exchange_weak(current, current - tokens,
                                             std::memory_order_release,
                                             std::memory_order_relaxed));
    return true;
}

void TokenBucket::acquire(double tokens) {
    while (!tryAcquire(tokens)) {
        std::this_thread::sleep_for(std::chrono::microseconds(
            static_cast<int64_t>(tokens / rate_ * 1000000)));
    }
}

double TokenBucket::availableTokens() {
    refill();
    return tokens_.load(std::memory_order_relaxed);
}

void TokenBucket::refill() {
    auto now = std::chrono::steady_clock::now();
    auto last = lastRefill_.load(std::memory_order_relaxed);

    double elapsed = std::chrono::duration<double>(now - last).count();
    double toAdd = elapsed * rate_;

    if (toAdd > 0.001 && lastRefill_.compare_exchange_weak(last, now,
                                                            std::memory_order_release,
                                                            std::memory_order_relaxed)) {
        double current = tokens_.load(std::memory_order_relaxed);
        double newTokens;
        do {
            newTokens = std::min(current + toAdd, burst_);
        } while (!tokens_.compare_exchange_weak(current, newTokens,
                                                 std::memory_order_release,
                                                 std::memory_order_relaxed));
    }
}

} // namespace heartlake::perf
