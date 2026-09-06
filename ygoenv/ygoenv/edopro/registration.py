from ygoenv.registration import register

register(
  task_id="EDOPro-v0",
  import_path="ygoenv.edopro",
  spec_cls="EDOProEnvSpec",
  dm_cls="EDOProDMEnvPool",
  gym_cls="EDOProGymEnvPool",
  gymnasium_cls="EDOProGymnasiumEnvPool",
)

# Same env, threadless pool: steps in the calling thread so the process can
# fork() a byte-identical snapshot of every duel it holds (P0-02 duelkit).
register(
  task_id="EDOProSync-v0",
  import_path="ygoenv.edopro",
  spec_cls="EDOProEnvSpec",
  dm_cls="EDOProSyncDMEnvPool",
  gym_cls="EDOProSyncGymEnvPool",
  gymnasium_cls="EDOProSyncGymnasiumEnvPool",
)
