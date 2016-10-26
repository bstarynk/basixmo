// file value.cc - value functions

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


bool BxoVal::less(const BxoVal&r) const
{
  if (_kind < r._kind) return true;
  if (_kind > r._kind) return false;
  switch (_kind)
    {
    case BxoVKind::NoneK:
      return false;
    case BxoVKind::IntK:
      return _int < r._int;
    case BxoVKind::StringK:
      return _str->less_than_string(*(r._str));
    case BxoVKind::ObjectK:
      return _obj->less(*(r._obj));
    case BxoVKind::SetK:
      return _set->less_than_set(*(r._set));
    case BxoVKind::TupleK:
      return _tup->less_than_tuple(*(r._tup));
    }
  return false;
} // end BxoVal::less

bool BxoVal::less_equal(const BxoVal&r) const
{
  if (_kind < r._kind) return true;
  if (_kind > r._kind) return false;
  switch (_kind)
    {
    case BxoVKind::NoneK:
      return true;
    case BxoVKind::IntK:
      return _int <= r._int;
    case BxoVKind::StringK:
      return _str->less_equal_string(*r._str);
    case BxoVKind::ObjectK:
      return _obj->less_equal(*r._obj);
    case BxoVKind::SetK:
      return _set->less_equal_set(*r._set);
      break;
    case BxoVKind::TupleK:
      return _tup->less_equal_tuple(*r._tup);
    }
  return false;
} // end BxoVal::less_equal

BxoJson
BxoSequence::sequence_to_json(BxoDumper&du) const
{
  std::vector<BxoJson> vecj;
  int l = length();
  vecj.reserve(l);
  for (int ix=0; ix<l; ix++)
    {
      if (_seq[ix] && du.is_dumpable(_seq[ix]))
        vecj.push_back(_seq[ix]->id_to_json());
    }
  BxoJson js = BxoJson(Json::arrayValue);
  int jlen = vecj.size();
  js.resize(jlen);
  for (int ix=0; ix<jlen; ix++) js[ix] = vecj[ix];
  return js;
} // end BxoSequence::sequence_to_json

BxoHash_t BxoString::hash_cstring(const char*s, int ln)
{
  if (!s)
    {
      if (ln>0)
        {
          BXO_BACKTRACELOG("hash_cstring: invalid null string of length:" << ln);
          throw std::runtime_error("invalid null string to hash");
        }
      return 0;
    }
  if (ln<0) ln = strlen(s);
  if (ln>BXO_SIZE_MAX)
    {
      char buf[32];
      memset (buf, 0, sizeof(buf));
      strncpy(buf, s, sizeof(buf)-1);
      BXO_BACKTRACELOG("hash_cstring: too long string of " << ln << " bytes: " << buf);
      throw std::runtime_error("too long string to hash");
    }
  int l = ln;
  BxoHash_t h1 = 0, h2 = ln, h = 0;
  const char*str = s;
  while (l > 4)
    {
      h1 =
        (509 * h2 +
         307 * ((signed char *) str)[0]) ^ (1319 * ((signed char *) str)[1]);
      h2 =
        (17 * l + 5 + 5309 * h2) ^ ((3313 * ((signed char *) str)[2]) +
                                    9337 * ((signed char *) str)[3] + 517);
      l -= 4;
      str += 4;
    }
  if (l > 0)
    {
      h1 = (h1 * 7703) ^ (503 * ((signed char *) str)[0]);
      if (l > 1)
        {
          h2 = (h2 * 7717) ^ (509 * ((signed char *) str)[1]);
          if (l > 2)
            {
              h1 = (h1 * 9323) ^ (11 + 523 * ((signed char *) str)[2]);
              if (l > 3)
                {
                  h2 =
                    (h2 * 7727 + 127) ^ (313 +
                                         547 * ((signed char *) str)[3]);
                }
            }
        }
    }
  h = (h1 * 29311 + 59) ^ (h2 * 7321 + 120501);
  if (!h)
    {
      h = h1 ? h1 : h2;
      if (!h)
        h = (ln & 0xffffff) + 11;
    }
  return h;
} // end  BxoString::hash_cstring


BxoJson
BxoVal::to_json(BxoDumper&du) const
{
  switch (_kind)
    {
    case BxoVKind::NoneK:
      return BxoJson::nullSingleton();
    case BxoVKind::IntK:
      return BxoJson((long long)_int);
    case BxoVKind::StringK:
      return BxoJson(_str->string());
    case BxoVKind::ObjectK:
      if (du.is_dumpable(_obj))
        {
          BxoJson job(Json::objectValue);
          job["oid"] = _obj->id_to_json();
          return job;
        }
      else return BxoJson::nullSingleton();
    case BxoVKind::SetK:
    {
      BxoJson job(Json::objectValue);
      job["set"] = _set->sequence_to_json(du);
      return job;
    };
    case BxoVKind::TupleK:
    {
      BxoJson job(Json::objectValue);
      job["tup"] = _set->sequence_to_json(du);
      return job;
    };
    }
  return BxoJson::nullSingleton();
} // end BxoVal::to_json

