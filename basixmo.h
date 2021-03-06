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
#include <fstream>
#include <set>
#include <initializer_list>
#include <map>
#include <vector>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <random>
#include <typeinfo>

// libbacktrace from GCC 6, i.e. libgcc-6-dev package
#include <backtrace.h>

#include <unistd.h>
#include <sys/syscall.h>
#include <pthread.h>
#include <sched.h>
#include <syslog.h>
#include <stdlib.h>
#include <dlfcn.h>

#include <utf8.h>

#include "jsoncpp/json/json.h"

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

/// the dlopen handle for the whole program
extern "C" void* bxo_dlh;

static inline pid_t
bxo_gettid (void)
{
  return syscall (SYS_gettid, 0L);
}

std::string bxo_demangled_typename(const std::type_info &ti);

extern "C" int64_t bxo_prime_above(int64_t n);
extern "C" int64_t bxo_prime_below(int64_t n);
class QApplication;
extern "C" void bxo_gui_init(QApplication*app);
extern "C" void bxo_gui_stop(QApplication*app);

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
#ifndef NDEBUG
#define BXO_ASSERT_AT(Fil,Lin,Prop,Log) do {    \
 if (BXO_UNLIKELY(!(Prop))) {                   \
   BXO_BACKTRACELOG_AT(Fil,Lin,                 \
           "**BXO_ASSERT FAILED** " #Prop ":"   \
           " @ " <<__PRETTY_FUNCTION__          \
                       <<  std::endl            \
                       << "::" << Log);         \
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


extern "C" bool bxo_verboseflag;
#define BXO_VERBOSELOG_AT(Fil,Lin,Log) do {     \
  if (bxo_verboseflag)                          \
    std::clog << "*BXO @" << Fil << ":" << Lin  \
              << " /" << __FUNCTION__ << ": " \
              << Log << std::endl;              \
 } while(0)
#define BXO_VERBOSELOG_AT_BIS(Fil,Lin,Log) \
  BXO_VERBOSELOG_AT(Fil,Lin,Log)
#define BXO_VERBOSELOG(Log) \
  BXO_VERBOSELOG_AT_BIS(__FILE__,__LINE__,Log)

//////////////// to ease debugging
class BxoOut
{
  std::function<void(std::ostream&)> _fn_out;
public:
  BxoOut(std::function<void(std::ostream&)> fout): _fn_out(fout) {};
  ~BxoOut() = default;
  void out(std::ostream&os) const
  {
    _fn_out(os);
  };
};
inline std::ostream& operator << (std::ostream& os, const BxoOut& bo)
{
  bo.out(os);
  return os;
};

class BxoUtf8Out
{
  std::string _str;
  unsigned _flags;
public:
  BxoUtf8Out(const std::string&str, unsigned flags=0) : _str(str), _flags(flags)
  {
    if (!utf8::is_valid(str.begin(), str.end()))
      {
        BXO_BACKTRACELOG("BxoUtf8Out invalid str=" << str);
        throw std::runtime_error("BxoUtf8Out invalid string");
      }
  };
  ~BxoUtf8Out()
  {
    _str.clear();
    _flags=0;
  };
  BxoUtf8Out(const BxoUtf8Out&) = default;
  BxoUtf8Out(BxoUtf8Out&&) = default;
  void out(std::ostream&os) const;
};        // end class BxoUtf8Out

inline std::ostream& operator << (std::ostream& os, const BxoUtf8Out& bo)
{
  bo.out(os);
  return os;
};

class BxoGplv3LicenseOut
{
  std::string _file;
  std::string _prefix;
  std::string _suffix;
public:
  BxoGplv3LicenseOut(const std::string&fil, const std::string& prefix, const std::string&suffix)
    : _file(fil), _prefix(prefix), _suffix(suffix) {};
  ~BxoGplv3LicenseOut()
  {
    _file.clear();
    _prefix.clear();
    _suffix.clear();
  };
  void out(std::ostream&) const;
  BxoGplv3LicenseOut(const BxoGplv3LicenseOut&) = default;
  BxoGplv3LicenseOut(BxoGplv3LicenseOut&&) = default;
};        // end of BxoGplv3LicenseOut


