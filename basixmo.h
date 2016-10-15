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
#include <set>
#include <map>
#include <vector>
#include <unordered_map>

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


typedef uint32_t BxoHash_t;

class BxoVal;
class BxoString;
class BxoObj;
class BxoSet;
class BxoTuple;
class BxoMix;
class BxoLink;

enum class BxoVKind : std::uint8_t
{
  NoneK,
  IntK,
  DoubleK,
  StringK,
  ObjectK,
  SetK,
  TupleK,
  MixK,
  LinkK
};

class BxoVal
{
  /// these classes are subclasses of BxoVal
  friend class BxoVNone;
  friend class BxoVInt;
  friend class BxoVString;
  friend class BxoVDouble;
  friend class BxoVObject;
  friend class BxoVSet;
  friend class BxoVTuple;
  friend class BxoVMix;
  friend class BxoVLink;
  /// this is the shared object
  friend class BxoObj;
public:
  struct TagNone {};
  struct TagInt {};
  struct TagString {};
  struct TagDouble {};
  struct TagObject {};
  struct TagSet {};
  struct TagTuple {};
  struct TagMix {};
  struct TagLink {};
protected:
  const BxoVKind _kind;
  const union
  {
    void* _ptr;
    const intptr_t _int;
    const double _dbl;
    const std::shared_ptr<BxoObj> _obj;
    const std::unique_ptr<const BxoString> _str;
    const std::unique_ptr<const BxoSet> _set;
    const std::unique_ptr<const BxoTuple> _tup;
    const std::unique_ptr<const BxoMix> _mix;
    const std::unique_ptr<const BxoLink> _link;
  };
  BxoVal(TagNone, std::nullptr_t)
    : _kind(BxoVKind::NoneK), _ptr(nullptr) {};
  BxoVal(TagInt, intptr_t i)
    : _kind(BxoVKind::IntK), _int(i) {};
  //// note: dont allow NaN-s
  BxoVal(TagDouble, double d)
    : _kind(std::isnan(d)?BxoVKind::NoneK:BxoVKind::DoubleK), _dbl(std::isnan(d)?0.0:d) {};
  inline BxoVal(TagString, std::string s);
  inline BxoVal(TagString, BxoString*);
  inline BxoVal(TagObject, BxoObj*po);
  inline BxoVal(BxoObj*po, TagObject);
  inline BxoVal(TagObject, const std::shared_ptr<BxoObj> op);
  inline BxoVal(const std::shared_ptr<BxoObj> op, TagObject);
  inline BxoVal(TagSet, BxoSet*pset);
  inline BxoVal(TagMix, BxoMix*pmix);
  inline BxoVal(TagLink, BxoLink*plnk);
  BxoVal() : BxoVal(TagNone {}, nullptr) {};
public:
  inline ~BxoVal();
  inline bool operator == (const BxoVal&) const;
};        // end class BxoVal

class BxoSequence
{
protected:
  const BxoHash_t _hash;
  const unsigned _len;
  std::shared_ptr<BxoObj> *_seq;
  BxoSequence(BxoHash_t h, unsigned len, std::shared_ptr<BxoObj> *seq)
    : _hash(h), _len(len), _seq(new std::shared_ptr<BxoObj>[len])
  {
    for (unsigned ix=0; ix<len; ix++)
      _seq[ix] = seq[ix];
  }
  bool same_sequence(const BxoSequence&r) const
  {
    if (_hash != r._hash) return false;
    if (_len != r._len) return false;
    for (unsigned ix=0; ix<_len; ix++)
      if (_seq[ix].get () != r._seq[ix].get ())
        return false;
    return true;
  }
public:
  BxoHash_t hash()const
  {
    return _hash;
  };
  unsigned length() const
  {
    return _len;
  };
};
class BxoSet : public BxoSequence
{
  friend class BxoVal;
  friend class BxoVSet;
public:
  bool same_set(const BxoSet& r) const
  {
    return same_sequence(r);
  }
};

class BxoTuple : public BxoSequence
{
  friend class BxoVal;
  friend class BxoVTuple;
public:
  bool same_tuple(const BxoTuple& r) const
  {
    return same_sequence(r);
  }
};

class BxoString
{
  friend class BxoVal;
  friend class BxoVString;
  const BxoHash_t _hash;
  const std::string _str;
public:
  bool same_string(const BxoString&r) const
  {
    return _hash == r._hash && _str == r._str;
  }
};