BxoVal
BxoVal::from_json(BxoLoader& ld, const BxoJson&js)
{
  switch(js.type())
    {
    case Json::nullValue:
      return BxoVNone();
    case Json::intValue:
    case Json::uintValue:
      return BxoVInt(js.asInt64());
    case Json::stringValue:
      return BxoVString(js.asString());
    case Json::arrayValue:
      return ld.val_from_json(js);
    case Json::objectValue:
    {
      if (js.isMember("oid"))
        {
          const auto& jid = js["oid"];
          if (jid.isString())
            {
              auto idstr = jid.asString();
              BxoObject* pob = ld.obj_from_idstr(idstr);
              if (pob)
                return BxoVObj(pob);
            }
        }
      else if (js.isMember("set"))
        {
          const auto& jset = js["set"];
          if (jset.isArray())
            {
              auto pset = BxoSet::load_set(ld,jset);
              if (pset)
                return BxoVSet(pset);
            }
        }
      else if (js.isMember("tup"))
        {
          const auto& jtup = js["tup"];
          if (jtup.isArray())
            {
              auto ptup = BxoTuple::load_tuple(ld,jtup);
              if (ptup)
                return BxoVTuple(ptup);
            }
        }
    }
#warning incomplete BxoVal::from_json
    }
  BXO_BACKTRACELOG("BxoVal::from_json: bad json " << js);
  throw std::runtime_error("BxoVal::from_json: bad json");
} // end of BxoVal::from_json



BxoVString::BxoVString(const std::string& s)
  : BxoVal(TagString {},s)
{
}

BxoVString::BxoVString(const BxoString&bs)
  : BxoVal(TagString {},&bs)
{
}

BxoSet
BxoSet::the_empty_set {BxoSet::init_hash,0,nullptr};

const BxoSet*
BxoSet::make_set(const std::set<std::shared_ptr<BxoObject>,BxoLessObjSharedPtr>&bs)
{
  auto siz = bs.size();
  if (BXO_UNLIKELY(siz <= 1))
    {
      if (siz==0) return &the_empty_set;
      std::shared_ptr<BxoObject>pob = *bs.begin();
      if (!pob)
        {
          BXO_BACKTRACELOG("make_set: nil element in singleton");
          throw std::runtime_error("BxoSet::make_set nil element");
        }
      auto h = combine_hash(init_hash, *pob);
      return new BxoSet(h,1,&pob);
    }
  else if (BXO_UNLIKELY(siz > BXO_SIZE_MAX))
    {
      BXO_BACKTRACELOG("make_set: too big size " << siz);
      throw std::runtime_error("BxoSet::make_set too big size");
    }
  std::vector<std::shared_ptr<BxoObject>> vec {siz};
  BxoHash_t h = init_hash;
  for (const auto&p : bs)
    {
      if (!p)
        {
          BXO_BACKTRACELOG("make_set: nil element");
          throw std::runtime_error("BxoSet::make_set nil element");
        }
      vec.push_back(p);
      h = combine_hash(h, *p);
    }
  h = adjust_hash(h,siz);
  return new BxoSet(h,siz,vec.data());
} // end BxoSet::make_set


const BxoSet*
BxoSet::make_set(const std::vector<BxoObject*>&vecptr)
{
  std::vector<std::shared_ptr<BxoObject>> vec {vecptr.size()};
  int cnt = 0;
  for (BxoObject* po: vecptr)
    {
      if (po) vec[cnt++] = po->shared_from_this();
    }
  vec.resize(cnt);
  return make_set(vec);
}

const BxoSet*
BxoSet::make_set(const std::vector<std::shared_ptr<BxoObject>>&vec)
{
  auto siz = vec.size();
  if (BXO_UNLIKELY(siz <= 1))
    {
      if (siz==0) return &the_empty_set;
      auto& pob = vec[0];
      if (!pob)
        {
          BXO_BACKTRACELOG("make_set: nil element in singleton");
          throw std::runtime_error("BxoSet::make_set nil element");
        }
      auto h = combine_hash(init_hash, *pob);
      return new BxoSet(h,1,&pob);
    }
  else if (BXO_UNLIKELY(siz > BXO_SIZE_MAX))
    {
      BXO_BACKTRACELOG("make_set: too big size " << siz);
      throw std::runtime_error("BxoSet::make_set too big size");
    }
  std::vector<std::shared_ptr<BxoObject>> copy {siz+1};
  for (unsigned ix=0; ix<(unsigned)siz; ix++)
    {
      if (BXO_UNLIKELY(!vec[ix]))
        {
          BXO_BACKTRACELOG("make_set: nil element");
          throw std::runtime_error("BxoSet::make_set nil element");
        }
      copy[ix] = vec[ix];
    }
  std::sort(copy.begin(), copy.end(), BxoLessObjSharedPtr {});
  int nbdup = 0;
  auto h = combine_hash(init_hash, *copy[0]);
  for (unsigned ix=1; ix<(unsigned)siz; ix++)
    if (copy[ix] == copy[ix-1])
      nbdup++;
    else
      h = combine_hash(h, *copy[ix]);
  if (BXO_LIKELY(nbdup==0))
    return new BxoSet(h,siz,copy.data());
  std::vector<std::shared_ptr<BxoObject>> unicopy {siz-nbdup+1};
  unicopy.push_back(copy[0]);
  for (unsigned ix=1; ix<(unsigned)siz; ix++)
    if (copy[ix] != copy[ix-1])
      unicopy.push_back(copy[ix]);
  return new BxoSet(h,siz-nbdup,unicopy.data());
} // end BxoSet::make_set

