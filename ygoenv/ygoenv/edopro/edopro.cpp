#include "ygoenv/edopro/edopro.h"
#include "ygoenv/core/py_envpool.h"
#include "ygoenv/core/sync_envpool.h"

using EDOProEnvSpec = PyEnvSpec<edopro::EDOProEnvSpec>;
using EDOProEnvPool = PyEnvPool<edopro::EDOProEnvPool>;
// Threadless pool: forkable snapshots for search (design v2 P0-02).
using EDOProSyncEnvPool = PyEnvPool<SyncEnvPool<edopro::EDOProEnv>>;

// The pool half of REGISTER, for a second pool class over the same spec.
#define REGISTER_POOL(MODULE, SPEC, ENVPOOL)                          \
  py::class_<ENVPOOL>(MODULE, "_" #ENVPOOL, py::metaclass(abc_meta))  \
      .def(py::init<const SPEC&>())                                   \
      .def_readonly("_spec", &ENVPOOL::py_spec)                       \
      .def("_recv", &ENVPOOL::PyRecv)                                 \
      .def("_send", &ENVPOOL::PySend)                                 \
      .def("_reset", &ENVPOOL::PyReset)                               \
      .def_readonly_static("_state_keys", &ENVPOOL::py_state_keys)    \
      .def_readonly_static("_action_keys", &ENVPOOL::py_action_keys);

PYBIND11_MODULE(edopro_ygoenv, m) {
  REGISTER(m, EDOProEnvSpec, EDOProEnvPool)
  REGISTER_POOL(m, EDOProEnvSpec, EDOProSyncEnvPool)

  m.def("init_module", &edopro::init_module);
}
