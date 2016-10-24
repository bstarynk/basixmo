// file object.cc - object functions

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
#include "basixmo.h"

std::unordered_set<std::shared_ptr<BxoObject>,BxoHashObjSharedPtr> BxoObject::_predef_set_;

std::unordered_set<BxoObject*,BxoHashObjPtr> BxoObject::_bucketarr_[BXO_HID_BUCKETMAX];
std::map<std::string,std::shared_ptr<BxoObject>> BxoObject::_namedict_;
std::unordered_map<const BxoObject*,std::string> BxoObject::_namemap_;

// we choose base 60, because with a 0-9 decimal digit then 13 extended
// digits in base 60 we can express a 80-bit number.  Notice that
// log(2**80/10)/log(60) is 12.98112
//...  and capital letters O & Q are missing on purpose
#define ID_DIGITS_BXO "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNPRSTUVWXYZ"
#define ID_BASE_BXO 60

static_assert (sizeof (ID_DIGITS_BXO) - 1 == ID_BASE_BXO,
               "invalid number of id digits");

static inline const char *
num80_to_char14_bxo (Bxo_uint128_t num, char *buf)
{
  Bxo_uint128_t initnum = num;
  for (int ix = 13; ix > 0; ix--)
    {
      unsigned dig = num % ID_BASE_BXO;
      num = num / ID_BASE_BXO;
      buf[ix] = ID_DIGITS_BXO[dig];
    }
  if (BXO_UNLIKELY (num > 9))
    {
      BXO_BACKTRACELOG ("bad num" << (int) num << " for "
                        << (unsigned long long) (initnum >> 64)
                        << "&"
                        << (unsigned long long) (initnum));
      throw std::runtime_error("num80_to_char14_bxo: bad num");
    }
  buf[0] = '0' + num;
  return buf;
}

static inline Bxo_uint128_t
char14_to_num80_bxo (const char *buf)
{
  Bxo_uint128_t num = 0;
  if (buf[0] < '0' || buf[0] > '9')
    return 0;
  const char *idigits = ID_DIGITS_BXO;
  for (int ix = 0; ix < 14; ix++)
    {
      char c = buf[ix];
      const char *p = strchr (idigits, c);
      if (!p)
        return 0;
      num = (num * ID_BASE_BXO + (Bxo_uint128_t) (p - idigits));
    }
  return num;
}

std::string BxoObject::str_from_hid_loid(Bxo_hid_t hid, Bxo_loid_t loid)
{
  if (hid==0 && loid==0) return "";
  if (!hid || !loid)
    {
      BXO_BACKTRACELOG("str_from_hid_loid: bad hid=" << hid << ", loid=" << loid);
      throw std::runtime_error("str_from_hid_loid: invalid id");
    }
  char buf[BXO_CSTRIDSIZ];
  memset (buf, 0, sizeof(buf));
  unsigned bn = hi_id_bucketnum(hid);
  char d0 = '0' + bn / (60 * 60);
  bn = bn % (60 * 60);
  char c1 = ID_DIGITS_BXO[bn / 60];
  char c2 = ID_DIGITS_BXO[bn % 60];
  Bxo_uint128_t wn =
    ((Bxo_uint128_t) (hid & 0xffff) << 64) + (Bxo_uint128_t) loid;
  char s16[16];
  memset (s16, 0, sizeof (s16));
  num80_to_char14_bxo (wn, s16);
  buf[0] = '_';
  buf[1] = d0;
  buf[2] = c1;
  buf[3] = c2;
  strcpy(buf+4, s16);
  return std::string {buf};
}