class BxoMix
{
  friend class BxoVal;
  friend class BxoVMix;
  const BxoHash_t _hash;
  const std::vector<std::shared_ptr<BxoObj>> _objvec;
  const std::vector<intptr_t> _intvec;
  const std::vector<double> _dblvec;
  const std::vector<BxoString> _strvec;
public:
  BxoHash_t hash() const
  {
    return _hash;
  };
  bool same_mix(const BxoMix& r) const
  {
    if (_hash != r._hash) return false;
    if (this == &r) return true;
    auto obsz = _objvec.size();
    if (obsz != r._objvec.size()) return false;
    auto stsz = _strvec.size();
    if (stsz != r._strvec.size()) return false;
    auto insz = _intvec.size();
    if (insz != r._intvec.size()) return false;
    auto dbsz = _dblvec.size();
    if (dbsz != r._dblvec.size()) return false;
    for (size_t obix = 0; obix < obsz; obix++)
      if (_objvec[obix] != r._objvec[obix])
        return false;
    for (size_t inix = 0; inix < insz; inix++)
      if (_intvec[inix] != r._intvec[inix])
        return false;
    for (size_t dbix = 0; dbix < dbsz; dbix++)
      if (_dblvec[dbix] != r._dblvec[dbix])
        return false;
    for (size_t stix = 0; stix < stsz; stix++)
      if (!_strvec[stix].same_string(r._strvec[stix]))
        return false;
    return true;
  }
};        // end class BxoMix

class BxoLink
{
  const BxoHash_t _hash;
};

BxoVal::BxoVal(TagObject, BxoObj*po)
  : _kind(po?BxoVKind::ObjectK:BxoVKind::NoneK), _obj(po) {};

BxoVal::BxoVal(BxoObj*po, TagObject)
  : _kind(BxoVKind::ObjectK), _obj(po) {};

BxoVal:: BxoVal(TagObject, const std::shared_ptr<BxoObj> op)
  : _kind(op?BxoVKind::ObjectK:BxoVKind::NoneK), _obj(op) {};

BxoVal:: BxoVal(const std::shared_ptr<BxoObj> op, TagObject)
  : _kind(BxoVKind::ObjectK), _obj(op) {};
BxoVal:: BxoVal(TagSet, BxoSet*pset)
  : _kind(pset?BxoVKind::SetK:BxoVKind::NoneK), _set(pset) {};
BxoVal:: BxoVal(TagMix, BxoMix*pmix)
  : _kind(pmix?BxoVKind::MixK:BxoVKind::NoneK), _mix(pmix) {};
BxoVal:: BxoVal(TagLink, BxoLink*plnk)
  : _kind(plnk?BxoVKind::LinkK:BxoVKind::NoneK), _link(plnk) {};

BxoVal::~BxoVal()
{
  switch(_kind)
    {
    case BxoVKind::NoneK:
    case BxoVKind::IntK:
    case BxoVKind::DoubleK:
      break;
    case BxoVKind::StringK:
    {
      _str.~unique_ptr<const BxoString>();
    }
    break;
    case BxoVKind::ObjectK:
      _obj.~shared_ptr<BxoObj>();
      break;
    case BxoVKind::SetK:
      _set.~unique_ptr<const BxoSet>();
      break;
    case BxoVKind::TupleK:
      _tup.~unique_ptr<const BxoTuple>();
      break;
    case BxoVKind::MixK:
      _mix.~unique_ptr<const BxoMix>();
      break;
    case BxoVKind::LinkK:
      _link.~unique_ptr<const BxoLink>();
      break;
    }
  _ptr = nullptr;
}

struct BxoHashObjSharedPtr
{
  inline size_t operator() (const std::shared_ptr<BxoObj>& po) const;
};

class BxoPayload;
class BxoObj
{
  friend class BxoVal;
  const BxoHash_t _hash;
  bool _gcmark;
  std::unordered_map<const std::shared_ptr<BxoObj>,BxoVal,BxoHashObjSharedPtr> _attrh;
  std::vector<BxoVal> _compv;
  /// what about routine payloads?
public:
  BxoHash_t hash()const
  {
    return _hash;
  };
  virtual ~BxoObj() {};
};        // end class BxoObj

size_t
BxoHashObjSharedPtr::operator() (const std::shared_ptr<BxoObj>& po) const
{
  if (!po) return 0;
  else return po->hash();
};

bool BxoVal::operator == (const BxoVal&r) const
{
  if (_kind != r._kind) return false;
  if (BXO_UNLIKELY(this == &r))
    return true;
  switch (_kind)
    {
    case BxoVKind::NoneK:
      return true;
    case BxoVKind::IntK:
      return _int == r._int;
    case BxoVKind::DoubleK:
      return _dbl == r._dbl;
    case BxoVKind::StringK:
      return _str->same_string(*r._str);
    case BxoVKind::ObjectK:
      return _obj == r._obj;
    case BxoVKind::TupleK:
      return _tup->same_tuple(*r._tup);
    case BxoVKind::SetK:
      return _set->same_set(*r._set);
    case BxoVKind::MixK:
      return _mix->same_mix(*r._mix);
    }
}
#endif /*BASIXMO_HEADER*/
