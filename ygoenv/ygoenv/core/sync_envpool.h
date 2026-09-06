// Threadless, single-thread envpool: every env steps in the CALLING thread.
//
// Why this exists (design v2, P0-02 `duelkit`): AsyncEnvPool always spawns
// worker threads, and a multithreaded process cannot be snapshotted with
// fork() (the child keeps only the forking thread, so the workers are gone).
// SyncEnvPool runs Env::EnvStep synchronously inside Send()/Reset(), so a
// Python process holding one of these pools can fork() and the child owns a
// byte-identical copy of the whole duel (core state, Lua state, obs buffers).
// That is the snapshot primitive the line finder builds on. It reuses the env
// class unchanged, so the observation/action surface is exactly the training
// env's. Not for throughput: no parallelism at all.
#ifndef YGOENV_CORE_SYNC_ENVPOOL_H_
#define YGOENV_CORE_SYNC_ENVPOOL_H_

#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "ygoenv/core/array.h"
#include "ygoenv/core/envpool.h"
#include "ygoenv/core/state_buffer_queue.h"

template <typename Env>
class SyncEnvPool : public EnvPool<typename Env::Spec> {
 protected:
  std::size_t num_envs_;
  std::size_t max_num_players_;
  std::size_t stepping_;  // envs stepped since the last Recv
  std::unique_ptr<StateBufferQueue> sbq_;
  std::vector<std::unique_ptr<Env>> envs_;

  template <typename V>
  void SendImpl(V&& action) {
    int* env_id = static_cast<int*>(action[0].Data());
    int n = action[0].Shape(0);
    auto action_batch =
        std::make_shared<std::vector<Array>>(std::forward<V>(action));
    for (int i = 0; i < n; ++i) {
      int eid = env_id[i];
      envs_[eid]->SetAction(action_batch, i);
      bool reset = envs_[eid]->IsDone();
      envs_[eid]->EnvStep(sbq_.get(), i, reset);
    }
    stepping_ += n;
  }

 public:
  using Spec = typename Env::Spec;
  using Action = typename Env::Action;
  using State = typename Env::State;

  explicit SyncEnvPool(const Spec& spec)
      : EnvPool<Spec>(spec),
        num_envs_(spec.config["num_envs"_]),
        max_num_players_(spec.config["max_num_players"_]),
        stepping_(0),
        sbq_(new StateBufferQueue(
            num_envs_, num_envs_, max_num_players_,
            spec.state_spec.template AllValues<ShapeSpec>())),
        envs_(num_envs_) {
    if (max_num_players_ != 1) {
      throw std::runtime_error("SyncEnvPool supports single-player envs only");
    }
    for (std::size_t i = 0; i < num_envs_; ++i) {
      envs_[i].reset(new Env(spec, i));
    }
  }

  void Send(const Action& action) {
    SendImpl(action.template AllValues<Array>());
  }
  void Send(const std::vector<Array>& action) override { SendImpl(action); }
  void Send(std::vector<Array>&& action) override { SendImpl(action); }

  std::vector<Array> Recv() override {
    // Mirrors AsyncEnvPool's synchronous mode: a batch is always num_envs
    // wide; slots not stepped since the last Recv are counted as done.
    std::size_t additional = num_envs_ - stepping_;
    auto ret = sbq_->Wait(additional);
    stepping_ = 0;
    return ret;
  }

  void Reset(const Array& env_ids) override {
    TArray<int> ids(env_ids);
    int n = ids.Shape(0);
    for (int i = 0; i < n; ++i) {
      envs_[ids[i]]->EnvStep(sbq_.get(), i, true);
    }
    stepping_ += n;
  }
};

#endif  // YGOENV_CORE_SYNC_ENVPOOL_H_
