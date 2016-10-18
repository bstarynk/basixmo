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
#include <initializer_list>
#include <map>
#include <vector>
#include <unordered_map>
#include <unordered_set>

// libbacktrace from GCC 6, i.e. libgcc-6-dev package
#include <backtrace.h>

#include <unistd.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <sched.h>
#include <syslog.h>

#include "json/json.h"

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

extern "C" int64_t bxo_prime_above(int64_t n);
extern "C" int64_t bxo_prime_below(int64_t n);

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

#define BXO_BACKTRACELOG_AT(Fil,Lin,Log) do {   \
    std::ostringstream _out_##Lin;              \
    _out_##Lin << Log << std::flush;            \
    bxo_backtracestr_at((Fil), (Lin),           \
      _out_##Lin.str());                        \
  } while(0)
#define BXO_BACKTRACELOG_AT_BIS(Fil,Lin,Log) \
  BXO_BACKTRACELOG_AT(Fil,Lin,Log)
#define BXO_BACKTRACELOG(Log) \
  BXO_BACKTRACELOG_AT(__FILE__,__LINE__,Log)


typedef uint32_t BxoHash_t;

typedef uint32_t Bxo_hid_t;
typedef uint64_t Bxo_loid_t;

typedef __int128 Bxo_int128_t;
typedef unsigned __int128 Bxo_uint128_t;

typedef Json::Value BxoJson;
class BxoVal;
class BxoString;
class BxoObj;
class BxoSet;
class BxoTuple;
class BxoDumper;
class BxoLoader;


struct BxoHashObjSharedPtr
{
  inline size_t operator() (const std::shared_ptr<BxoObj>& po) const;
};

struct BxoLessObjSharedPtr
{
  inline bool operator() (const std::shared_ptr<BxoObj>&lp, const std::shared_ptr<BxoObj>&rp) const;
};

#define BXO_SIZE_MAX (INT32_MAX/2)
enum class BxoVKind : std::uint8_t
{
  NoneK,
  IntK,
  /* we probably dont need doubles at first. But we want to avoid NaNs if we need them; we would use nil instead of boxed NaN */
  // DoubleK,
  StringK,
  ObjectK,
  SetK,
  TupleK,
  /* we don't need mix (of scalar values, e.g. ints, doubles, strings, objects) at first */
  // MixK,
  /* links (à la symlinks) would be nice, e.g. some indirect reference to an attribute inside an object */
  // LinkK
};

class BxoVal
{
  /// these classes are subclasses of BxoVal
  friend class BxoVNone;
  friend class BxoVInt;
  friend class BxoVString;
  friend class BxoVObject;
  friend class BxoVSet;
  friend class BxoVTuple;
  /// this is the shared object
  friend class BxoObj;
  /// the dumper
  friend class BxoDumper;
public:
  struct TagNone {};
  struct TagInt {};
  struct TagString {};
  struct TagObject {};
  struct TagSet {};
  struct TagTuple {};
protected:
  const BxoVKind _kind;
  union
  {
    void* _ptr;
    intptr_t _int;
    std::shared_ptr<BxoObj> _obj;
    std::shared_ptr<const BxoString> _str;
    std::shared_ptr<const BxoSet> _set;
    std::shared_ptr<const BxoTuple> _tup;
  };
  BxoVal(TagNone, std::nullptr_t)
    : _kind(BxoVKind::NoneK), _ptr(nullptr) {};
  BxoVal(TagInt, intptr_t i)
    : _kind(BxoVKind::IntK), _int(i) {};
  inline BxoVal(TagString, const std::string& s);
  inline BxoVal(TagString, const BxoString*);
  inline BxoVal(TagObject, BxoObj*po);
  inline BxoVal(BxoObj*po, TagObject);
  inline BxoVal(TagObject, const std::shared_ptr<BxoObj> op);
  inline BxoVal(const std::shared_ptr<BxoObj> op, TagObject);
  inline BxoVal(TagSet, const BxoSet*pset);
  BxoVal() : BxoVal(TagNone {}, nullptr) {};
public:
  inline BxoVal(const BxoVal&v);
  inline BxoVal(BxoVal&&v);
  inline ~BxoVal();
  inline bool equal(const BxoVal&) const;
  bool operator == (const BxoVal&r) const
  {
    return equal(r);
  };
  bool less(const BxoVal&) const;
  bool less_equal(const BxoVal&) const;
  bool operator < (const BxoVal&v) const
  {
    return less(v);
  };
  bool operator <= (const BxoVal&v) const
  {
    return less_equal(v);
  };
  inline BxoHash_t hash() const;
  BxoJson to_json(BxoDumper&) const;
  static BxoVal from_json(BxoLoader&, const BxoJson&);
};        // end class BxoVal

