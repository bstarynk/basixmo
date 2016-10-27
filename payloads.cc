// file payloads.cc - some payloads

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

extern "C" BxoPayload::loader_create_sigt
bxoload_5JG8lVw6jwlUT7PLK BXO_OPTIMIZEDFUN; // payload_assoval
extern "C" BxoPayload::loader_create_sigt
bxoload_8261sbF1f9ohzu2Iu BXO_OPTIMIZEDFUN; // payload_hashset
extern "C" BxoPayload::loader_create_sigt
bxoload_7yw2rDBxjs1XBu2ee BXO_OPTIMIZEDFUN; // payload_system

extern "C" BxoPayload::loader_create_sigt bxoload_payload_assoval;
extern "C" BxoPayload::loader_create_sigt bxoload_payload_hashset;
extern "C" BxoPayload::loader_create_sigt bxoload_payload_system;

BxoPayload*bxoload_5JG8lVw6jwlUT7PLK(BxoObject*obj,BxoLoader*ld)
{
  return bxoload_payload_assoval(obj,ld);
}

BxoPayload*bxoload_8261sbF1f9ohzu2Iu(BxoObject*obj,BxoLoader*ld)
{
  return bxoload_payload_hashset(obj,ld);
}

BxoPayload*bxoload_7yw2rDBxjs1XBu2ee(BxoObject*obj,BxoLoader*ld)
{
  return bxoload_payload_system(obj,ld);
}

class BxoAssovalPayload final : public BxoPayload
{
  std::unordered_map<const std::shared_ptr<BxoObject>,BxoVal,BxoHashObjSharedPtr> _asso;
public:
  virtual std::shared_ptr<BxoObject> kind_ob() const
  {
    return BXO_VARPREDEF(payload_assoval);
  };
  virtual std::shared_ptr<BxoObject> module_ob() const
  {
    return nullptr;
  };
  virtual void scan_payload_content(BxoDumper&) const;
  virtual const BxoJson emit_payload_content(BxoDumper&) const;
  virtual void load_payload_content(const BxoJson&, BxoLoader&);
  BxoAssovalPayload(BxoObject& own)
    : BxoPayload(own, PayloadTag {}),
  _asso() {};
  ~BxoAssovalPayload()
  {
    _asso.clear();
  };
}; // end class BxoAssovalPayload



void
BxoAssovalPayload::scan_payload_content(BxoDumper&du) const
{
  for (auto &p : _asso)
    {
      if (du.scan_dumpable(p.first.get()))
        {
          p.second.scan_dump(du);
        }
    }
} // end BxoAssovalPayload::scan_payload_content

const BxoJson
BxoAssovalPayload::emit_payload_content(BxoDumper&du) const
{
  std::set<std::shared_ptr<BxoObject>,BxoLessObjSharedPtr> atset;
  for (const auto& p: _asso)
    {
      auto atob = p.first;
      if (!du.is_dumpable(atob)) continue;
      atset.insert(atob);
    }
  BxoJson job {Json::objectValue};
  BxoJson jarr {Json::arrayValue};
  for (auto pob: atset)
    {
      const std::shared_ptr<BxoObject> cpob = pob;
      auto pv = _asso.find(cpob);
      if (pv == _asso.end()) continue;
      const BxoVal& aval = pv->second;
      BxoJson jpair {Json::objectValue};
      jpair["at"] = pob->strid();
      jpair["va"] = aval.to_json(du);
      jarr.append(jpair);
    }
  job["@owner"] = owner()->strid();
  job["assoval"] = jarr;
  return job;
} // end BxoAssovalPayload::emit_payload_content

void
BxoAssovalPayload::load_payload_content(const BxoJson&jv, BxoLoader&ld)
{
  if (jv.isMember("assoval"))
    {
      auto jass = jv["assoval"];
      if (jass.isArray())
        {
          auto nbat = jass.size();
          _asso.reserve(5*nbat/4+1);
          for (int ix=0; ix<(int)nbat; ix++)
            {
              const BxoJson& jpair = jass[ix];
              if (!jpair.isObject()) continue;
              const BxoJson& jat = jpair["at"];
              if (!jat.isString()) continue;
              auto pobat = ld.find_loadedobj(jat.asString());
              if (!pobat) continue;
              const BxoJson& jva = jpair["va"];
              BxoVal aval = BxoVal::from_json(ld,jva);
              _asso.insert({pobat,aval});
            }
        }
    }
} // end BxoAssovalPayload::load_payload_content


