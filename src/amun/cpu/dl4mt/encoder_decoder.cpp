#include "cpu/dl4mt/encoder_decoder.h"

#include <vector>
#include <yaml-cpp/yaml.h>

#include "common/sentences.h"
#include "common/hypothesis.h"
#include "cpu/dl4mt/encoder.h"
#include "cpu/dl4mt/decoder.h"


namespace amunmt {
namespace CPU {
namespace dl4mt {

using EDState = EncoderDecoderState;

EncoderDecoder::EncoderDecoder(const God &god,
							   const std::string& name,
                               const YAML::Node& config,
                               size_t tab,
                               const dl4mt::Weights& model,
                               const Search &search)
  : CPUEncoderDecoderBase(god, name, config, tab, search),
    model_(model),
    encoder_(new dl4mt::Encoder(model_)),
    decoder_(new dl4mt::Decoder(model_))
{}


void EncoderDecoder::Decode(EncOutPtr encOut,
                            const State& in,
                            State& out,
                            const std::vector<uint>&)
{
  const EDState& edIn = in.get<EDState>();
  EDState& edOut = out.get<EDState>();

  decoder_->Decode(edOut.GetStates(), edIn.GetStates(),
                   edIn.GetEmbeddings(), SourceContext_);
}


void EncoderDecoder::BeginSentenceState(EncOutPtr encOut, State& state, size_t batchSize)
{
  EDState& edState = state.get<EDState>();
  decoder_->EmptyState(edState.GetStates(), SourceContext_, batchSize);
  decoder_->EmptyEmbedding(edState.GetEmbeddings(), batchSize);
}


void EncoderDecoder::Encode(SentencesPtr sources) {
  encoder_->Encode(sources->Get(0).GetWords(tab_), SourceContext_);
}


void EncoderDecoder::AssembleBeamState(const State& in,
                                       const Hypotheses& beam,
                                       State& out) const
{
  std::vector<uint> beamWords;
  std::vector<uint> beamStateIds;
  for(auto h : beam) {
      beamWords.push_back(h->GetWord());
      beamStateIds.push_back(h->GetPrevStateIndex());
  }

  const EDState& edIn = in.get<EDState>();
  EDState& edOut = out.get<EDState>();

  edOut.GetStates() = mblas::Assemble<mblas::byRow, mblas::Matrix>(edIn.GetStates(), beamStateIds);
  decoder_->Lookup(edOut.GetEmbeddings(), beamWords);
}


void EncoderDecoder::GetAttention(mblas::Matrix& Attention) {
  decoder_->GetAttention(Attention);
}


mblas::Matrix& EncoderDecoder::GetAttention() {
  return decoder_->GetAttention();
}


size_t EncoderDecoder::GetVocabSize() const {
  return decoder_->GetVocabSize();
}


void EncoderDecoder::Filter(const std::vector<uint>& filterIds) {
  decoder_->Filter(filterIds);
}


BaseMatrix& EncoderDecoder::GetProbs() {
  return decoder_->GetProbs();
}

}
}
}