bool BxoObject::cstr_to_hid_loid(const char*buf, Bxo_hid_t* phid, Bxo_loid_t* ploid, const char**endp)
{
  if (!buf || buf[0] != '_' || !isdigit(buf[1]))
    return false;
  if (!phid || !ploid)
    {
      BXO_BACKTRACELOG("cstr_to_hid_loid: bad pointers phid=" << (void*)phid << ", ploid=" << (void*)ploid);
      throw std::runtime_error("cstr_to_hid_loid: bad id pointers");
    }
  const char*idigits = ID_DIGITS_BXO;
  for (int i = 2; i < BXO_CSTRIDLEN; i++)
    if (!strchr (idigits, buf[i]))
      return false;
  if (!isdigit (buf[4]))
    return false;
  unsigned bn = (buf[1] - '0') * 60 * 60
                + (strchr (idigits, buf[2]) - idigits) * 60
                + (strchr (idigits, buf[3]) - idigits);
  if (bn == 0 || bn >= BXO_HID_BUCKETMAX)
    return false;
  Bxo_uint128_t wn = char14_to_num80_bxo (buf + 4);
  Bxo_hid_t hid = 0;
  Bxo_loid_t loid = 0;
  hid = (bn << 16) + (Bxo_hid_t) (wn >> 64);
  loid = wn & (Bxo_uint128_t) 0xffffffffffffffffLL;
  *phid = hid;
  *ploid = loid;
  if (endp)
    *endp = buf+BXO_CSTRIDLEN+1;
  return true;
}



#define BXO_HAS_PREDEFINED(Name,Idstr,Hid,Loid,Hash) \
std::shared_ptr<BxoObject> BXO_VARPREDEF(Name);
#include "_bxo_predef.h"

#define BXO_HAS_GLOBAL(Name,Idstr,Hid,Loid,Hash) \
std::shared_ptr<BxoObject> BXO_VARGLOBAL(Name);
#include "_bxo_global.h"