BxoPayload*
bxoload_payload_assoval(BxoObject*obj,BxoLoader*ld)
{
  BXO_ASSERT (obj != nullptr, "null obj");
  BXO_ASSERT (ld != nullptr, "no loader");
  return new BxoAssovalPayload(*obj);
}




////////////////
class BxoHashsetPayload final : public BxoPayload
{
  std::unordered_set<std::shared_ptr<BxoObject>,BxoHashObjSharedPtr> _hset;
public:
  virtual std::shared_ptr<BxoObject> kind_ob() const
  {
    return BXO_VARPREDEF(payload_hashset);
  };
  virtual std::shared_ptr<BxoObject> module_ob() const
  {
    return nullptr;
  };
  virtual void scan_payload_content(BxoDumper&) const;
  virtual const BxoJson emit_payload_content(BxoDumper&) const;
  virtual void load_payload_content(const BxoJson&, BxoLoader&);
  BxoHashsetPayload(BxoObject& own)
    : BxoPayload(own, PayloadTag {}),
  _hset() {};
  ~BxoHashsetPayload()
  {
    _hset.clear();
  };
};        // end class BxoHashsetPayload

void
BxoHashsetPayload::scan_payload_content(BxoDumper&du) const
{
  for (auto pob : _hset)
    du.scan_dumpable(pob.get());
} // end of BxoHashsetPayload::scan_payload_content

const BxoJson
BxoHashsetPayload::emit_payload_content(BxoDumper&du) const
{
  std::set<std::shared_ptr<BxoObject>,BxoLessObjSharedPtr> elset;
  for (auto pob: _hset)
    {
      BXO_ASSERT(pob, "null element");
      if (!du.is_dumpable(pob)) continue;
      elset.insert(pob);
    }
  BxoJson job {Json::objectValue};
  BxoJson jarr {Json::arrayValue};
  for (auto pob: elset)
    {
      jarr.append(pob->strid());
    }
  job["@owner"] = owner()->strid();
  job["hashset"] = jarr;
  return job;
} // end of BxoHashsetPayload::emit_payload_content

void
BxoHashsetPayload::load_payload_content(const BxoJson&jv, BxoLoader&ld)
{
  if (jv.isMember("hashset"))
    {
      auto jhs = jv["hashset"];
      if (jhs.isArray())
        {
          auto nbel = jhs.size();
          _hset.reserve(5*nbel/4+1);
          for (int ix=0; ix<(int)nbel; ix++)
            {
              const BxoJson& jel = jhs[ix];
              if (!jel.isString()) continue;
              auto pobel = ld.find_loadedobj(jel.asString());
              if (!pobel) continue;
              _hset.insert(pobel);
            }
        }
    }
} // end of BxoHashsetPayload::load_payload_content


BxoPayload*
bxoload_payload_hashset(BxoObject*obj,BxoLoader*ld)
{
  BXO_ASSERT (obj != nullptr, "null obj");
  BXO_ASSERT (ld != nullptr, "no loader");
  return new BxoHashsetPayload(*obj);
}

////////////////
class BxoSystemPayload final : public BxoPayload
{
  std::string _predefpath;
  std::string _globalpath;
public:
  void generate_predef(BxoDumper*du) const;
  void generate_global(BxoDumper*du) const;
  virtual std::shared_ptr<BxoObject> kind_ob() const
  {
    return BXO_VARPREDEF(payload_system);
  };
  virtual std::shared_ptr<BxoObject> module_ob() const
  {
    return nullptr;
  };
  virtual void scan_payload_content(BxoDumper&) const;
  virtual const BxoJson emit_payload_content(BxoDumper&) const;
  virtual void load_payload_content(const BxoJson&, BxoLoader&);
  BxoSystemPayload(BxoObject& own)
    : BxoPayload(own, PayloadTag {}),
  _predefpath("_bxo_predef.h"), _globalpath("_bxo_global.h") {};
  ~BxoSystemPayload()
  {
    _predefpath.clear();
    _globalpath.clear();
  };
};        // end class BxoSystemPayload

