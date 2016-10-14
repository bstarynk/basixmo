// file basixmo.h - common header file to be included everywhere. -*- C++ -*-

/**   Copyright (C)  2016 Basile Starynkevitch

      BASIXMO is free software; you can redistribute it and/or modify
      it under the terms of the GNU General Public License as published by
      the Free Software Foundation; either version 3, or (at your option)
      any later version.

      BASIXMO is distributed in the hope that it will be useful,
      but WITHOUT ANY WARRANTY; without even the implied warranty of
      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
      GNU General Public License for more details.
      You should have received a copy of the GNU General Public License
      along with BASIXMO; see the file COPYING3.   If not see
      <http://www.gnu.org/licenses/>.
**/
#ifndef BASIXMO_HEADER
#define BASIXMO_HEADER "basixmo.h"

#include <features.h>           // GNU things
#include <stdexcept>
#include <cstdint>
#include <climits>
#include <cmath>
#include <cstring>
#include <memory>
#include <algorithm>
#include <iostream>
#include <sstream>

// libbacktrace from GCC 6, i.e. libgcc-6-dev package
#include <backtrace.h>

#include <unistd.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <sched.h>
#include <syslog.h>

// common prefix bxo

// mark unlikely conditions to help optimization
#ifdef __GNUC__
#define BXO_UNLIKELY(P) __builtin_expect(!!(P),0)
#define BXO_LIKELY(P) !__builtin_expect(!(P),0)
#define BXO_UNUSED __attribute__((unused))
#define BXO_OPTIMIZEDFUN __attribute__((optimize("O2")))
#else
#define BXO_UNLIKELY(P) (P)
#define BXO_LIKELY(P) (P)
#define BXO_UNUSED
#define BXO_OPTIMIZEDFUN
#endif

static inline pid_t
bxo_gettid (void)
{
  return syscall (SYS_gettid, 0L);
}

// time measurement, in seconds
// query a clock
static inline double
bxo_clock_time (clockid_t cid)
{
  struct timespec ts = { 0, 0 };
  if (clock_gettime (cid, &ts))
    return NAN;
  else
    return (double) ts.tv_sec + 1.0e-9 * ts.tv_nsec;
}

static inline struct timespec
bxo_timespec (double t)
{
  struct timespec ts = { 0, 0 };
  if (std::isnan (t) || t < 0.0)
    return ts;
  double fl = floor (t);
  ts.tv_sec = (time_t) fl;
  ts.tv_nsec = (long) ((t - fl) * 1.0e9);
  // this should not happen
  if (BXO_UNLIKELY (ts.tv_nsec < 0))
    ts.tv_nsec = 0;
  while (BXO_UNLIKELY (ts.tv_nsec >= 1000 * 1000 * 1000))
    {
      ts.tv_sec++;
      ts.tv_nsec -= 1000 * 1000 * 1000;
    };
  return ts;
}


extern "C" double bxo_elapsed_real_time (void);    /* relative to start of program */
extern "C" double bxo_process_cpu_time (void);
extern "C" double bxo_thread_cpu_time (void);

// call strftime on ti, but replace .__ with centiseconds for ti
extern "C" char *bxo_strftime_centi (char *buf, size_t len, const char *fmt, double ti)
__attribute__ ((format (strftime, 3, 0)));

class BxoData;

#define BXO_EMPTY_SLOT ((void*)(2*sizeof(void*)))

extern "C" void bxo_backtracestr_at (const char*fil, int lin, const std::string&msg);

#define BXO_BACKTRACELOG_AT(Fil,Lin,Log) do { \
    std::ostringstream _out_##Lin;    \
    _out_##Lin << Log << std::flush;    \
    bxo_backtracestr_at((Fil), (Lin),   \
      _out_##Lin.str());  \
  } while(0)
#define BXO_BACKTRACELOG_AT_BIS(Fil,Lin,Log) \
  BXO_BACKTRACELOG_AT(Fil,Lin,Log)
#define BXO_BACKTRACELOG(Log) \
  BXO_BACKTRACELOG_AT(__FILE__,__LINE__,Log)

class BxoVal
{
  union
  {
    intptr_t _int;
    std::shared_ptr<BxoData> _val;
    void* _ptr;
  };
  //static_assert(sizeof(std::shared_ptr<BxoData>) == sizeof(void*), "check shared_ptr size");
public:
  BxoVal(std::nullptr_t) : _val(nullptr) {};
  BxoVal(): BxoVal(nullptr) {};
  template<class BXClass> BxoVal(std::shared_ptr<BXClass>sp): _val(sp)
  {
    static_assert( alignof(BXClass) >= sizeof(void*), "bad alignment");
    if (BXO_UNLIKELY((intptr_t)sp.get() & 1))
      {
        BXO_BACKTRACELOG("misasligned " << typeid(BXClass).name() << " @" << (void*)(sp.get()));
        throw std::logic_error(std::string {"misasligned "}+typeid(BXClass).name());
      }
  };
  BxoVal(intptr_t i) : _int((i*2)|1)
  {
    if (BXO_UNLIKELY(i<=INTPTR_MIN/2 || i>=INTPTR_MAX/2))
      {
        BXO_BACKTRACELOG("integer out of bounds " << i);
        throw std::invalid_argument("integer out of bounds");
      }
  };
  bool is_int(void) const
  {
    return (_int&1) != 0;
  };
  bool is_null(void) const
  {
    return _ptr == nullptr;
  };
  bool is_empty(void) const
  {
    return _ptr == BXO_EMPTY_SLOT;
  };
  bool is_cleared(void) const
  {
    return is_null() || is_empty();
  };
  bool is_data(void) const
  {
    return !is_int() && !is_cleared();
  };
  ~BxoVal()
  {
    if (!is_int()) _val.reset();
    _ptr= nullptr;
  };
  template<class BXClass,class... Args>
  BxoVal(Args&&... args): BxoVal(std::make_shared<BXClass>(args...)) {};
  BxoVal(BxoVal&&d)
  {
    if (d.is_int()) _int = d._int;
    else _val = d._val;
  };
  BxoVal(const BxoVal& v)
  {
    if (v.is_int()) _int = v._int;
    else _val = v._val;
  };
  BxoData& operator*() const
  {
    if (!is_data())
      {
        BXO_BACKTRACELOG("non data *");
        throw std::invalid_argument("non data *");
      }
    return *_val.get();
  }
  BxoData* operator-> () const
  {
    if (!is_data())
      {
        BXO_BACKTRACELOG("non data ->");
        throw std::invalid_argument("non data ->");
      }
    return _val.get();
  }
  BxoData* raw_data() const
  {
    return _val.get();
  };
};        /* end of BxoVal */
#endif /*BASIXMO_HEADER*/