inline std::ostream& operator << (std::ostream& os, const BxoGplv3LicenseOut& bo)
{
  bo.out(os);
  return os;
};

////////////////

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
class BxoJsonProcessor;		// abstract "loader"-like
class BxoJsonEmitter;		// abstract "dumper-like

#define BXO_DUMP_SCRIPT "basixmo-dump-state.sh"

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

struct BxoAlphaLessObjSharedPtr
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

class BxoSequence;
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
  friend class BxoJsonEmitter;
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
public:
  BxoVKind kind() const
  {
    return _kind;
  };
  BxoVal() : BxoVal(TagNone {}, nullptr) {};
  BxoVal(std::nullptr_t) : BxoVal(TagNone {}, nullptr) {};
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
  BxoJson to_json(BxoJsonEmitter&) const;
  void scan_dump(BxoJsonEmitter&) const;
  static BxoVal from_json(BxoJsonProcessor&, const BxoJson&);
  void out(std::ostream&os) const;
  /// the is_XXX methods are testing the kind
  /// the as_XXX methods may throw an exception
  /// the get_XXX methods may throw an exception or gives a raw non-null ptr
  /// the to_XXX methods make return a default
  bool is_null(void) const
  {
    return _kind == BxoVKind::NoneK;
  };
  bool operator ! (void) const
  {
    return is_null();
  };
  operator bool (void) const
  {
    return !is_null();
  };
  inline std::nullptr_t as_null(void) const;
  //
  bool is_int(void) const
  {
    return  _kind == BxoVKind::IntK;
  };
  inline intptr_t as_int (void) const;
  inline intptr_t to_int (intptr_t def=0) const
  {
    if (_kind != BxoVKind::IntK) return def;
    return _int;
  };
  //
  bool is_string(void) const
  {
    return _kind == BxoVKind::StringK;
  };
  inline std::shared_ptr<const BxoString> as_bstring(void) const;
  inline std::shared_ptr<const BxoString> to_bstring(const std::shared_ptr<const BxoString>& def=nullptr) const;
  inline const BxoString*get_bstring(void) const;
  inline std::string as_string(void) const;
  inline std::string to_string(const std::string& str="") const;
  //
  bool is_set(void) const
  {
    return _kind == BxoVKind::SetK;
  };
  inline std::shared_ptr<const BxoSet> as_set(void) const;
  inline std::shared_ptr<const BxoSet> to_set(const std::shared_ptr<const BxoSet> def=nullptr) const;
  inline const BxoSet*get_set(void) const;
  //
  bool is_tuple(void) const
  {
    return _kind == BxoVKind::TupleK;
  };
  inline std::shared_ptr<const BxoTuple> as_tuple(void) const;
  inline std::shared_ptr<const BxoTuple> to_tuple(const std::shared_ptr<const BxoTuple> def=nullptr) const;
  inline const BxoTuple*get_tuple(void) const;
  //
  bool is_sequence(void) const
  {
    return  _kind == BxoVKind::SetK || _kind ==BxoVKind::TupleK;
  };
  inline std::shared_ptr<const BxoSequence> as_sequence(void) const;
  inline std::shared_ptr<const BxoSequence> to_sequence(const std::shared_ptr<const BxoSequence> def=nullptr) const;
  inline const BxoSequence*get_sequence(void) const;
  //
  bool is_object(void) const
  {
    return _kind == BxoVKind::ObjectK;
  };
  inline std::shared_ptr<BxoObject> as_object(void) const;
  inline std::shared_ptr<BxoObject> to_object(const std::shared_ptr<BxoObject> defob=nullptr) const;
  inline BxoObject* get_object(void) const;
  inline BxoObject* as_objptr(void) const;
  inline BxoObject* to_objptr(BxoObject*defobp=nullptr) const;
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
  inline BxoVSet(void);
  inline BxoVSet(const BxoSet&bs);
  inline BxoVSet(const std::set<std::shared_ptr<BxoObject>,BxoLessObjSharedPtr>&);
  inline BxoVSet(const std::unordered_set<std::shared_ptr<BxoObject>,BxoHashObjSharedPtr>&);
  inline BxoVSet(const std::vector<std::shared_ptr<BxoObject>>&);
  inline BxoVSet(const std::vector<BxoObject*>&);
  BxoVSet(std::initializer_list<std::shared_ptr<BxoObject>> il)
    : BxoVSet(std::vector<std::shared_ptr<BxoObject>>(il)) {};
  BxoVSet(std::initializer_list<BxoObject*>il)
    : BxoVSet(std::vector<BxoObject*>(il)) {};
  template <typename... Args> BxoVSet(BxoObject*obp, Args ... args)
    : BxoVSet(std::initializer_list<BxoObject*>
  {
    obp, args...
  }) {};
  template <typename... Args> BxoVSet(std::shared_ptr<BxoObject> pob, Args ... args)
    : BxoVSet(std::initializer_list<std::shared_ptr<BxoObject>>
  {
    pob, args...
  }) {};
};        // end BxoVSet

