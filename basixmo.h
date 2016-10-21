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
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <random>

// libbacktrace from GCC 6, i.e. libgcc-6-dev package
#include <backtrace.h>

#include <unistd.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <sched.h>
#include <syslog.h>
#include <stdlib.h>
#include <dlfcn.h>

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

#define BXO_PROGBINARY "bxmo"
#define BXO_MODULEDIR "modules.dir"
#define BXO_MODULEPREFIX "modu_"
#define BXO_MODULESUFFIX ".so"

class QSqlDatabase;
class QSqlQuery;

// from generated _timestamp.c
extern "C" const char basixmo_timestamp[];
extern "C" const char basixmo_lastgitcommit[];
extern "C" const char basixmo_lastgittag[];
extern "C" const char*const basixmo_cxxsources[];
extern "C" const char*const basixmo_csources[];
extern "C" const char*const basixmo_shellsources[];
extern "C" const char basixmo_directory[];
extern "C" const char basixmo_statebase[];

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
  BXO_BACKTRACELOG_AT_BIS(__FILE__,__LINE__,Log)

extern "C" void bxo_abort(void) __attribute__((noreturn));
#ifdef NDEBUG
#define BXO_ASSERT_AT(Fil,Lin,Prop,Log) do {    \
 if (BXO_UNLIKELY(!(Prop))) {                   \
   BXO_BACKTRACELOG_AT(Fil,Lin,                 \
           "**BXO_ASSERT FAILED** " #Prop ":"   \
           " @ " __FUNCTION__                   \
           << Log);                             \
   bxo_abort();                                 \
 }                                              \
} while(0)
#else
#define BXO_ASSERT_AT(Fil,Lin,Prop,Log)  do {   \
    if (false && !(Prop))                       \
      BXO_BACKTRACELOG_AT(Fil,Lin,Log);         \
} while(0)
#endif  // NDEBUG
#define BXO_ASSERT_AT_BIS(Fil,Lin,Prop,Log) \
  BXO_ASSERT_AT(Fil,Lin,Prop,Log)
#define BXO_ASSERT(Prop,Log) \
  BXO_ASSERT_AT_BIS(__FILE__,__LINE__,Prop,Log)


typedef uint32_t BxoHash_t;

typedef uint32_t Bxo_hid_t;
typedef uint64_t Bxo_loid_t;

typedef __int128 Bxo_int128_t;
typedef unsigned __int128 Bxo_uint128_t;

typedef Json::Value BxoJson;
class BxoVal;
class BxoString;
class BxoObject;
class BxoSet;
class BxoTuple;
class BxoDumper;
class BxoLoader;


class BxoRandom
{
  static thread_local BxoRandom _rand_thr_;
  unsigned long _rand_count;
  std::mt19937 _rand_generator;
  uint32_t generate_32u(void)
  {
    if (BXO_UNLIKELY(_rand_count++ % 4096 == 0))
      {
        std::random_device randev;
        auto s1=randev(), s2=randev(), s3=randev(), s4=randev(),
             s5=randev(), s6=randev(), s7=randev();
        std::seed_seq seq {s1,s2,s3,s4,s5,s6,s7};
        _rand_generator.seed(seq);
      }
    return _rand_generator();
  };
  uint32_t generate_nonzero_32u(void)
  {
    uint32_t r = 0;
    do
      {
        r = generate_32u();
      }
    while (BXO_UNLIKELY(r==0));
    return r;
  };
  uint64_t generate_64u(void)
  {
    return (static_cast<uint64_t>(generate_32u())<<32) | static_cast<uint64_t>(generate_32u());
  };
public:
  static uint32_t random_32u(void)
  {
    return _rand_thr_.generate_32u();
  };
  static uint64_t random_64u(void)
  {
    return _rand_thr_.generate_64u();
  };
  static uint32_t random_nonzero_32u(void)
  {
    return _rand_thr_.generate_nonzero_32u();
  };
};        // end class BxoRandom

struct BxoHashObjSharedPtr
{
  inline size_t operator() (const std::shared_ptr<BxoObject>& po) const;
};

