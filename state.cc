// file state.cc - persistence

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
#include <QtSql>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QFileInfo>

BxoLoader::BxoLoader(const std::string dirnam)
  : _ld_dirname(dirnam), _ld_sqldb(nullptr),
    _ld_startelapsedtime(bxo_elapsed_real_time()),
    _ld_startprocesstime(bxo_process_cpu_time ())
{
} // end of BxoLoader::BxoLoader


BxoObject*
BxoLoader::obj_from_idstr(const std::string&s)
{
  auto p = _ld_idtoobjmap.find(s);
  if (p != _ld_idtoobjmap.end())
    return p->second.get();
  return BxoObject::find_from_idstr(s);
}

BxoLoader::~BxoLoader()
{
  delete _ld_sqldb;
}

const BxoSet*
BxoSet::load_set(BxoLoader&ld, const BxoJson&js)
{
  if (!js.isArray()) return nullptr;
  std::vector<std::shared_ptr<BxoObject>> vec;
  auto ln = js.size();
  if (BXO_UNLIKELY(ln >  BXO_SIZE_MAX))
    {
      BXO_BACKTRACELOG("load_set: too wide set " << ln);
      throw std::runtime_error("BxoSet::load_set too wide set");
    }
  for (int ix=0; ix<(int)ln; ix++)
    {
      auto& jcomp = js[ix];
      if (!jcomp.isString())
        {
          BXO_BACKTRACELOG("load_set: invalid jcomp="
                           << jcomp << " at ix=" << ix);
          throw std::runtime_error("BxoSet::load_set invalid jcomp");
        }
      auto pob = ld.obj_from_idstr(jcomp.asString());
      if (!pob)
        {
          BXO_BACKTRACELOG("load_set: bad jcomp="
                           << jcomp << " at ix=" << ix);
          throw std::runtime_error("BxoSet::load_set bad jcomp");
        }
      vec.push_back(pob->shared_from_this());
    }
  return make_set(vec);
} // end of BxoSet::load_set


const BxoTuple*
BxoTuple::load_tuple(BxoLoader&ld, const BxoJson&js)
{
  if (!js.isArray()) return nullptr;
  std::vector<std::shared_ptr<BxoObject>> vec;
  auto ln = js.size();
  if (BXO_UNLIKELY(ln >  BXO_SIZE_MAX))
    {
      BXO_BACKTRACELOG("load_tuple: too wide tuple " << ln);
      throw std::runtime_error("BxoTuple::load_tuple too wide tuple");
    }
  for (int ix=0; ix<(int)ln; ix++)
    {
      auto& jcomp = js[ix];
      if (!jcomp.isString())
        {
          BXO_BACKTRACELOG("load_tuple: invalid jcomp="
                           << jcomp << " at ix=" << ix);
          throw std::runtime_error("BxoTuple::load_tuple invalid jcomp");
        }
      auto pob = ld.obj_from_idstr(jcomp.asString());
      if (!pob)
        {
          BXO_BACKTRACELOG("load_set: bad jcomp="
                           << jcomp << " at ix=" << ix);
          throw std::runtime_error("BxoSet::load_set bad jcomp");
        }
      vec.push_back(pob->shared_from_this());
    }
  return make_tuple(vec);
} // end of BxoTuple::load_tuple




void
BxoLoader::load()
{
  if (!QSqlDatabase::drivers().contains("QSQLITE"))
    {
      BXO_BACKTRACELOG("load: missing QSQLITE driver");
      throw std::runtime_error("BxoLoader::load missing QSQLITE");
    }
  _ld_sqldb = new QSqlDatabase();
  QString sqlitepath((_ld_dirname+"/"+basixmo_statebase+".sqlite").c_str());
  QString sqlpath((_ld_dirname+"/"+basixmo_statebase+".sql").c_str());
  if (!QFileInfo::exists(sqlitepath) || !QFileInfo::exists(sqlpath))
    {
      BXO_BACKTRACELOG("load: missing " << sqlitepath.toStdString()
                       << " or " << sqlpath.toStdString());
      throw std::runtime_error("BxoLoader::load missing file");
    }
  if (QFileInfo(sqlitepath).lastModified() > QFileInfo(sqlpath).lastModified())
    {
      BXO_BACKTRACELOG("load: " << sqlitepath.toStdString()
                       << " younger than " << sqlpath.toStdString());
      throw std::runtime_error("BxoLoader::load .sqlite youger");
    }
  _ld_sqldb->setDatabaseName(sqlitepath);
  if (!_ld_sqldb->open())
    {
      BXO_BACKTRACELOG("load " << sqlitepath.toStdString()
                       << " failed to open: " << _ld_sqldb->lastError().text().toStdString());
      throw std::runtime_error("BxoLoader::load open failure");
    }
  create_objects();
  set_globals ();
  name_objects ();
  name_predefined ();
  // link_modules ();
  // fill_objects_contents ();
  // load_class ();
  // load_payload ();
} // end of BxoLoader::load