class BxoVTuple: public BxoVal
{
public:
  ~BxoVTuple() = default;
  inline BxoVTuple(const BxoTuple&);
  inline BxoVTuple(void);
  BxoVTuple(const std::vector<std::shared_ptr<BxoObject>>&);
  BxoVTuple(const std::vector<BxoObject*>);
  BxoVTuple(std::initializer_list<std::shared_ptr<BxoObject>> il)
    : BxoVTuple(std::vector<std::shared_ptr<BxoObject>>(il)) {};
  BxoVTuple(std::initializer_list<BxoObject*>il)
    : BxoVTuple(std::vector<BxoObject*>(il)) {};
  template <typename... Args> BxoVTuple(std::shared_ptr<BxoObject> pob,Args ... args)
    : BxoVTuple(std::initializer_list<std::shared_ptr<BxoObject>>
  {
    pob,args...
  }) {};
  template <typename... Args> BxoVTuple(BxoObject*obp,Args ... args)
    : BxoVTuple(std::initializer_list<BxoObject*>
  {
    obp,args...
  }) {};
};        // end BxoVTuple





class BxoVObj: public BxoVal
{
public:
  ~BxoVObj() = default;
  BxoVObj(BxoObject*ob) : BxoVal(TagObject {},ob) {};
  BxoVObj(std::shared_ptr<BxoObject>pob): BxoVal(TagObject {},pob) {};
};        // end BxoVObj


class BxoJsonEmitter {
};				// end class BxoJsonEmitter

class BxoDumper : public BxoJsonEmitter
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
  double _du_startelapsedtime;
  double _du_startprocesstime;
  std::string _du_dirname;
  std::string _du_tempsuffix;
  std::unordered_set<BxoObject*,BxoHashObjPtr> _du_objset;
  std::set<std::string> _du_outfilset;
  std::deque<std::shared_ptr<BxoObject>> _du_scanque;
  std::deque<std::pair<std::function<void(BxoDumper&,BxoVal)>,BxoVal>> _du_todoafterscan;
  static std::string _defaultdumpdir_;
  static std::string generate_temporary_suffix(void);
  void rename_temporary(const std::string&filpath);
public:
  // given a relative filpath, register it and generate it pristine
  // variant with the temporary suffix
  std::string output_path(const std::string&filpath);
  static bool same_file_content(const char*path1, const char*path2);
  static void set_default_dump_dir(const std::string&s)
  {
    _defaultdumpdir_=s;
  };
  static const std::string& default_dump_dir(void)
  {
    return _defaultdumpdir_;
  };
  BxoDumper(const std::string&dir = ".");
  ~BxoDumper();
  BxoDumper(const BxoDumper&) = delete;
  BxoDumper(BxoDumper&&) = delete;
  void scan_all(void);
  void initialize_data_schema(void);
  void emit_all(void);
  void full_dump(void);
  void do_after_scan(std::function<void(BxoDumper&,BxoVal)> f, BxoVal v)
  {
    BXO_ASSERT(_du_state == DuScan, "non-scan state for do_after_scan");
    _du_todoafterscan.push_back({f,v});
  }
  // emit the object, and return its module if any
  std::shared_ptr<BxoObject> emit_object_row_module(BxoObject*pob);
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