struct BxoHashObjWeakPtr
{
  inline size_t operator() (const std::weak_ptr<BxoObject>& po) const;
};

struct BxoHashObjPtr
{
  inline size_t operator() (BxoObject* po) const;
};

struct BxoLessObjSharedPtr
{
  inline bool operator() (const std::shared_ptr<BxoObject>&lp, const std::shared_ptr<BxoObject>&rp) const;
};

struct BxoLessObjPtr
{
  inline bool operator() (const BxoObject*lp, const BxoObject*rp) const;
};

#define BXO_SIZE_MAX (INT32_MAX/3)
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
  friend class BxoVObj;
  friend class BxoVSet;
  friend class BxoVTuple;
  /// this is the shared object
  friend class BxoObject;
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
    std::shared_ptr<BxoObject> _obj;
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
  inline BxoVal(TagObject, BxoObject*po);
  inline BxoVal(BxoObject*po, TagObject);
  inline BxoVal(TagObject, const std::shared_ptr<BxoObject> op);
  inline BxoVal(const std::shared_ptr<BxoObject> op, TagObject);
  inline BxoVal(TagSet, const BxoSet*pset);
  inline BxoVal(TagTuple, const BxoTuple*ptup);
  BxoVal() : BxoVal(TagNone {}, nullptr) {};
public:
  inline BxoVal(const BxoVal&v);
  inline BxoVal(BxoVal&&v);
  inline BxoVal& operator = (const BxoVal&);
  inline BxoVal& operator = (BxoVal&&);
  inline ~BxoVal();
  inline void clear();
  void reset(void)
  {
    clear();
  };
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
  void scan_dump(BxoDumper&) const;
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
  inline BxoVSet(const BxoSet&bs);
  inline BxoVSet(const std::set<std::shared_ptr<BxoObject>,BxoLessObjSharedPtr>&);
  inline BxoVSet(const std::vector<std::shared_ptr<BxoObject>>&);
  inline BxoVSet(const std::vector<BxoObject*>&);
  BxoVSet(std::initializer_list<std::shared_ptr<BxoObject>> il)
    : BxoVSet(std::vector<std::shared_ptr<BxoObject>>(il)) {};
  BxoVSet(std::initializer_list<BxoObject*>il)
    : BxoVSet(std::vector<BxoObject*>(il)) {};
  template <typename... Args> BxoVSet(Args ... args)
    : BxoVSet({args...}) {};
};        // end BxoVSet

class BxoVTuple: public BxoVal
{
public:
  ~BxoVTuple() = default;
  inline BxoVTuple(const BxoTuple&);
  BxoVTuple(const std::vector<std::shared_ptr<BxoObject>>&);
  BxoVTuple(const std::vector<BxoObject*>);
  BxoVTuple(std::initializer_list<std::shared_ptr<BxoObject>> il)
    : BxoVTuple(std::vector<std::shared_ptr<BxoObject>>(il)) {};
  BxoVTuple(std::initializer_list<BxoObject*>il)
    : BxoVTuple(std::vector<BxoObject*>(il)) {};
  template <typename... Args> BxoVTuple(Args ... args)
    : BxoVTuple({args...}) {};
};        // end BxoVTuple





class BxoVObj: public BxoVal
{
public:
  ~BxoVObj() = default;
  BxoVObj(BxoObject*ob) : BxoVal(TagObject {},ob) {};
  BxoVObj(std::shared_ptr<BxoObject>pob): BxoVal(TagObject {},pob) {};
};        // end BxoVObj


