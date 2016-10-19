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
#include <QFileInfo>

BxoLoader::BxoLoader(const std::string dirnam)
  : _ld_dirname(dirnam), _ld_sqldb(nullptr)
{
} // end of BxoLoader::BxoLoader


BxoObject*
BxoLoader::obj_from_idstr(const std::string&s)
{
  auto p = _ld_objmap.find(s);
  if (p != _ld_objmap.end())
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

void BxoLoader::load()
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
} // end of BxoLoader::load