class BxoVNone: public BxoVal
{
public:
  BxoVNone() : BxoVal(TagNone {},nullptr) {};
  ~BxoVNone() = default;
};        // end BxoVNone

class BxoVInt: public BxoVal
{
public:
  BxoVInt(int64_t i=0): BxoVal(TagInt {},i) {};
  ~BxoVInt() = default;
};        // end BxoVInt


class BxoVString: public BxoVal
{
public:
  BxoVString(const BxoString&);
  BxoVString(const char*s, int l= -1);
  BxoVString(const std::string& str);
  ~BxoVString() = default;
};        // end BxoVString

class BxoVSet: public BxoVal
{
public:
  ~BxoVSet() = default;
  BxoVSet(const BxoSet&);
  BxoVSet(const std::set<std::shared_ptr<BxoObj>,BxoLessObjSharedPtr>&);
  BxoVSet(const std::vector<std::shared_ptr<BxoObj>>&);
  BxoVSet(const std::vector<const BxoObj*>);
  BxoVSet(std::initializer_list<std::shared_ptr<BxoObj>>);
  BxoVSet(std::initializer_list<BxoObj*>);
};        // end BxoVSet

class BxoVTuple: public BxoVal
{
public:
  ~BxoVTuple() = default;
};        // end BxoVTuple

class BxoDumper
{
public:
  virtual ~BxoDumper() {};
  virtual bool is_dumpable(BxoObj*);
  inline bool is_dumpable(std::shared_ptr<BxoObj> obp)
  {
    return obp && is_dumpable(obp.get());
  }
};        // end class BxoDumper

class BxoLoader
{
public:
  virtual ~BxoLoader() {};
  virtual BxoObj* obj_from_idstr(const std::string&);
  virtual BxoVal val_from_json(const BxoJson&);
};        // end BxoLoader

class BxoSequence : public std::enable_shared_from_this<BxoSequence>
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
  inline bool less_than_sequence(const BxoSequence&r) const;
  bool less_equal_sequence(const BxoSequence&r) const
  {
    if (BXO_UNLIKELY(_hash == r._hash))
      if (same_sequence(r)) return true;
    return less_than_sequence(r);
  }
  BxoJson sequence_to_json(BxoDumper&) const;
public:
  BxoHash_t hash()const
  {
    return _hash;
  };
  unsigned length() const
  {
    return _len;
  };
};        // end class BxoSequence


class BxoSet : public BxoSequence
{
  friend class BxoVal;
  friend class BxoVSet;
  static constexpr BxoHash_t init_hash = 99;
  static inline BxoHash_t combine_hash(BxoHash_t h, const BxoObj&ob);
  static BxoHash_t adjust_hash(BxoHash_t h, unsigned ln)
  {
    return h?h:(((ln*449)&0xffff)+23);
  }
  static BxoSet the_empty_set;
  static BxoSet*make_set(const std::set<std::shared_ptr<BxoObj>,BxoLessObjSharedPtr>&bs);
  static BxoSet*make_set(const std::vector<std::shared_ptr<BxoObj>>&vec);
  BxoSet(BxoHash_t h, unsigned len, std::shared_ptr<BxoObj> *seq)
    : BxoSequence(h, len, seq) {};
public:
  bool same_set(const BxoSet& r) const
  {
    return same_sequence(r);
  }
  bool less_than_set(const BxoSet& r) const
  {
    return less_than_sequence(r);
  }
  bool less_equal_set(const BxoSet& r) const
  {
    return less_equal_sequence(r);
  }
};        // end of BxoSet