class BxoDumper
{
  static constexpr const char* insert_object_sql =
    "INSERT INTO t_objects "
    " (ob_id, ob_mtime, ob_jsoncont, ob_classid, ob_paylkid, ob_paylcont, ob_paylmod)"
    " VALUES (?, ?, ?, ?, ?, ?, ?)";
  enum { InsobIdIx, InsobMtimIx, InsobJsoncontIx, InsobClassidIx,
         InsobPaylkindIx, InsobPaylcontIx, InsobPaylmodIx,
         Insob_LastIx
       };
  QSqlQuery* _du_queryinsobj;
  QSqlDatabase* _du_sqldb;
  enum { DuStop, DuScan, DuEmit } _du_state;
  std::string _du_dirname;
  std::unordered_set<BxoObject*,BxoHashObjPtr> _du_objset;
  std::deque<std::shared_ptr<BxoObject>> _du_scanque;
public:
  BxoDumper(const std::string&dir = ".");
  ~BxoDumper();
  BxoDumper(const BxoDumper&) = delete;
  BxoDumper(BxoDumper&&) = delete;
  void scan_all(void);
  void emit_all(void);
  void emit_object_row(BxoObject*pob);
  bool is_dumpable(BxoObject*pob)
  {
    return pob && _du_objset.find(pob) != _du_objset.end();
  };
  bool is_dumpable(std::shared_ptr<BxoObject> obp)
  {
    return obp && is_dumpable(obp.get());
  }
  bool scan_dumpable(BxoObject*); // return true if the object is
  // dumpable, and add it to the
  // dumpobset
};        // end class BxoDumper

class BxoLoader
{
  friend class BxoObject;
  std::string _ld_dirname;
  QSqlDatabase* _ld_sqldb;
  double _ld_startelapsedtime;
  double _ld_startprocesstime;
  std::unordered_map<std::string,std::shared_ptr<BxoObject>> _ld_idtoobjmap;
  void create_objects(void);
  void set_globals(void);
  void name_objects(void);
  void name_predefined(void);
  void link_modules(void);
  void fill_objects_contents(void);
  void load_objects_class(void);
  void load_objects_payload(void);
protected:
  void register_objref(const std::string&,std::shared_ptr<BxoObject> obp);
public:
  BxoLoader(const std::string dirname=".");
  ~BxoLoader();
  void load(void);
  std::shared_ptr<BxoObject> find_loadedobj(const std::string& str)
  {
    auto it = _ld_idtoobjmap.find(str);
    if (it != _ld_idtoobjmap.end())
      return it->second;
    return nullptr;
  }
  BxoObject* obj_from_idstr(const std::string&);
  BxoObject* obj_from_idstr(const char*cs)
  {
    return obj_from_idstr(std::string(cs));
  };
  BxoVal val_from_json(const BxoJson&js)
  {
    return BxoVal::from_json(*this,js);
  };
};        // end BxoLoader

class BxoSequence : public std::enable_shared_from_this<BxoSequence>
{
protected:
  const BxoHash_t _hash;
  const unsigned _len;
  std::shared_ptr<BxoObject> *_seq;
  BxoSequence(BxoHash_t h, unsigned len, const std::shared_ptr<BxoObject> *seq)
    : _hash(h), _len(len), _seq(new std::shared_ptr<BxoObject>[len])
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
  inline void sequence_scan_dump(BxoDumper&) const;
};        // end class BxoSequence


