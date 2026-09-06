from ygoenv.python.api import py_env

from .edopro_ygoenv import (
  _EDOProEnvPool,
  _EDOProEnvSpec,
  _EDOProSyncEnvPool,
  init_module,
)

(
  EDOProEnvSpec,
  EDOProDMEnvPool,
  EDOProGymEnvPool,
  EDOProGymnasiumEnvPool,
) = py_env(_EDOProEnvSpec, _EDOProEnvPool)

# Threadless single-thread pool (forkable; for search snapshots, not throughput).
(
  _,
  EDOProSyncDMEnvPool,
  EDOProSyncGymEnvPool,
  EDOProSyncGymnasiumEnvPool,
) = py_env(_EDOProEnvSpec, _EDOProSyncEnvPool)


__all__ = [
  "EDOProEnvSpec",
  "EDOProDMEnvPool",
  "EDOProGymEnvPool",
  "EDOProGymnasiumEnvPool",
  "EDOProSyncDMEnvPool",
  "EDOProSyncGymEnvPool",
  "EDOProSyncGymnasiumEnvPool",
]