class BxoTuple : public BxoSequence
{
  friend class BxoVal;
  friend class BxoVTuple;
public:
  bool same_tuple(const BxoTuple& r) const
  {
    return same_sequence(r);
  }
  bool less_than_tuple(const BxoTuple& r) const
  {
    return less_than_sequence(r);
  }
  bool less_equal_tuple(const BxoTuple& r) const
  {
    return less_equal_sequence(r);
  }
};        // end of BxoTuple

class BxoString: public std::enable_shared_from_this<BxoString>
{
  friend class BxoVal;
  friend class BxoVString;
  const BxoHash_t _hash;
  const std::string _str;
  BxoString(BxoHash_t h, const std::string str) : _hash(h), _str(str) {};
public:
  static BxoHash_t hash_cstring(const char*s, int ln= -1);
  BxoString(const BxoString&s) : _hash(s._hash), _str(s._str) {};
  BxoString(const char*s, int l= -1)
    : BxoString(hash_cstring(s,l),
                std::string(s?s:"",(l>=0)?l:(s?strlen(s):0))) {};
  BxoString(const std::string& str): BxoString(str.c_str(), str.size()) {};
  BxoHash_t hash()const
  {
    return _hash;
  };
  const std::string&string(void) const
  {
    return _str;
  };
  bool same_string(const BxoString&r) const
  {
    return _hash == r._hash && _str == r._str;
  }
  bool same_string(const std::string&s) const
  {
    return _str == s;
  }
  bool less_than_string(const BxoString&r) const
  {
    return _str < r._str;
  };
  bool less_equal_string(const BxoString&r) const
  {
    return _str <= r._str;
  };
};        // end of BxoString


BxoVal::BxoVal(TagString, const std::string& s)
  : _kind(BxoVKind::StringK)
{
  new(&_str) std::shared_ptr<const BxoString>(new BxoString(s));
}

BxoVal::BxoVal(TagString,const BxoString*bs)
  : _kind(BxoVKind::StringK)
{
  if (!bs)
    {
      BXO_BACKTRACELOG("string BxoVal: null BxoString");
      throw std::runtime_error("string BxoVal: null BxoString");
    }
  new(&_str) std::shared_ptr<const BxoString>(bs);
}

BxoVal::BxoVal(TagObject, BxoObj*po)
  : _kind(po?BxoVKind::ObjectK:BxoVKind::NoneK), _obj(po) {};

BxoVal::BxoVal(BxoObj*po, TagObject)
  : _kind(BxoVKind::ObjectK), _obj(po) {};

BxoVal:: BxoVal(TagObject, const std::shared_ptr<BxoObj> op)
  : _kind(op?BxoVKind::ObjectK:BxoVKind::NoneK), _obj(op) {};

BxoVal:: BxoVal(const std::shared_ptr<BxoObj> op, TagObject)
  : _kind(BxoVKind::ObjectK), _obj(op) {};
BxoVal:: BxoVal(TagSet, const BxoSet*pset)
  : _kind(pset?BxoVKind::SetK:BxoVKind::NoneK), _set(pset) {};