class BxoSet : public BxoSequence
{
  friend class BxoVal;
  friend class BxoVSet;
  static constexpr BxoHash_t init_hash = 99;
  static inline BxoHash_t combine_hash(BxoHash_t h, const BxoObject&ob);
  static BxoHash_t adjust_hash(BxoHash_t h, unsigned ln)
  {
    return h?h:(((ln*449)&0xffff)+23);
  }
  static BxoSet the_empty_set;
  static const BxoSet*make_set(const std::set<std::shared_ptr<BxoObject>,BxoLessObjSharedPtr>&bs);
  static const BxoSet*make_set(const std::vector<std::shared_ptr<BxoObject>>&vec);
  static const BxoSet*make_set(const std::vector<BxoObject*>&vec);
  BxoSet(BxoHash_t h, unsigned len, const std::shared_ptr<BxoObject> * seq)
    : BxoSequence(h, len, seq) {};
public:
  static const BxoSet*load_set(BxoLoader&, const BxoJson&);
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
  static BxoTuple the_empty_tuple;
  static const BxoTuple*make_tuple(const std::vector<std::shared_ptr<BxoObject>>&vec);
  static const BxoTuple*make_tuple(const std::vector<BxoObject*>&vec);
  BxoTuple(BxoHash_t h, unsigned len, const std::shared_ptr<BxoObject> * seq)
    : BxoSequence(h, len, seq) {};
  static constexpr BxoHash_t init_hash = 127;
  static inline BxoHash_t combine_hash(BxoHash_t h, const BxoObject&ob);
  static BxoHash_t adjust_hash(BxoHash_t h, unsigned ln)
  {
    return h?h:(((ln*491)&0xffff)+31);
  }
public:
  static const BxoTuple*load_tuple(BxoLoader&, const BxoJson&);
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
  BxoString(const BxoString&s) : std::enable_shared_from_this<BxoString>(s), _hash(s._hash), _str(s._str) {};
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

BxoVal::BxoVal(TagObject, BxoObject*po)
  : _kind(po?BxoVKind::ObjectK:BxoVKind::NoneK), _obj(po) {};

BxoVal::BxoVal(BxoObject*po, TagObject)
  : _kind(BxoVKind::ObjectK), _obj(po) {};

BxoVal:: BxoVal(TagObject, const std::shared_ptr<BxoObject> op)
  : _kind(op?BxoVKind::ObjectK:BxoVKind::NoneK), _obj(op) {};

BxoVal:: BxoVal(const std::shared_ptr<BxoObject> op, TagObject)
  : _kind(BxoVKind::ObjectK), _obj(op) {};
BxoVal:: BxoVal(TagSet, const BxoSet*pset)
  : _kind(pset?BxoVKind::SetK:BxoVKind::NoneK), _set(pset) {};


BxoVal:: BxoVal(TagTuple, const BxoTuple*ptup)
  : _kind(ptup?BxoVKind::TupleK:BxoVKind::NoneK), _tup(ptup) {};


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
      new(&_obj) std::shared_ptr<BxoObject>(v._obj);
      break;
    case BxoVKind::SetK:
      new(&_set) std::shared_ptr<BxoSet>(new BxoSet(*v._set));
      break;
    case BxoVKind::TupleK:
      new(&_tup) std::shared_ptr<BxoTuple>(new BxoTuple(*v._tup));
      break;
    }
} // end BxoVal::BxoVal(const BxoVal&v)


BxoVal& BxoVal::operator =(const BxoVal&s)
{
  auto tk = _kind;
  auto sk = s._kind;
  if (tk == sk)
    {
      switch (s._kind)
        {
        case BxoVKind::NoneK:
          _ptr = nullptr;
          break;
        case BxoVKind::IntK:
          _int = s._int;
          break;
        case BxoVKind::StringK:
          _str = s._str;
          break;
        case BxoVKind::ObjectK:
          _obj = s._obj;
          break;
        case BxoVKind::SetK:
          _set = s._set;
          break;
        case BxoVKind::TupleK:
          _tup = s._tup;
          break;
        }
      return *this;
    }
  if (tk != BxoVKind::NoneK)
    clear();
  new(this) BxoVal(s);
  return *this;
} // end BxoVal::operator =(const BxoVal&)

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



BxoVal& BxoVal::operator =(BxoVal&&s)
{
  auto tk = _kind;
  auto sk = s._kind;
  if (tk == sk)
    {
      switch (s._kind)
        {
        case BxoVKind::NoneK:
          _ptr = nullptr;
          break;
        case BxoVKind::IntK:
          _int = s._int;
          break;
        case BxoVKind::StringK:
          _str = std::move(s._str);
          break;
        case BxoVKind::ObjectK:
          _obj = std::move(s._obj);
          break;
        case BxoVKind::SetK:
          _set = std::move(s._set);
          break;
        case BxoVKind::TupleK:
          _tup = std::move(s._tup);
          break;
        }
      *const_cast<BxoVKind*>(&s._kind) = BxoVKind::NoneK;
      s._ptr = nullptr;
      return *this;
    }
  if (tk != BxoVKind::NoneK)
    clear();
  new(this) BxoVal(s);
  return *this;
} // end BxoVal::operator =(BxoVal&&)


