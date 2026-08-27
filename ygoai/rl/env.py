import numpy as np
import gymnasium as gym


class RecordEpisodeStatistics(gym.Wrapper):
    def __init__(self, env):
        super().__init__(env)
        self.num_envs = getattr(env, "num_envs", 1)
        self.episode_returns = None
        self.episode_lengths = None

    def reset(self, **kwargs):
        observations, infos = self.env.reset(**kwargs)
        self.episode_returns = np.zeros(self.num_envs, dtype=np.float32)
        self.episode_lengths = np.zeros(self.num_envs, dtype=np.int32)
        self.returned_episode_returns = np.zeros(self.num_envs, dtype=np.float32)
        self.returned_episode_lengths = np.zeros(self.num_envs, dtype=np.int32)
        return observations, infos

    def step(self, action):
        return self.update_stats_and_infos(*super().step(action))

    # --- async (envpool batch_size < num_envs) --------------------------------
    # Every counter here is per-env, and an async recv() returns an arbitrary
    # SUBSET of envs, so all updates have to be scattered by env_id rather than
    # applied to a dense batch.
    def async_reset(self):
        self.env.async_reset()
        n = self.num_envs
        self.episode_returns = np.zeros(n, dtype=np.float32)
        self.episode_lengths = np.zeros(n, dtype=np.int32)
        self.returned_episode_returns = np.zeros(n, dtype=np.float32)
        self.returned_episode_lengths = np.zeros(n, dtype=np.int32)

    def send(self, action, env_id):
        return self.env.send(action, env_id)

    def recv(self):
        observations, rewards, terminated, truncated, infos = self.env.recv()
        eid = infos["env_id"]
        dones = np.logical_or(terminated, truncated)
        self.episode_returns[eid] += infos.get("reward", rewards)
        self.episode_lengths[eid] += 1
        self.returned_episode_returns[eid] = np.where(
            dones, self.episode_returns[eid], self.returned_episode_returns[eid])
        self.returned_episode_lengths[eid] = np.where(
            dones, self.episode_lengths[eid], self.returned_episode_lengths[eid])
        self.episode_returns[eid] *= 1 - dones
        self.episode_lengths[eid] *= 1 - dones
        infos["r"] = self.returned_episode_returns[eid]
        infos["l"] = self.returned_episode_lengths[eid]
        return observations, rewards, dones, infos

    def update_stats_and_infos(self, *args):
        observations, rewards, terminated, truncated, infos = args
        dones = np.logical_or(terminated, truncated)
        self.episode_returns += infos.get("reward", rewards)
        self.episode_lengths += 1
        self.returned_episode_returns = np.where(
            dones, self.episode_returns, self.returned_episode_returns
        )
        self.returned_episode_lengths = np.where(
            dones, self.episode_lengths, self.returned_episode_lengths
        )
        self.episode_returns *= 1 - dones
        self.episode_lengths *= 1 - dones
        infos["r"] = self.returned_episode_returns
        infos["l"] = self.returned_episode_lengths

        return (
            observations,
            rewards,
            dones,
            infos,
        )

    # (Upstream had a second async block here that assumed a DENSE batch and a
    # send() without env_id -- i.e. it could never have worked against a real
    # async pool, and being later in the class body it silently overrode the
    # env_id-indexed versions above. Removed.)


class CompatEnv(gym.Wrapper):

    def reset(self, **kwargs):
        observations, infos = self.env.reset(**kwargs)
        return observations, infos

    def step(self, action):
        observations, rewards, terminated, truncated, infos = super().step(action)
        dones = np.logical_or(terminated, truncated)
        return (
            observations,
            rewards,
            dones,
            infos,
        )


class EnvPreprocess(gym.Wrapper):

    def __init__(self, env, skip_mask):
        super().__init__(env)
        self.num_envs = env.num_envs
        self.skip_mask = skip_mask

    def reset(self, **kwargs):
        observations, infos = self.env.reset(**kwargs)
        if self.skip_mask:
            observations['mask_'] = None
        return observations, infos

    def step(self, action):
        observations, rewards, terminated, truncated, infos = super().step(action)
        if self.skip_mask:
            observations['mask_'] = None
        return (
            observations,
            rewards,
            terminated,
            truncated,
            infos,
        )

    def async_reset(self):
        return self.env.async_reset()

    def send(self, action, env_id):
        return self.env.send(action, env_id)

    def recv(self):
        observations, rewards, terminated, truncated, infos = self.env.recv()
        if self.skip_mask:
            observations['mask_'] = None
        return observations, rewards, terminated, truncated, infos