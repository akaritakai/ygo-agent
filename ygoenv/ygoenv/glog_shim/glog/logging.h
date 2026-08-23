#pragma once
// Minimal drop-in replacement for the small subset of glog that ygoenv uses
// (CHECK*/DCHECK*/LOG(INFO|ERROR|WARNING|FATAL) stream macros). Added to avoid
// linking glog at all: the system glog (0.7+) has a different ABI namespace
// than the 0.6 headers this code was written against, and the training env
// needs no logging framework.

#include <cstdlib>
#include <iostream>
#include <sstream>

namespace glog_shim {

class LogMessage {
 public:
  LogMessage(const char* file, int line, bool fatal) : fatal_(fatal) {
    stream_ << file << ":" << line << "] ";
  }
  LogMessage(const LogMessage&) = delete;
  LogMessage& operator=(const LogMessage&) = delete;
  ~LogMessage() {
    stream_ << '\n';
    std::cerr << stream_.str() << std::flush;
    if (fatal_) {
      std::abort();
    }
  }
  std::ostringstream& stream() { return stream_; }

 private:
  std::ostringstream stream_;
  bool fatal_;
};

}  // namespace glog_shim

#define GLOG_SHIM_INFO glog_shim::LogMessage(__FILE__, __LINE__, false).stream()
#define GLOG_SHIM_WARNING GLOG_SHIM_INFO
#define GLOG_SHIM_ERROR GLOG_SHIM_INFO
#define GLOG_SHIM_FATAL glog_shim::LogMessage(__FILE__, __LINE__, true).stream()

#define LOG(severity) GLOG_SHIM_##severity

// DLOG: no-op in release (NDEBUG), LOG otherwise — glog semantics.
#ifdef NDEBUG
#define DLOG(severity) \
  if (true) {          \
  } else               \
    LOG(severity)
#else
#define DLOG(severity) LOG(severity)
#endif

#define CHECK(cond) \
  if (cond) {       \
  } else            \
    LOG(FATAL) << "Check failed: " #cond " "

#define GLOG_SHIM_CHECK_OP(a, op, b) \
  if ((a)op(b)) {                    \
  } else                             \
    LOG(FATAL) << "Check failed: " #a " " #op " " #b " "

#define CHECK_EQ(a, b) GLOG_SHIM_CHECK_OP(a, ==, b)
#define CHECK_NE(a, b) GLOG_SHIM_CHECK_OP(a, !=, b)
#define CHECK_LT(a, b) GLOG_SHIM_CHECK_OP(a, <, b)
#define CHECK_LE(a, b) GLOG_SHIM_CHECK_OP(a, <=, b)
#define CHECK_GT(a, b) GLOG_SHIM_CHECK_OP(a, >, b)
#define CHECK_GE(a, b) GLOG_SHIM_CHECK_OP(a, >=, b)

// DCHECK*: no-ops in release (NDEBUG), full checks otherwise — glog semantics.
#ifdef NDEBUG
#define GLOG_SHIM_DCHECK_OP(a, op, b) \
  if (true) {                         \
  } else                              \
    LOG(FATAL)
#define DCHECK(cond) GLOG_SHIM_DCHECK_OP(cond, &&, true)
#else
#define GLOG_SHIM_DCHECK_OP(a, op, b) GLOG_SHIM_CHECK_OP(a, op, b)
#define DCHECK(cond) CHECK(cond)
#endif

#define DCHECK_EQ(a, b) GLOG_SHIM_DCHECK_OP(a, ==, b)
#define DCHECK_NE(a, b) GLOG_SHIM_DCHECK_OP(a, !=, b)
#define DCHECK_LT(a, b) GLOG_SHIM_DCHECK_OP(a, <, b)
#define DCHECK_LE(a, b) GLOG_SHIM_DCHECK_OP(a, <=, b)
#define DCHECK_GT(a, b) GLOG_SHIM_DCHECK_OP(a, >, b)
#define DCHECK_GE(a, b) GLOG_SHIM_DCHECK_OP(a, >=, b)