void BxoVal::clear()
{
  auto k = _kind;
  *const_cast<BxoVKind*>(&_kind) = BxoVKind::NoneK;
  switch(k)
    {
    case BxoVKind::NoneK:
      break;
    case BxoVKind::IntK:
      _int = 0;
      break;
    case BxoVKind::StringK:
      _str.reset();
      break;
    case BxoVKind::ObjectK:
      _obj.reset();
      break;
    case BxoVKind::SetK:
      _set.reset();
    case BxoVKind::TupleK:
      _tup.reset();
      break;
    }
  _ptr = nullptr;
} // end BxoVal::clear()



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
      _obj.~shared_ptr<BxoObject>();
      break;
    case BxoVKind::SetK:
      _set.~shared_ptr<const BxoSet>();
      break;
    case BxoVKind::TupleK:
      _tup.~shared_ptr<const BxoTuple>();
      break;
    }
  *const_cast<BxoVKind*>(&_kind) = BxoVKind::NoneK;
  _ptr = nullptr;
} // end BxoVal::~BxoVal



enum class BxoSpace: std::uint8_t
{
  TransientSp,
  PredefSp,
  GlobalSp,
  /// UserSp,
  _Last
};

class BxoPayload;
#define BXO_CSTRIDLEN 18        // used length
#define BXO_CSTRIDSIZ ((BXO_CSTRIDLEN|3)+1)
#define BXO_CSTRIDSCANF "_%17[A-Za-z0-9]"
#define BXO_HID_BUCKETMAX 36000
#define BXO_MAX_NAME_LEN 1024