void
BxoSystemPayload::scan_payload_content(BxoDumper&du) const
{
  BXO_VERBOSELOG("owner=" << owner()
                 << " predefpath=" << _predefpath << " globalpath=" << _globalpath);
  if (!_predefpath.empty())
    du.do_after_scan([=](BxoDumper&dum,BxoVal)
    {
      generate_predef(&dum);
    },nullptr);
  if (!_globalpath.empty())
    du.do_after_scan([=](BxoDumper&dum,BxoVal)
    {
      generate_global(&dum);
    },nullptr);
} // end of BxoSystemPayload::scan_payload_content



void
BxoSystemPayload::generate_predef(BxoDumper*pdu) const
{
  BXO_ASSERT(pdu != nullptr, "missing dumper");
  auto prpath = pdu->output_path(_predefpath);
  BXO_VERBOSELOG("should generate predef owner=" << owner()
                 << " _predefpath=" << _predefpath
                 << " prpath=" << prpath);
  std::ofstream os(prpath);
  os << BxoGplv3LicenseOut(_predefpath, "//", "") << std::flush;
  auto prset = BxoObject::set_of_predefined_objects().get_set();
  std::vector<std::shared_ptr<BxoObject>> prvec;
  prvec.reserve(prset->length()+1);
  auto pbeg = prset->begin();
  auto pend = prset->end();
  for (auto p = pbeg; p<pend; p++)
    {
      auto pob = *p;
      prvec.push_back(pob);
    }
  std::sort(prvec.begin(), prvec.end(),
            [=](const std::shared_ptr<BxoObject>&l,
                const std::shared_ptr<BxoObject>&r)
  {
    return l->pname () < r->pname();
  });
  os << std::endl
     << "#ifndef BXO_HAS_PREDEFINED" << std::endl
     << "#error missing BXO_HAS_PREDEFINED" << std::endl
     << "#endif /*BXO_HAS_PREDEFINED*/" << std::endl;
  os << std::endl
     << "#undef BXO_NB_PREDEFINED" << std::endl
     << "#define BXO_NB_PREDEFINED  " << prvec.size() << std::endl;

  os << "/// BXO_HAS_PREDEFINED(Nam,Idstr,Hid,Loid,Hash)" << std::endl;
  os << std::endl;

  for (auto pob: prvec)
    {
      BXO_ASSERT(pob, "null pob");
      os << std::endl
         << "BXO_HAS_PREDEFINED(" << pob->pname()
         << "," << pob->strid()
         << "," << pob->loid()
         << "," << pob->hid()
         << "," << pob->hash()
         << ")" << std::endl;
    }
  os << std::endl << "#undef BXO_HAS_PREDEFINED" << std::endl << std::endl;
  os << std::endl << "/* end of generated file " << _predefpath << " */"
     << std::endl;
} // end of BxoSystemPayload::generate_predef



void BxoSystemPayload::generate_global(BxoDumper*pdu) const
{
  BXO_ASSERT(pdu != nullptr, "missing dumper");
  BXO_VERBOSELOG("should generate global owner=" << owner()
                 << " _globalpath=" << _globalpath);
} // end of BxoSystemPayload::generate_global



const BxoJson
BxoSystemPayload::emit_payload_content(BxoDumper&du BXO_UNUSED) const
{
  BxoJson job {Json::objectValue};
  job["@owner"] = owner()->strid();
  job["system"] = true;
  job["predefpath"] = _predefpath;
  job["globalpath"] = _globalpath;
  return job;
} // end of BxoSystemPayload::emit_payload_content

void
BxoSystemPayload::load_payload_content(const BxoJson&jv, BxoLoader&ld BXO_UNUSED)
{
  if (jv.isMember("system"))
    {
      auto jpredefp = jv["predefpath"];
      if (jpredefp.isString())
        _predefpath = jpredefp.asString();
      auto jglobalp = jv["globalpath"];
      if (jglobalp.isString())
        _globalpath = jglobalp.asString();
    }
} // end of BxoSystemPayload::load_payload_content


BxoPayload*
bxoload_payload_system(BxoObject*obj,BxoLoader*ld)
{
  BXO_ASSERT (obj != nullptr, "null obj");
  BXO_ASSERT (ld != nullptr, "no loader");
  BXO_VERBOSELOG("obj=" << obj);
  return new BxoSystemPayload(*obj);
}
