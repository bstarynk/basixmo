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
BxoLoader::register_objref(const std::string&idstr,std::shared_ptr<BxoObject> obp)
{
  Bxo_hid_t hid=0;
  Bxo_loid_t loid=0;
  BXO_ASSERT(BxoObject::str_to_hid_loid(idstr,&hid,&loid),
             "register_objref bad idstr:" << idstr);
  BXO_ASSERT(obp, "register_objref empty obp");
  _ld_idtoobjmap[idstr] = obp;
} // end BxoLoader::register_objref