class BxoObject: public std::enable_shared_from_this<BxoObject>
{
  friend class BxoVal;
  friend class BxoPayload;
  friend class std::shared_ptr<BxoObject>;
  const BxoHash_t _hash;
  bool _gcmark;
  BxoSpace _space;
  const Bxo_hid_t _hid;
  const Bxo_loid_t _loid;
  std::shared_ptr<BxoObject> _classob;
  std::unordered_map<const std::shared_ptr<BxoObject>,BxoVal,BxoHashObjSharedPtr> _attrh;
  std::vector<BxoVal> _compv;
  std::unique_ptr<BxoPayload> _payl;
  time_t _mtime;
  struct PredefTag {};
  struct PseudoTag {};
  struct LoadedTag {};
  static std::unordered_set<std::shared_ptr<BxoObject>,BxoHashObjSharedPtr> _predef_set_;
  static std::unordered_set<BxoObject*,BxoHashObjPtr> _bucketarr_[BXO_HID_BUCKETMAX];
  static std::map<std::string,std::shared_ptr<BxoObject>> _namedict_;
  static std::unordered_map<const BxoObject*,std::string> _namemap_;
  static inline void register_in_bucket(BxoObject*pob)
  {
    _bucketarr_[hi_id_bucketnum(pob->_hid)].insert(pob);
  }
public:
  BxoSpace space() const
  {
    return _space;
  };
  void change_space(BxoSpace);
  /// since PredefTag is private this is only reachable from our
  /// member functions
  BxoObject(PredefTag, BxoHash_t hash, Bxo_hid_t hid, Bxo_loid_t loid)
    : std::enable_shared_from_this<BxoObject>(),
      _hash(hash), _gcmark(false), _space(BxoSpace::PredefSp), _hid(hid), _loid(loid),
      _classob {nullptr},
             _attrh {}, _compv {}, _payl {nullptr}, _mtime(0)
  {
    register_in_bucket(this);
  };
  BxoObject(PseudoTag, BxoHash_t hash, Bxo_hid_t hid, Bxo_loid_t loid)
    : std::enable_shared_from_this<BxoObject>(),
      _hash(hash), _gcmark(false), _space(BxoSpace::TransientSp), _hid(hid), _loid(loid),
      _classob {nullptr},
             _attrh {}, _compv {}, _payl {nullptr}, _mtime(0)
  {
  };
  BxoObject(LoadedTag, BxoHash_t hash, Bxo_hid_t hid, Bxo_loid_t loid)
    : std::enable_shared_from_this<BxoObject>(),
      _hash(hash), _gcmark(false), _space(BxoSpace::GlobalSp), _hid(hid), _loid(loid),
      _classob {nullptr},
             _attrh {}, _compv {}, _payl {nullptr}, _mtime(0)
  {
    register_in_bucket(this);
  };
  static void initialize_predefined_objects (void);
  static BxoVal set_of_predefined_objects (void);
  BxoHash_t hash()const
  {
    return _hash;
  };
  time_t mtime() const
  {
    return _mtime;
  };
  ~BxoObject();
  void touch()
  {
    _mtime = ::time(nullptr);
  };
  bool same(const BxoObject&r) const
  {
    return this == &r;
  };
  bool less(const BxoObject&r) const
  {
    if (this == &r) return false;
    if (_hid >= r._hid) return false;
    if (_hid < r._hid) return true;
    return _loid < r._loid;
  };
  bool less_equal(const BxoObject&r) const
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
  static unsigned hi_id_bucketnum(Bxo_hid_t hid, bool unchecked=false)
  {
    if (hid == 0)
      return 0;
    unsigned bn = hid >> 16;
    if (bn > 0 && bn < BXO_HID_BUCKETMAX)
      return bn;
    if (unchecked)
      return 0;
    BXO_BACKTRACELOG("hi_id_bucketnum: bad hid=" << hid);
    throw std::runtime_error("hi_id_bucketnum: bad hid");
  }
  static bool valid_name(const std::string&str);
  bool register_named(const std::string&str);
  bool forget_named(void);
  static bool forget_name(const std::string&str);
  static std::shared_ptr<BxoObject> find_named_objref(const std::string&str)
  {
    auto it = _namedict_.find(str);
    if (it != _namedict_.end())
      return it->second;
    else return nullptr;
  };
  static BxoObject*find_named(const std::string&str)
  {
    return find_named_objref(str).get();
  }
  std::string name(void) const
  {
    auto it = _namemap_.find(this);
    if (it != _namemap_.end())
      return it->second;
    return "";
  }
  std::string strid(void) const
  {
    return str_from_hid_loid(_hid,_loid);
  };
  std::string pname(void) const
  {
    std::string n = name();
    if (n.empty()) return strid();
    else return n;
  };
  std::string short_pname(void) const
  {
    std::string n = name();
    if (n.empty()) return strid().substr(1);
    else return n;
  };
  BxoJson id_to_json(void) const
  {
    return BxoJson(strid());
  };
  static inline BxoHash_t hash_from_hid_loid (Bxo_hid_t hid, Bxo_loid_t loid);
  static BxoObject* find_from_hid_loid (Bxo_hid_t hid, Bxo_loid_t loid);
  static BxoObject* find_from_idstr(const std::string&idstr);
  static BxoObject* make_object(BxoSpace sp = BxoSpace::TransientSp);
  static std::shared_ptr<BxoObject> make_objref(BxoSpace sp = BxoSpace::TransientSp)
  {
    BxoObject* pob = make_object(sp);
    BXO_ASSERT (pob != nullptr, "make_object failed");
    return pob->shared_from_this();
  }
  static std::shared_ptr<BxoObject> load_objref(BxoLoader&ld, const std::string& idstr);
  void load_content(const BxoJson&, BxoLoader&);
  void touch_load(time_t, BxoLoader&);
  void scan_content_dump(BxoDumper&) const;
  BxoJson json_for_content(BxoDumper&) const;
  std::shared_ptr<BxoObject> class_obj() const
  {
    return _classob;
  };
  BxoPayload* payload() const
  {
    return _payl.get();
  };
};        // end class BxoObject