class BxoJsonProcessor
{
  friend class BxoObject;
  friend class BxoVal;
protected:
  BxoJsonProcessor() {};
  virtual ~BxoJsonProcessor() {};
public:
  virtual  BxoObject* obj_from_idstr(const std::string&) =0;
  BxoVal val_from_json(const BxoJson&js)
  {
    return BxoVal::from_json(*this,js);
  };
};        // end abstract class BxoJsonProcessor


class BxoLoader : public BxoJsonProcessor
{
  friend class BxoObject;
  std::string _ld_dirname;
  QSqlDatabase* _ld_sqldb;
  double _ld_startelapsedtime;
  double _ld_startprocesstime;
  std::unordered_map<std::string,std::shared_ptr<BxoObject>> _ld_idtoobjmap;
  void bind_predefined(void);
  void create_objects(void);
  void set_globals(void);
  void name_objects(void);
  void name_predefined(void);
  void link_modules(void);
  void fill_objects_contents(void);
  void load_objects_class(void);
  void load_objects_create_payload(void);
  void load_objects_fill_payload(void);
  std::shared_ptr<BxoObject> name_the_predefined(const std::string&nam, const std::string&idstr);
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
      {
        auto comp = seq[ix];
        BXO_ASSERT(comp, "nil comp#" << ix);
        _seq[ix] = comp;
      }
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
  std::shared_ptr<BxoObject> *begin() const
  {
    return _len?_seq:nullptr;
  };
  std::shared_ptr<BxoObject> *end() const
  {
    return _len?(_seq+_len):nullptr;
  };
  std::shared_ptr<BxoObject> at(int rk) const
  {
    if (rk<0) rk += _len;
    if (rk>=0 && rk<(int)_len) return _seq[rk];
    return nullptr;
  }
  BxoHash_t hash()const
  {
    return _hash;
  };
  unsigned length() const
  {
    return _len;
  };
  void out(std::ostream&os) const
  {
    for (unsigned ix=0; ix<_len; ix++)
      {
        if (ix>0) os << ' ';
        auto comp = _seq[ix];
        BXO_ASSERT (comp, "no comp ix#"<< ix);
        os << comp;
      }
  }
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
  static const BxoSet*make_set(const std::set<std::shared_ptr<BxoObject>,BxoLessObjSharedPtr>& bs);
  static const BxoSet*make_set(const std::unordered_set<std::shared_ptr<BxoObject>, BxoHashObjSharedPtr>& uset);
  static const BxoSet*make_set(const std::vector<std::shared_ptr<BxoObject>> &vec);
  static const BxoSet*make_set(const std::vector<BxoObject*> &vec);
  BxoSet(BxoHash_t h, unsigned len, const std::shared_ptr<BxoObject> * seq)
    : BxoSequence(h, len, seq) {};
public:
  static const BxoSet*load_set(BxoJsonProcessor&, const BxoJson&);
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
  static const BxoTuple*load_tuple(BxoJsonProcessor&, const BxoJson&);
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
      new(&_str) std::shared_ptr<const BxoString>(v._str);
      break;
    case BxoVKind::ObjectK:
      new(&_obj) std::shared_ptr<BxoObject>(v._obj);
      break;
    case BxoVKind::SetK:
      new(&_set) std::shared_ptr<const BxoSet>(v._set);
      break;
    case BxoVKind::TupleK:
      new(&_tup) std::shared_ptr<const BxoTuple>(v._tup);
      break;
    }
} // end BxoVal::BxoVal(const BxoVal&v)