BxoTuple
BxoTuple::the_empty_tuple {BxoTuple::init_hash,0,nullptr};

const BxoTuple*
BxoTuple::make_tuple(const std::vector<BxoObject*>&vecptr)
{
  std::vector<std::shared_ptr<BxoObject>> vec {vecptr.size()};
  int cnt = 0;
  for (BxoObject* po: vecptr)
    {
      if (po) vec[cnt++] = po->shared_from_this();
    }
  vec.resize(cnt);
  return make_tuple(vec);
}

const BxoTuple*
BxoTuple::make_tuple(const std::vector<std::shared_ptr<BxoObject>>& vec)
{
  int nilcnt = 0;
  auto siz = vec.size();
  if (!siz) return &the_empty_tuple;
  BxoHash_t h = init_hash;
  for (auto po: vec)
    {
      if (BXO_UNLIKELY(!po))
        {
          nilcnt++;
        }
      else
        h = combine_hash(h, *po);
    };
  h = adjust_hash(h,siz-nilcnt);
  if (BXO_UNLIKELY(siz - nilcnt > BXO_SIZE_MAX))
    {
      BXO_BACKTRACELOG("make_tuple: too big size " << siz - nilcnt);
      throw std::runtime_error("BxoTuple::make_tuple too big size");
    }
  if (BXO_LIKELY(nilcnt == 0))
    return new BxoTuple(h,(unsigned)siz,vec.data());
  if (BXO_UNLIKELY(nilcnt == (int)siz))
    return &the_empty_tuple;
  std::vector<std::shared_ptr<BxoObject>>copyvec;
  copyvec.reserve(siz-nilcnt+1);
  for (auto po: vec)
    {
      if (BXO_LIKELY(po))
        copyvec.push_back(po);
    };
  return new BxoTuple(h,(unsigned)copyvec.size(), copyvec.data());
} // end of BxoTuple::make_tuple

void
BxoVal::scan_dump(BxoDumper&du) const
{
  switch (_kind)
    {
    case BxoVKind::NoneK:
    case BxoVKind::IntK:
    case BxoVKind::StringK:
      return;
    case BxoVKind::ObjectK:
      du.scan_dumpable(_obj.get());
      break;
    case BxoVKind::TupleK:
      _tup->sequence_scan_dump(du);
      break;
    case BxoVKind::SetK:
      _set->sequence_scan_dump(du);
      break;
    }
} // end BxoVal::scan_dump



void
BxoUtf8Out::out(std::ostream&os) const
{
  uint32_t uc = 0;
  auto it = _str.begin();
  auto end = _str.end();
  while ((uc=utf8::next(it, end)) != 0)
    {
      switch (uc)
        {
        case 0:
          os << "\\0";
          break;
        case '\"':
          os << "\\\"";
          break;
        case '\\':
          os << "\\\\";
          break;
        case '\a':
          os << "\\a";
          break;
        case '\b':
          os << "\\b";
          break;
        case '\f':
          os << "\\f";
          break;
        case '\n':
          os << "\\n";
          break;
        case '\r':
          os << "\\r";
          break;
        case '\t':
          os << "\\t";
          break;
        case '\v':
          os << "\\v";
          break;
        case '\033':
          os << "\\e";
          break;
        default:
          if (uc<127 && ::isprint((char)uc))
            os << (char)uc;
          else if (uc<=0xffff)
            {
              char buf[8];
              snprintf(buf, sizeof(buf), "\\u%04x", (int)uc);
              os << buf;
            }
          else
            {
              char buf[16];
              snprintf(buf, sizeof(buf), "\\U%08x", (int)uc);
              os << buf;
            }
        }
    }
} // end of BxoUtf8Out::out



/// only for debugging
void
BxoVal::out(std::ostream&os) const
{
  switch(_kind)
    {
    case BxoVKind::NoneK:
      os << "~";
      break;
    case BxoVKind::IntK:
      os << _int;
      break;
    case BxoVKind::StringK:
      os << '"'<< BxoUtf8Out(_str->string()) << '"';
      break;
    case BxoVKind::ObjectK:
      os << _obj->pname();
      break;
    case BxoVKind::SetK:
    {
      os << "{" ;
      _set->out(os);
      os << "}";
    }
    break;
    case BxoVKind::TupleK:
    {
      os << "{" ;
      _tup->out(os);
      os << "}";
    }
    break;
    }
} // end of BxoVal::out