void
BxoObject::initialize_predefined_objects(void)
{
  static bool inited;
  if (inited)
    {
      BXO_BACKTRACELOG("initialize_predefined_objects runnin more than once");
      bxo_abort();
    }
  inited = true;

#define BXO_HAS_PREDEFINED(Name,Idstr,Hid,Loid,Hash)    \
  BXO_VARPREDEF(Name) =                                 \
    std::make_shared<BxoObject>(PredefTag{},            \
           Hash,Hid,Loid);                              \
  BXO_ASSERT(hash_from_hid_loid(Hid,Loid) == Hash,      \
       "bad predefined " #Name);                        \
  _predef_set_.insert(BXO_VARPREDEF(Name));

#include "_bxo_predef.h"
  printf("created %d predefined objects\n", (int)_predef_set_.size());
  fflush(NULL);
} // end BxoObject::initialize_predefined_objects

void BxoObject::change_space(BxoSpace newsp)
{
  BXO_ASSERT(newsp < BxoSpace::_Last, "bad newsp:" << (int)newsp);
  if (_space == newsp) return;
  if (BXO_UNLIKELY(_space==BxoSpace::PredefSp))
    {
      _predef_set_.erase(shared_from_this());
    };
  if (newsp == BxoSpace::PredefSp)
    {
      _predef_set_.insert(shared_from_this());
    }
} // end BxoObject::change_space

BxoVal
BxoObject::set_of_predefined_objects ()
{
  std::set<std::shared_ptr<BxoObject>,BxoLessObjSharedPtr> pset;
  for (auto obp : _predef_set_)
    {
      BXO_ASSERT(obp, "nil predefined pointer");
      pset.insert(obp);
    }
  return BxoVSet(pset);
} // end of BxoObject::set_of_predefined_objects

BxoObject::~BxoObject()
{
  /// objects are destroyed after main, then the bucket might be empty
  auto& curbuck = _bucketarr_[hi_id_bucketnum(_hid)];
  if (!curbuck.empty())
    {
      auto it = curbuck.find(this);
      // we could have temporarily some pseudo object of this id
      if (it != curbuck.end() && *it == this)
        curbuck.erase(it);
    }
  if (!_namemap_.empty() && !_namedict_.empty())
    {
      auto it = _namemap_.find(this);
      if (it != _namemap_.end())
        {
          const std::string& nstr = it->second;
          _namedict_.erase(nstr);
          _namemap_.erase(it);
        }
    }
  _classob.reset();
  _attrh.clear();
  _compv.clear();
  _payl.reset();
} // end of BxoObject::~BxoObject

BxoObject*
BxoObject::find_from_hid_loid(Bxo_hid_t hid, Bxo_loid_t loid)
{
  unsigned bn = hi_id_bucketnum(hid, true);
  if (!bn) return nullptr;
  auto& curbuck = _bucketarr_[bn];
  /// in principle we could make a pseudoobject of hid&loid and find it in curbuck
  /// in practice, the curbuck is small enough to make that useless
  for (BxoObject*pob : curbuck)
    {
      if (pob && pob->_hid == hid && pob->_loid == loid)
        return pob;
    }
  return nullptr;
} // end of BxoObject::find_from_hid_loid

BxoObject*
BxoObject::find_from_idstr(const std::string&idstr)
{
  Bxo_hid_t hid=0;
  Bxo_loid_t loid=0;
  if (!str_to_hid_loid(idstr, &hid, &loid)) return nullptr;
  return find_from_hid_loid(hid, loid);
} // end BxoObject::find_from_idstr


BxoObject*
BxoObject::make_object(BxoSpace sp)
{
  Bxo_hid_t hid=0;
  Bxo_loid_t loid=0;
  unsigned bn=0;
  BxoObject* obres = nullptr;
  std::unordered_set<BxoObject*,BxoHashObjPtr>*pbuck = nullptr;
  do
    {
      do
        {
          hid = BxoRandom::random_32u();
        }
      while (BXO_UNLIKELY((bn=hi_id_bucketnum(hid,true))==0));
      pbuck = &_bucketarr_[bn];
      do
        {
          loid = BxoRandom::random_64u();
        }
      while (BXO_UNLIKELY(loid == 0));
      auto h = hash_from_hid_loid(hid,loid);
      obres = new BxoObject(PseudoTag {},h,hid,loid);
      if (BXO_UNLIKELY(pbuck->find(obres) != pbuck->end()))
        {
          delete obres;
          obres = nullptr;
        }
    }
  while (BXO_UNLIKELY(obres == nullptr));
  pbuck->insert(obres);
  if (sp != BxoSpace::TransientSp)
    obres->change_space(sp);
  return obres;
} // end BxoObject::make_object


std::shared_ptr<BxoObject>
BxoObject::load_objref(BxoLoader&ld, const std::string& idstr)
{
  std::shared_ptr<BxoObject> pob = ld.find_loadedobj(idstr);
  if (pob) return pob;
  Bxo_hid_t hid=0;
  Bxo_loid_t loid=0;
  if (!str_to_hid_loid(idstr,&hid,&loid))
    {
      BXO_BACKTRACELOG("load_objref bad idstr:" << idstr);
      throw std::runtime_error("BxoObject::load_objref bad idstr");
    }
  auto h = hash_from_hid_loid(hid,loid);
  pob.reset(new BxoObject(LoadedTag {},h,hid,loid));
  ld.register_objref(idstr,pob);
  return pob;
} // end BxoObject::load_objref


bool
BxoObject::valid_name(const std::string&str)
{
  if (str.empty()) return false;
  auto sz = str.size();
  if (sz > BXO_MAX_NAME_LEN) return false;
  if (!isalpha(str[0])) return false;
  for (int ix=1; ix<(int)sz; ix++)
    {
      if (isalnum(str[ix])) continue;
      else if (str[ix] == '_')
        {
          if (ix==(int)sz-1) return false;
          if (str[ix-1] == '_') return false;
        }
      else return false;
    }
  return true;
}

bool
BxoObject::register_named(const std::string&namstr)
{
  if (!valid_name(namstr))
    return false;
  if (_namedict_.find(namstr) != _namedict_.end())
    return false;
  if (_namemap_.find(this) != _namemap_.end())
    return false;
  _namedict_[namstr] = shared_from_this();
  _namemap_[this] = namstr;
  return true;
} // end BxoObject::register_named

bool
BxoObject::forget_named(void)
{
  auto it = _namemap_.find(this);
  if (it == _namemap_.end())
    return false;
  std::string nam = it->second;
  BXO_ASSERT(_namedict_.find(nam) != _namedict_.end(),
             "corrupted _namedict_ for " << nam);
  _namedict_.erase(nam);
  _namemap_.erase (it);
  return true;
} // end BxoObject::forget_named

bool
BxoObject::forget_name(const std::string&nam)
{
  auto it = _namedict_.find(nam);
  if (it == _namedict_.end())
    return false;
  BxoObject*obj = it->second.get();
  BXO_ASSERT(obj != nullptr && _namemap_.find(obj) != _namemap_.end(),
             "corrupted _namemap_ for " << nam);
  _namemap_.erase (obj);
  _namedict_.erase (it);
  return true;
} // end BxoObject::forget_name
