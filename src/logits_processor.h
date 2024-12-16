// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <future>

#if USE_GUIDANCE
#include <llguidance.h>
#endif

#include "models/model.h"

namespace Generators {

struct LogitsProcessor {
  LogitsProcessor() = default;
  virtual ~LogitsProcessor() = default;
  virtual void CommitTokens(std::span<int32_t> tokens) = 0;
  virtual void ProcessLogits(DeviceSpan<float> logits) = 0;
  virtual void Reset() = 0;
};

#if USE_GUIDANCE
struct LlgConstraintDeleter {
  void operator()(LlgConstraint* lc) const {
    llg_free_constraint(lc);
  }
};

struct LlgTokenizerDeleter {
  void operator()(LlgTokenizer* lt) const {
    llg_free_tokenizer(lt);
  }
};

struct GuidanceLogitsProcessor : public LogitsProcessor {
  static constexpr const char* kDefaultVocabFile = "tokenizer.json";
  static constexpr const char* kTokenizePrefixStr = "\x02";

  GuidanceLogitsProcessor(const State& state);

  void ProcessLogits(DeviceSpan<float> logits) override;
  void CommitTokens(std::span<int32_t> tokens) override;
  void Reset() override;
  std::vector<std::vector<uint32_t>> GetMask();
  static std::vector<int32_t> tokenize_partial(const Tokenizer* tokenizer, const size_t prefix_len,
                                               const uint8_t* bytes, size_t bytes_len);

 private:
  std::vector<std::vector<uint32_t>> ComputeMask();

  int vocab_size_;
  uint32_t eos_token_;
  int batch_size_;
  DeviceType device_type_;
  std::string_view guidance_type_;
  std::string_view guidance_data_;
  std::vector<std::vector<uint32_t>> masks_;
  std::vector<std::unique_ptr<LlgConstraint, LlgConstraintDeleter>> llg_constraints_;
  std::unique_ptr<LlgTokenizer, LlgTokenizerDeleter> llg_tokenizer_;
  std::shared_ptr<Tokenizer> tokenizer_;

  std::future<std::vector<std::vector<uint32_t>>> mask_future_;
  std::vector<std::vector<uint32_t>> logits_masks_;

#if USE_CUDA
  DeviceSpan<uint32_t> cuda_logits_mask_ptr_;
  cudaStream_t cuda_stream_;
#endif

  struct TokenizeData {
    Tokenizer* tokenizer;
    size_t prefix_len;
  };
  TokenizeData tokenize_data_;
};
#endif

std::unique_ptr<LogitsProcessor> CreateLogitsProcessor(const State& state);

}  // namespace Generators