BxoVal::BxoVal(const BxoVal&v)
  : _kind(v._kind)
{
  switch (v._kind)
    {
    case BxoVKind::NoneK:
      _ptr = nullptr;
      break;
    case BxoVKind::IntK:
      _int = v._int;
      break;
    case BxoVKind::StringK:
      new(&_str) std::shared_ptr<BxoString>(new BxoString(*v._str));
      break;
    case BxoVKind::ObjectK:
      new(&_obj) std::shared_ptr<BxoObj>(v._obj);
      break;
    case BxoVKind::SetK:
      new(&_set) std::shared_ptr<BxoSet>(new BxoSet(*v._set));
      break;
    case BxoVKind::TupleK:
      new(&_tup) std::shared_ptr<BxoTuple>(new BxoTuple(*v._tup));
      break;
    }
} // end BxoVal::BxoVal(const BxoVal&v)


BxoVal::BxoVal(BxoVal&&v)
  : _kind(v._kind)
{
  switch (v._kind)
    {
    case BxoVKind::NoneK:
      _ptr = nullptr;
      break;
    case BxoVKind::IntK:
      _int = v._int;
      break;
    case BxoVKind::StringK:
      _str = std::move(v._str);
      break;
    case BxoVKind::ObjectK:
      _obj = std::move(v._obj);
      break;
    case BxoVKind::SetK:
      _set = std::move(v._set);
      break;
    case BxoVKind::TupleK:
      _tup = std::move(v._tup);
      break;
    }
  *const_cast<BxoVKind*>(&v._kind) = BxoVKind::NoneK;
  v._ptr = nullptr;
} // end BxoVal::BxoVal(BxoVal&&v)


BxoVal::~BxoVal()
{
  switch(_kind)
    {
    case BxoVKind::NoneK:
    case BxoVKind::IntK:
      break;
    case BxoVKind::StringK:
    {
      _str.~shared_ptr<const BxoString>();
    }
    break;
    case BxoVKind::ObjectK:
      _obj.~shared_ptr<BxoObj>();
      break;
    case BxoVKind::SetK:
      _set.~shared_ptr<const BxoSet>();
      break;
    case BxoVKind::TupleK:
      _tup.~shared_ptr<const BxoTuple>();
      break;
    }
  _ptr = nullptr;
}


class BxoPayload;
#define BXO_CSTRIDLEN 18        // used length
#define BXO_CSTRIDSIZ ((BXO_CSTRIDLEN|3)+1)
#define BXO_CSTRIDSCANF "_%17[A-Za-z0-9]"
#define BXO_HID_BUCKETMAX 36000
class BxoObj: public std::enable_shared_from_this<BxoObj>
{
  friend class BxoVal;
  friend class BxoPayload;
  const BxoHash_t _hash;
  bool _gcmark;
  const Bxo_hid_t _hid;
  const Bxo_loid_t _loid;
  std::unordered_map<const std::shared_ptr<BxoObj>,BxoVal,BxoHashObjSharedPtr> _attrh;
  std::vector<BxoVal> _compv;
  std::unique_ptr<BxoPayload> _payl;
public:
  BxoHash_t hash()const
  {
    return _hash;
  };
  ~BxoObj() = default;
  bool same(const BxoObj&r) const
  {
    return this == &r;
  };
  bool less(const BxoObj&r) const
  {
    if (this == &r) return false;
    if (_hid >= r._hid) return false;
    if (_hid < r._hid) return true;
    return _loid < r._loid;
  };
  bool less_equal(const BxoObj&r) const
  {
    if (this == &r) return true;
    if (_hid >= r._hid) return false;
    if (_hid < r._hid) return true;
    return _loid <= r._loid;
  }
  static std::string str_from_hid_loid(Bxo_hid_t hid, Bxo_loid_t loid);
  static bool cstr_to_hid_loid(const char*cstr, Bxo_hid_t* phid, Bxo_loid_t* ploid, const char**endp=nullptr);
  static bool str_to_hid_loid(const std::string& str,  Bxo_hid_t* phid, Bxo_loid_t* ploid)
  {
    return cstr_to_hid_loid(str.c_str(), phid, ploid);
  }
  static unsigned hi_id_bucketnum(Bxo_hid_t hid)
  {
    if (hid == 0)
      return 0;
    unsigned bn = hid >> 16;
    if (bn > 0 && bn < BXO_HID_BUCKETMAX)
      return bn;
    BXO_BACKTRACELOG("hi_id_bucketnum: bad hid=" << hid);
    throw std::runtime_error("hi_id_bucketnum: bad hid");
  }
  std::string strid(void) const
  {
    return str_from_hid_loid(_hid,_loid);
  };
  BxoJson id_to_json(void) const
  {
    return BxoJson(strid());
  };
  static inline BxoHash_t hash_from_hid_loid (Bxo_hid_t hid, Bxo_loid_t loid);
};        // end class BxoObj


