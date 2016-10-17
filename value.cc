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
      return BxoJson((Json::Int64)_int);
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
#warning incomplete BxoVal::from_json
      /**
      case Json::stringValue:
      return BxoVString(js.asString());
      **/
    }
} // end of BxoVal::from_json

BxoVString::BxoVString(const BxoString&bs)
{
#warning incomplete BxoVString::BxoVString
}