BxoVal& BxoVal::operator =(const BxoVal&s)
{
  auto tk = _kind;
  auto sk = s._kind;
  if (tk == sk)
    {
      if (this == &s) return *this;
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
  _ptr = nullptr;
  switch (v._kind)
    {
    case BxoVKind::NoneK:
      _ptr = nullptr;
      break;
    case BxoVKind::IntK:
      _int = v._int;
      break;
    case BxoVKind::StringK:
      new(&_str) std::shared_ptr<const BxoString>(std::move(v._str));
      break;
    case BxoVKind::ObjectK:
      new(&_obj) std::shared_ptr<BxoObject>(std::move(v._obj));
      break;
    case BxoVKind::SetK:
      new(&_set) std::shared_ptr<const BxoSet>(std::move(v._set));
      break;
    case BxoVKind::TupleK:
      new(&_tup)  std::shared_ptr<const BxoTuple>(std::move(v._tup));
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
  _ptr = nullptr;
  new(this) BxoVal(s);
  return *this;
} // end BxoVal::operator =(BxoVal&&)


/// see also http://stackoverflow.com/a/28613483/841108
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
      _str.~shared_ptr<const BxoString>();;
      break;
    case BxoVKind::ObjectK:
      _obj.~shared_ptr<BxoObject>();
      break;
    case BxoVKind::SetK:
      _set.~shared_ptr<const BxoSet>();
    case BxoVKind::TupleK:
      _tup.~shared_ptr<const BxoTuple>();
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


std::nullptr_t
BxoVal::as_null(void) const
{
  if (_kind != BxoVKind::NoneK)
    {
      BXO_BACKTRACELOG("as_null: non null value " << this);
      throw std::runtime_error("as_null: non-null value");
    }
  return nullptr;
} // end BxoVal::as_null

intptr_t
BxoVal::as_int(void) const
{
  if (_kind != BxoVKind::IntK)
    {
      BXO_BACKTRACELOG("as_int: non-int value " << this);
      throw std::runtime_error("as_int: non-null value");
    }
  return _int;
} // end BxoVal::as_int

std::shared_ptr<const BxoString>
BxoVal::as_bstring(void) const
{
  if (_kind != BxoVKind::StringK)
    {
      BXO_BACKTRACELOG("as_bstring: non-string value " << this);
      throw std::runtime_error("as_bstring: non-string value");
    }
  return _str;
} // end of BxoVal::as_bstring

const BxoString*
BxoVal::get_bstring(void) const
{
  if (_kind != BxoVKind::StringK)
    {
      BXO_BACKTRACELOG("get_bstring: non-string value " << this);
      throw std::runtime_error("get_bstring: non-string value");
    }
  return _str.get();
}

std::shared_ptr<const BxoString>
BxoVal::to_bstring(const std::shared_ptr<const BxoString>& def) const
{
  if (_kind != BxoVKind::StringK) return def;
  return _str;
} // end of BxoVal::to_bstring

std::string
BxoVal::as_string(void) const
{
  if (_kind != BxoVKind::StringK)
    {
      BXO_BACKTRACELOG("as_string: non-string value " << this);
      throw std::runtime_error("as_string: non-string value");
    }
  return _str->string();
} // end of BxoVal::as_string

std::string
BxoVal::to_string(const std::string&def) const
{
  if (_kind != BxoVKind::StringK) return def;
  return _str->string();
} // end BxoVal::to_string

std::shared_ptr<const BxoSet>
BxoVal::as_set(void) const
{
  if (_kind != BxoVKind::SetK)
    {
      BXO_BACKTRACELOG("as_set: non-st value " << this);
      throw std::runtime_error("as_set: non-set value");
    }
  return _set;
} // end BxoVal::as_set

std::shared_ptr<const BxoSet>
BxoVal::to_set(const std::shared_ptr<const BxoSet> def) const
{
  if (_kind != BxoVKind::SetK) return def;
  return _set;
}

const BxoSet*
BxoVal::get_set(void) const
{
  if (_kind != BxoVKind::SetK)
    {
      BXO_BACKTRACELOG("get_set: non-set value " << this);
      throw std::runtime_error("get_set: non-set value");
    }
  return _set.get();
} // end of BxoVal::get_set

std::shared_ptr<const BxoTuple>
BxoVal::as_tuple(void) const
{
  if (_kind != BxoVKind::TupleK)
    {
      BXO_BACKTRACELOG("as_tuple: non-tuple value " << this);
      throw std::runtime_error("as_tuple: non-tuple value");
    }
  return _tup;
} // end BxoVal::as_tuple

std::shared_ptr<const BxoTuple>
BxoVal::to_tuple(const std::shared_ptr<const BxoTuple> def) const
{
  if (_kind != BxoVKind::TupleK) return def;
  return _tup;
}

const BxoTuple*
BxoVal::get_tuple(void) const
{
  if (_kind != BxoVKind::TupleK)
    {
      BXO_BACKTRACELOG("get_tuple: non-tuple value " << this);
      throw std::runtime_error("get_tuple: non-tuple value");
    }
  return _tup.get();
} // end of BxoVal::get_tuple



std::shared_ptr<const BxoSequence>
BxoVal::as_sequence(void) const
{
  if (_kind == BxoVKind::TupleK)
    return _tup;
  else if (_kind == BxoVKind::SetK)
    return _set;
  else
    {
      BXO_BACKTRACELOG("as_sequence: non-sequence value " << this);
      throw std::runtime_error("as_sequence: non-sequence value");
    }
} // end BxoVal::as_tuple

std::shared_ptr<const BxoSequence>
BxoVal::to_sequence(const std::shared_ptr<const BxoSequence> def) const
{
  if (_kind == BxoVKind::TupleK)
    return _tup;
  else if (_kind == BxoVKind::SetK)
    return _set;
  else return def;
}

const BxoSequence*
BxoVal::get_sequence(void) const
{
  if (_kind == BxoVKind::TupleK)
    return _tup.get();
  else if (_kind == BxoVKind::SetK)
    return _set.get();
  else
    {
      BXO_BACKTRACELOG("get_sequence: non-sequence value " << this);
      throw std::runtime_error("get_sequence: non-sequence value");
    }
} // end of BxoVal::get_sequence


std::shared_ptr<BxoObject>
BxoVal::as_object(void) const
{
  if (_kind != BxoVKind::ObjectK)
    {
      BXO_BACKTRACELOG("as_object: non-object value " << this);
      throw std::runtime_error("as_object: non-object value");
    }
  return _obj;
} // end BxoVal::as_object

std::shared_ptr<BxoObject>
BxoVal::to_object(const std::shared_ptr<BxoObject> def) const
{
  if (_kind != BxoVKind::ObjectK) return def;
  return _obj;
}

BxoObject*
BxoVal::get_object(void) const
{
  if (_kind != BxoVKind::ObjectK)
    {
      BXO_BACKTRACELOG("get_object: non-object value " << this);
      throw std::runtime_error("get_object: non-object value");
    }
  return _obj.get();
} // end of BxoVal::get_object

BxoObject*
BxoVal::as_objptr(void) const
{
  if (_kind != BxoVKind::ObjectK)
    {
      BXO_BACKTRACELOG("as_objptr: non-object value " << this);
      throw std::runtime_error("as_objptr: non-object value");
    }
  return _obj.get();
} // end BxoVal::as_objptr

BxoObject*
BxoVal::to_objptr(BxoObject*defobp) const
{
  if (_kind == BxoVKind::ObjectK) return _obj.get();
  return defobp;
} // end of BxoVal::to_objptr
////////////////

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



////////////////////////////////////////////////////////////////
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
  static std::unordered_map<BxoObject*,std::string> _namemap_;
  static inline void register_in_bucket(BxoObject*pob)
  {
    _bucketarr_[hi_id_bucketnum(pob->_hid)].insert(pob);
  }
public:
  inline bool has_attr(const std::shared_ptr<BxoObject> pobat) const;
  inline BxoVal get_attr(const std::shared_ptr<BxoObject> pobat) const;
  inline BxoVal get_comp(int rk) const;
  unsigned nb_attrs() const
  {
    return _attrh.size();
  };
  unsigned nb_comps() const
  {
    return _compv.size();
  };
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
    BXO_VERBOSELOG("BxoObject Predef strid:"<< strid() << " @" << (void*)this);
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
    BXO_VERBOSELOG("BxoObject Loaded strid:"<< strid() << " @" << (void*)this);
  };
  static void initialize_predefined_objects (void);
  static BxoVal set_of_predefined_objects (void);
  static const std::set<std::string> all_names (void);
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
  Bxo_hid_t hid() const
  {
    return _hid;
  };
  Bxo_loid_t loid() const
  {
    return _loid;
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
  inline bool alpha_less(const BxoObject&r) const;
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
  static BxoObject*find_named_objptr(const std::string&str)
  {
    return find_named_objref(str).get();
  }
  bool is_named(void) const
  {
    return _namemap_.find(const_cast<BxoObject*>(this)) != _namemap_.end();
  }
  std::string name(void) const
  {
    auto it = _namemap_.find(const_cast<BxoObject*>(this));
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
    return std::shared_ptr<BxoObject> {pob};
  }
  static std::shared_ptr<BxoObject> load_objref(BxoLoader&ld, const std::string& idstr);
  void load_content(const BxoJson&, BxoLoader&);
  void load_set_class(std::shared_ptr<BxoObject> obclass, BxoLoader&);
  void load_set_payload(BxoPayload*payl, BxoLoader&);
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
  template <class PaylClass>
  PaylClass*dyncast_payload() const
  {
    return dynamic_cast<PaylClass*>(_payl.get());
  }
  template <class PaylClass>
  PaylClass*dynget_payload() const
  {
    auto py = _payl.get();
    if (!py)
      {
        BXO_BACKTRACELOG("dynget_payload no payload for " << this
                         << " but expecting " << bxo_demangled_typename(typeid(PaylClass)));
        throw std::runtime_error("dynget_payload no payload");
      }
    auto dp = dynamic_cast<PaylClass*>(py);
    if (!dp)
      {
        BXO_BACKTRACELOG("dynget_payload expecting "
                         << bxo_demangled_typename(typeid(PaylClass))
                         << " but got " << bxo_demangled_typename(typeid(*py)));
        throw std::runtime_error("dynget_payload inappropriate payload");
      }
    return dp;
  }
  template <class PaylClass, typename... Args> PaylClass* put_payload(Args... args)
  {
    auto py = new PaylClass(this, args...);
    _payl.reset(py);
    return py;
  }
  void reset_payload()
  {
    _payl.reset();
  };
};        // end class BxoObject



bool BxoObject::alpha_less(const BxoObject&r) const
{
  if (this == &r) return false;
  {
    auto tn = name();
    auto rn = r.name();
    if (!tn.empty())
      {
        if (rn.empty()) return true;
        return tn < rn;
      }
    if (!rn.empty()) return false;
  }
  if (_hid >= r._hid) return false;
  if (_hid < r._hid) return true;
  return _loid < r._loid;
};        // end BxoObject::alpha_less


bool
BxoObject::has_attr(const std::shared_ptr<BxoObject> pobat) const
{
  if (!pobat) return false;
  return _attrh.find(pobat) != _attrh.end();
} // end of BxoObject::has_attr

BxoVal
BxoObject::get_attr(const std::shared_ptr<BxoObject> pobat) const
{
  if (!pobat) return nullptr;
  auto pit = _attrh.find(pobat);
  if (pit == _attrh.end()) return nullptr;
  return pit->second;
} // end of BxoObject::get_attr


BxoVal
BxoObject::get_comp(int rk) const
{
  auto nbc = nb_comps();
  if (rk<0) rk += nbc;
  if (rk<0 || rk>=(int)nbc) return nullptr;
  return _compv[rk];
} // end of BxoObject::get_comp

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



////////////////
class BxoPayload
{
  friend class BxoObject;
  friend class BxoLoader;
  friend class BxoVal;
  friend class std::unique_ptr<BxoPayload>;
  friend class std::shared_ptr<BxoObject>;
  BxoObject*const _owner;
protected:
  struct PayloadTag {};
  BxoPayload(BxoObject& own, PayloadTag) : _owner(&own) {};
  BxoPayload(BxoObject& own, BxoLoader&) : _owner(&own) {};
public:
  typedef BxoPayload*loader_create_sigt (BxoObject*,BxoLoader*);
  // each Payload class Foo of kind object of id KindId comes with a function
  /// extern "C" loader_create_sigt bxoload_KindId;
  static constexpr const char* loader_prefix = "bxoload";
  virtual ~BxoPayload() {};
  BxoPayload(BxoPayload&&) = delete;
  BxoPayload(const BxoPayload&) = delete;
  virtual std::shared_ptr<BxoObject> kind_ob() const =0;
  virtual std::shared_ptr<BxoObject> module_ob() const =0;
  virtual void scan_payload_content(BxoDumper&) const =0;
  virtual const BxoJson emit_payload_content(BxoDumper&) const =0;
  virtual void load_payload_content(const BxoJson&, BxoLoader&) =0;
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


bool BxoAlphaLessObjSharedPtr::operator() (const std::shared_ptr<BxoObject>&lp, const std::shared_ptr<BxoObject>&rp) const
{
  if (!lp)
    {
      return !rp;
    };
  if (!rp) return false;
  return lp->alpha_less(*rp);
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

BxoVSet::BxoVSet(const std::unordered_set<std::shared_ptr<BxoObject>,BxoHashObjSharedPtr>&uset) :
  BxoVal(TagSet {}, BxoSet::make_set(uset)) {}

BxoVSet::BxoVSet(const std::set<std::shared_ptr<BxoObject>,BxoLessObjSharedPtr>& s)
  : BxoVal(TagSet {},BxoSet::make_set(s)) {}

BxoVSet::BxoVSet(const std::vector<std::shared_ptr<BxoObject>>&vs)
  : BxoVal(TagSet {}, BxoSet::make_set(vs)) {}

BxoVSet::BxoVSet(const std::vector<BxoObject*>&vo)
  : BxoVal(TagSet {}, BxoSet::make_set(vo)) {}

BxoVTuple::BxoVTuple(const BxoTuple& tup)
  :  BxoVal(TagTuple {},&tup) {}


inline std::ostream& operator << (std::ostream& os,  std::shared_ptr<BxoObject> pob)
{
  if (pob)
    os << pob->pname();
  else
    os << "~";
  return os;
};

inline std::ostream& operator << (std::ostream& os, const BxoObject* pob)
{
  if (pob)
    os << pob->pname();
  else
    os << "~";
  return os;
};

inline std::ostream& operator << (std::ostream& os, const BxoVal&v)
{
  v.out(os);
  return os;
}
////////////////
class BxoHashsetPayload final : public BxoPayload
{
  std::unordered_set<std::shared_ptr<BxoObject>,BxoHashObjSharedPtr> _hset;
public:
  virtual std::shared_ptr<BxoObject> kind_ob() const;
  virtual std::shared_ptr<BxoObject> module_ob() const;
  virtual void scan_payload_content(BxoDumper&) const;
  virtual const BxoJson emit_payload_content(BxoDumper&) const;
  virtual void load_payload_content(const BxoJson&, BxoLoader&);
  BxoHashsetPayload(BxoObject& own);
  virtual ~BxoHashsetPayload();
  void add(std::shared_ptr<BxoObject> pob)
  {
    if (pob) _hset.insert(pob);
  };
  void remove(std::shared_ptr<BxoObject> pob)
  {
    if (pob) _hset.erase(pob);
  };
  BxoVal vset() const
  {
    return BxoVSet(_hset);
  };
  bool contains(std::shared_ptr<BxoObject> pob) const
  {
    return pob && _hset.find(pob) != _hset.end();
  };
  void clear(void)
  {
    _hset.clear();
  }
};        // end class BxoHashsetPayload

#endif /*BASIXMO_HEADER*/