class BxoPayload
{
  friend class BxoObj;
  friend class BxoVal;
  friend class std::unique_ptr<BxoPayload>;
  friend class std::shared_ptr<BxoObj>;
  BxoObj*const _owner;
protected:
  BxoPayload(BxoObj& own) : _owner(&own) {};
public:
  virtual ~BxoPayload() {};
  BxoPayload(BxoPayload&&) = delete;
  BxoPayload(const BxoPayload&) = delete;
  virtual std::shared_ptr<BxoObj> kind() const =0;
  BxoObj* owner () const
  {
    return _owner;
  };
};        // end BxoPayload


size_t
BxoHashObjSharedPtr::operator() (const std::shared_ptr<BxoObj>& po) const
{
  if (!po) return 0;
  else return po->hash();
};

bool BxoLessObjSharedPtr::operator() (const std::shared_ptr<BxoObj>&lp, const std::shared_ptr<BxoObj>&rp) const
{
  if (!lp)
    {
      return !rp;
    };
  if (!rp) return false;
  return lp->less(*rp);
}

bool BxoSequence::less_than_sequence(const BxoSequence&r) const
{
  if (BXO_UNLIKELY(_hash == r._hash))
    {
      if (same_sequence(r)) return false;
    }
  return std::lexicographical_compare(_seq+0, _seq+_len, r._seq+0, r._seq+r._len, BxoLessObjSharedPtr {});
}

bool BxoVal::equal (const BxoVal&r) const
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
    case BxoVKind::StringK:
      return _str->same_string(*r._str);
    case BxoVKind::ObjectK:
      return _obj == r._obj;
    case BxoVKind::TupleK:
      return _tup->same_tuple(*r._tup);
    case BxoVKind::SetK:
      return _set->same_set(*r._set);
    }
}

inline BxoHash_t bxo_int_hash(intptr_t i)
{
  BxoHash_t h = (BxoHash_t)(i ^ (i % 1208215531));
  if (BXO_UNLIKELY(h == 0))
    h = (i & 0xffff) + 31;
  return h;
}

BxoHash_t BxoVal::hash() const
{
  switch (_kind)
    {
    case BxoVKind::NoneK:
      return 0;
    case BxoVKind::IntK:
      return bxo_int_hash(_int);
    case BxoVKind::StringK:
      return _str->hash();
    case BxoVKind::ObjectK:
      return _obj->hash();
    case BxoVKind::TupleK:
      return _tup->hash();
    case BxoVKind::SetK:
      return _set->hash();
    }
}

BxoHash_t
BxoObj::hash_from_hid_loid (Bxo_hid_t hid, Bxo_loid_t loid)
{
  if (hid == 0 && loid == 0)
    return 0;
  BxoHash_t h = 0;
  h = (hid % 2500067) ^ ((BxoHash_t) (loid % 357313124579LL));
  if (BXO_UNLIKELY (h < 128))
    h = 17 + (hid % 1500043) + (BxoHash_t) (loid % 4500049);
  return h;
}

BxoHash_t
BxoSet::combine_hash(BxoHash_t h, const BxoObj&ob)
{
  return (h * 12011) ^ (ob.hash() * 439);
}

#endif /*BASIXMO_HEADER*/