#define BXO_VARPREDEF(Nam) bxopredef_##Nam
#define BXO_HAS_PREDEFINED(Name,Idstr,Hid,Loid,Hash) \
  extern "C" std::shared_ptr<BxoObject> BXO_VARPREDEF(Name);
#include "_bxo_predef.h"

enum Bxo_PredefHash_en
{
#define BXO_HAS_PREDEFINED(Name,Idstr,Hid,Loid,Hash) \
  bxopredh_##Name = Hash,
#include "_bxo_predef.h"
};

#define BXO_VARGLOBAL(Nam) bxoglob_##Nam
#define BXO_HAS_GLOBAL(Name,Idstr,Hid,Loid,Hash) \
  extern "C" std::shared_ptr<BxoObject> BXO_VARGLOBAL(Name);
#include "_bxo_global.h"

class BxoPayload
{
  friend class BxoObject;
  friend class BxoVal;
  friend class std::unique_ptr<BxoPayload>;
  friend class std::shared_ptr<BxoObject>;
  BxoObject*const _owner;
protected:
  BxoPayload(BxoObject& own) : _owner(&own) {};
public:
  virtual ~BxoPayload() {};
  BxoPayload(BxoPayload&&) = delete;
  BxoPayload(const BxoPayload&) = delete;
  virtual std::shared_ptr<BxoObject> kind_ob() const =0;
  virtual std::shared_ptr<BxoObject> module_ob() const =0;
  virtual void scan_payload_content(BxoDumper&) const =0;
  virtual const BxoJson emit_payload_content(BxoDumper&) const =0;
  BxoObject* owner () const
  {
    return _owner;
  };
};        // end BxoPayload


size_t
BxoHashObjSharedPtr::operator() (const std::shared_ptr<BxoObject>& po) const
{
  if (!po) return 0;
  else return po->hash();
};

size_t
BxoHashObjWeakPtr::operator() (const std::weak_ptr<BxoObject>& po) const
{
  if (po.expired()) return 0;
  else return po.lock()->hash();
};

size_t
BxoHashObjPtr::operator() (BxoObject* po) const
{
  if (!po) return 0;
  else return po->hash();
};

bool BxoLessObjSharedPtr::operator() (const std::shared_ptr<BxoObject>&lp, const std::shared_ptr<BxoObject>&rp) const
{
  if (!lp)
    {
      return !rp;
    };
  if (!rp) return false;
  return lp->less(*rp);
}

bool BxoLessObjPtr::operator() (const BxoObject*lp, const BxoObject*rp) const
{
  if (!lp) return !rp;
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

void
BxoSequence::sequence_scan_dump(BxoDumper&du) const
{
  for (unsigned ix=0; ix<_len; ix++)
    du.scan_dumpable(_seq[ix].get());
} // end of BxoSequence::sequence_scan_dump

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
BxoObject::hash_from_hid_loid (Bxo_hid_t hid, Bxo_loid_t loid)
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
BxoSet::combine_hash(BxoHash_t h, const BxoObject&ob)
{
  return (h * 12011) ^ (ob.hash() * 439);
}
BxoHash_t
BxoTuple::combine_hash(BxoHash_t h, const BxoObject&ob)
{
  return (h * 11213) ^ (ob.hash() * 367);
}

BxoVSet::BxoVSet(const BxoSet&bs)
  : BxoVal(TagSet {},&bs) {}

BxoVSet::BxoVSet(const std::set<std::shared_ptr<BxoObject>,BxoLessObjSharedPtr>&s)
  : BxoVal(TagSet {},BxoSet::make_set(s)) {}

BxoVSet::BxoVSet(const std::vector<std::shared_ptr<BxoObject>>&vs)
  : BxoVal(TagSet {}, BxoSet::make_set(vs)) {}

BxoVSet::BxoVSet(const std::vector<BxoObject*>&vo)
  : BxoVal(TagSet {}, BxoSet::make_set(vo)) {}

BxoVTuple::BxoVTuple(const BxoTuple& tup)
  :  BxoVal(TagTuple {},&tup) {}



#endif /*BASIXMO_HEADER*/