void
BxoLoader::create_objects(void)
{
  QSqlQuery query;
  enum { ResixId, Resix_LAST };
  if (!query.exec("SELECT ob_id FROM t_objects"))
    {
      BXO_BACKTRACELOG("create_objects Sql query failure: " <<  _ld_sqldb->lastError().text().toStdString());
      throw std::runtime_error("BxoLoader::create_objects query failure");
    }
  while (query.next())
    {
      std::string idstr = query.value(ResixId).toString().toStdString();
      (void) BxoObject::load_objref(*this,idstr);
    }
} // end BxoLoader::create_objects


void
BxoLoader::set_globals(void)
{
#define BXO_HAS_GLOBAL(Nam,Idstr,Hid,Loid,Hash) do {  \
    if (!BXO_VARGLOBAL(Nam))        \
      BXO_VARGLOBAL(Nam) =        \
  BxoObject::load_objref(*this,#Idstr);   \
    BXO_ASSERT(BXO_VARGLOBAL(Nam)->hash() == Hash,  \
         "bad hash for " << #Nam);    \
} while(0)
#include "_bxo_global.h"
} // end of BxoLoader::set_globals

void
BxoLoader::name_objects(void)
{
  QSqlQuery query;
  enum { ResixId, ResixName, Resix_LAST };
  if (!query.exec("SELECT nam_oid, nam_str FROM t_names"))
    {
      BXO_BACKTRACELOG("name_objects Sql query failure: " <<  _ld_sqldb->lastError().text().toStdString());
      throw std::runtime_error("BxoLoader::name_objects query failure");
    }
  while (query.next())
    {
      std::string idstr = query.value(ResixId).toString().toStdString();
      std::string namstr = query.value(ResixName).toString().toStdString();
      auto pob = find_loadedobj(idstr);
      if (!pob)
        {
          BXO_BACKTRACELOG("name_objects cant find " << idstr);
          throw std::runtime_error("BxoLoader::name_objects missing object");
        }
      if (!pob->register_named(namstr))
        {
          BXO_BACKTRACELOG("name_objects cant register " << namstr
                           << " for " << idstr);
          throw std::runtime_error("BxoLoader::name_objects cant register named");
        }
    }
} // end of BxoLoader::name_objects


void
BxoLoader::name_predefined(void)
{
#define BXO_HAS_PREDEFINED(Nam,Idstr,Hid,Loid,Hash)  do {       \
    auto pob = find_loadedobj(#Idstr);                          \
    if (pob) {                                                  \
      if (!pob->register_named(#Nam))                           \
        fprintf(stderr,                                         \
                "Failed to name predefined %s : %s\n",          \
                #Idstr, #Nam);                                  \
    }                                                           \
    else                                                        \
      fprintf(stderr,                                           \
                 "no predefined object %s to name : %s\n",      \
                 #Idstr, #Nam);                                 \
  } while(0);

#include "_bxo_predef.h"
  fflush(nullptr);
} // end of BxoLoader::name_predefined

void
BxoLoader::register_objref(const std::string&idstr,std::shared_ptr<BxoObject> obp)
{
  Bxo_hid_t hid=0;
  Bxo_loid_t loid=0;
  BXO_ASSERT(BxoObject::str_to_hid_loid(idstr,&hid,&loid),
             "register_objref bad idstr:" << idstr);
  BXO_ASSERT(obp, "register_objref empty obp");
  _ld_idtoobjmap[idstr] = obp;
} // end BxoLoader::register_objref


void
BxoLoader::link_modules(void)
{
  typedef std::pair<std::shared_ptr<BxoObject>,void*> Pobjdlh_t;
  std::vector<Pobjdlh_t> vecmod;
  {
    QSqlQuery query;
    enum { ResixId, Resix_LAST };
    if (!query.exec("SELECT mod_oid FROM t_modules"))
      {
        BXO_BACKTRACELOG("link_modules Sql query failure: " <<  _ld_sqldb->lastError().text().toStdString());
        throw std::runtime_error("BxoLoader::link_modules query failure");
      }
    while (query.next())
      {
        std::string idstr = query.value(ResixId).toString().toStdString();
        auto pob = find_loadedobj(idstr);
        if (!pob)
          {
            BXO_BACKTRACELOG("link_modules cant find " << idstr);
            throw std::runtime_error("BxoLoader::link_modules missing object");
          }
        std::string srcmodpath = std::string {basixmo_directory}+"/" BXO_MODULEPREFIX + idstr + ".cc";
        if (!QFileInfo::exists(QString::fromStdString(srcmodpath)))
          {
            BXO_BACKTRACELOG("link_modules missing module source " << srcmodpath);
            throw std::runtime_error("BxoLoader::link_module missing module source");
          }
        std::string binmodrelpath = std::string {BXO_MODULEDIR "/" BXO_MODULEPREFIX} + idstr + BXO_MODULESUFFIX;
        // should run make -C basixmo_directory -q binmodrelpath
        {
          QProcess makeproc;
          QStringList makeargs;
          makeargs << "-C" << basixmo_directory << "-q" << binmodrelpath.c_str();
          makeproc.start("make",makeargs);
          makeproc.waitForFinished(-1);
          if (makeproc.exitStatus() != QProcess::NormalExit || makeproc.exitCode() != 0)
            {
              BXO_BACKTRACELOG("link_modules module binary  " << binmodrelpath << " in " << basixmo_directory << " is obsolete");
              throw std::runtime_error("BxoLoader::link_module obsolete binary module");
            }
        }
        vecmod.push_back(Pobjdlh_t {pob,nullptr});
      }
  }
  for (Pobjdlh_t po : vecmod)
    {
      std::string idstr = po.first->strid();
      std::string binmodpath =
        std::string {basixmo_directory} + "/"
        + std::string {BXO_MODULEDIR "/" BXO_MODULEPREFIX} + idstr + BXO_MODULESUFFIX;
      void* dlh = dlopen(binmodpath.c_str(), RTLD_LAZY | RTLD_GLOBAL);
      if (!dlh)
        {
          BXO_BACKTRACELOG("link_modules dlopen " << binmodpath << " failed:" << dlerror());
          throw std::runtime_error("BxoLoader::link_module dlopen failure");
        }
    }
} // end of BxoLoader::link_modules


void
BxoLoader::fill_objects_contents(void)
{
} // end of BxoLoader::fill_objects_contents

void
BxoLoader::load_class(void)
{
} // end of BxoLoader::load_class

void
BxoLoader::load_payload(void)
{
} // end of BxoLoader::load_payload

bool
BxoDumper::scan_dumpable(BxoObject*pob)
{
  BXO_ASSERT(_du_state == DuScan, "non-scan state #" << (int)_du_state);
  if (!pob) return false;
  if (is_dumpable(pob)) return true;
  if (pob->space() == BxoSpace::TransientSp) return false;
  _du_objset.insert(pob);
  _du_scanque.push_back(pob->shared_from_this());
  return true;
} // end BxoDumper::scan_dumpable

void
BxoDumper::scan_all(void)
{
  BXO_ASSERT(_du_state == DuStop, "non stop state for scan");
  _du_objset.clear();
  _du_state = DuScan;
  BxoVal proset = BxoObject::set_of_predefined_objects();
  proset.scan_dump(*this);
  while (!_du_scanque.empty())
    {
      auto scf = _du_scanque.front();
      BXO_ASSERT(scf, "empty object to scan");
      _du_scanque.pop_front();
      scf->scan_content_dump(*this);
    }
} // end of BxoDumper::scan_all

void
BxoDumper::emit_all()
{
  _du_state = DuEmit;
  for (BxoObject*pob : _du_objset)
    {
#warning BxoDumper::emit_all incomplete
    }
} // end BxoDumper::emit_all

void
BxoObject::scan_content_dump(BxoDumper&du) const
{
  if (_classob)
    du.scan_dumpable(_classob.get());
  for (auto &p : _attrh)
    {
      if (du.scan_dumpable(p.first.get()))
        {
          p.second.scan_dump(du);
        }
    }
  for (auto& p: _compv)
    {
      p.scan_dump(du);
    }
  if (_payl)
    {
      BXO_ASSERT(_payl->owner() == this, "bad payload owner");
      auto kob = _payl->kind();
      if (kob && du.scan_dumpable(kob.get()))
        {
          _payl->scan_payload_content(du);
        }
    }
} // end BxoObject::scan_content_dump
