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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <QtSql>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QProcess>
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
  _ld_sqldb = nullptr;
}

const BxoSet*
BxoSet::load_set(BxoJsonProcessor&bxj, const BxoJson&js)
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
      auto pob = bxj.obj_from_idstr(jcomp.asString());
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
BxoTuple::load_tuple(BxoJsonProcessor&bxj, const BxoJson&js)
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
      auto pob = bxj.obj_from_idstr(jcomp.asString());
      if (!pob)
        {
          BXO_BACKTRACELOG("load_set: bad jcomp="
                           << jcomp << " at ix=" << ix);
          throw std::runtime_error("BxoTuple::load_tuple bad jcomp");
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
  _ld_sqldb = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE","bxoloader"));
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
  bind_predefined();
  create_objects();
  set_globals ();
  name_objects ();
  name_predefined ();
  link_modules ();
  fill_objects_contents ();
  load_objects_class ();
  load_objects_create_payload ();
  load_objects_fill_payload ();
  _ld_sqldb->close();
  int nbobj = _ld_idtoobjmap.size();
  _ld_idtoobjmap.clear();
  delete _ld_sqldb;
  _ld_sqldb = nullptr;
  QSqlDatabase::removeDatabase("bxoloader");
  double elaptim = bxo_elapsed_real_time() - _ld_startelapsedtime;
  double cputim = bxo_process_cpu_time () - _ld_startprocesstime;
  printf("\n"
         "Loaded %d objects in %.3f elapsed, %.4f cpu seconds (%.3f elapsed, %.3f cpu µs/obj)\n",
         nbobj, elaptim, cputim, 1.0e6*(elaptim/nbobj), 1.0e6*(cputim/nbobj));
  fflush(nullptr);
} // end of BxoLoader::load

void
BxoLoader::bind_predefined(void)
{
#define BXO_HAS_PREDEFINED(Name,Idstr,Hid,Loid,Hash)            \
    _ld_idtoobjmap.insert({BXO_VARPREDEF(Name)->strid(),        \
                           BXO_VARPREDEF(Name)});
#include "_bxo_predef.h"
} // end BxoLoader::bind_predefined

void
BxoLoader::create_objects(void)
{
  QSqlQuery query(*_ld_sqldb);
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
#define BXO_HAS_GLOBAL(Nam,Idstr,Hid,Loid,Hash) do {    \
    if (!BXO_VARGLOBAL(Nam))                            \
      BXO_VARGLOBAL(Nam) =                              \
        BxoObject::load_objref(*this,#Idstr);           \
    BXO_ASSERT(BXO_VARGLOBAL(Nam)->hash() == Hash,      \
         "bad hash for " << #Nam);                      \
  } while(0);
#include "_bxo_global.h"
} // end of BxoLoader::set_globals

void
BxoLoader::name_objects(void)
{
  QSqlQuery query(*_ld_sqldb);
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

std::shared_ptr<BxoObject>
BxoLoader::name_the_predefined(const std::string&nam, const std::string&idstr)
{
  auto pob = find_loadedobj(idstr);
  if (!pob)
    {
      BXO_BACKTRACELOG("name_the_predefined dont find idstr=" << idstr);
      throw  std::runtime_error("BxoLoader::name_the_predefined missing");
    }
  if (pob->name() == nam) // already named
    return pob;
  if (!pob->register_named(nam))
    {
      BXO_BACKTRACELOG("name_the_predefined for idstr=" << idstr
                       << " failed to name " << nam);
      throw  std::runtime_error("BxoLoader::name_the_predefined failed naming");
    }
  return pob;
}

void
BxoLoader::name_predefined(void)
{
#define BXO_HAS_PREDEFINED(Nam,Idstr,Hid,Loid,Hash)  do {       \
    auto pob = name_the_predefined(#Nam,#Idstr);                \
    BXO_ASSERT (pob, "no pob for idstr=" << #Idstr);            \
    BXO_ASSERT (pob->hash() == Hash, "bad hash");               \
    BXO_ASSERT (pob->strid() == #Idstr, "bad idstr");           \
    BXO_ASSERT (pob->hid() == Hid, "bad hid");                  \
    BXO_ASSERT (pob->loid() == Loid, "bad hid");                \
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
    QSqlQuery query(*_ld_sqldb);
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
      po.second = dlh;
    }
} // end of BxoLoader::link_modules


void
BxoLoader::fill_objects_contents(void)
{
  QSqlQuery query(*_ld_sqldb);
  enum { ResixId, ResixMtime, ResixJsoncont, Resix_LAST };
  if (!query.exec("SELECT ob_id, ob_mtime, ob_jsoncont FROM t_objects"))
    {
      BXO_BACKTRACELOG("fill_objects_contents Sql query failure: " <<  _ld_sqldb->lastError().text().toStdString());
      throw std::runtime_error("BxoLoader::fill_objects_contents query failure");
    }
  while (query.next())
    {
      std::string idstr = query.value(ResixId).toString().toStdString();
      double mtimdb = query.value(ResixMtime).toDouble();
      auto pob = find_loadedobj(idstr);
      if (!pob)
        {
          BXO_BACKTRACELOG("fill_objects_contents cant find " << idstr);
          throw std::runtime_error("BxoLoader::fill_objects_contents missing object");
        }
      std::string jsonstr = query.value(ResixJsoncont).toString().toStdString();
      Json::Reader jrd(Json::Features::strictMode());
      BxoJson jv;
      if (!jrd.parse(jsonstr,jv,false))
        {
          BXO_BACKTRACELOG("fill_objects_contents parse failure for " << idstr
                           << ": " << jrd.getFormattedErrorMessages()
                           << std::endl << "jsonstr=" << jsonstr
                           << std::endl);
          throw std::runtime_error("BxoLoader::fill_objects_contents Json parse failure");
        }
      pob->touch_load((time_t)mtimdb,*this);
      pob->load_content(jv,*this);
    }
} // end of BxoLoader::fill_objects_contents



void
BxoLoader::load_objects_class(void)
{
  QSqlQuery query(*_ld_sqldb);
  constexpr const char*sqlsel = "SELECT ob_id, ob_classid FROM t_objects WHERE ob_classid IS NOT \"\" ";
  enum { ResixId, ResixClassid, Resix_LAST };
  if (!query.exec(sqlsel))
    {
      BXO_BACKTRACELOG("load_objects_class Sql query failure: " <<  _ld_sqldb->lastError().text().toStdString());
      throw std::runtime_error("BxoLoader::load_objects_class query failure");
    }
  while (query.next())
    {
      std::string idstr = query.value(ResixId).toString().toStdString();
      std::string classidstr = query.value(ResixClassid).toString().toStdString();
      auto pob = find_loadedobj(idstr);
      if (!pob)
        {
          BXO_BACKTRACELOG("load_objects_class cant find object " << idstr);
          throw std::runtime_error("BxoLoader::load_objects_class missing object");
        }
      auto pclassob = find_loadedobj(classidstr);
      if (!pclassob)
        {
          BXO_BACKTRACELOG("load_objects_class cant find class " << classidstr << " for object " << idstr);
          throw std::runtime_error("BxoLoader::load_objects_class missing class");
        }
      pob->load_set_class(pclassob,*this);
    }
} // end of BxoLoader::load_objects_class



void
BxoLoader::load_objects_create_payload(void)
{
  QSqlQuery query(*_ld_sqldb);
  constexpr const char*sqlsel = "SELECT ob_id, ob_paylkid FROM t_objects WHERE ob_paylkid IS NOT \"\" ";
  enum { ResixId, ResixPaylkId, Resix_LAST };
  if (!query.exec(sqlsel))
    {
      BXO_BACKTRACELOG("load_objects_create_payload Sql query failure: " <<  _ld_sqldb->lastError().text().toStdString());
      throw std::runtime_error("BxoLoader::load_objects_create_payload query failure");
    }
  while (query.next())
    {
      std::string idstr = query.value(ResixId).toString().toStdString();
      std::string pykidstr = query.value(ResixPaylkId).toString().toStdString();
      auto pob = find_loadedobj(idstr);
      if (!pob)
        {
          BXO_BACKTRACELOG("load_objects_create_payload cant find object " << idstr);
          throw std::runtime_error("BxoLoader::load_objects_create_payload missing object");
        }
      auto pykindob = find_loadedobj(pykidstr);
      if (!pykindob)
        {
          BXO_BACKTRACELOG("load_objects_create_payload cant find payload kind " << pykidstr << " for object " << idstr);
          throw std::runtime_error("BxoLoader::load_objects_create_payload missing payload kind");
        }
      std::string loadername = std::string {BxoPayload::loader_prefix} + pykidstr;
      void* ldfunad = dlsym(bxo_dlh, loadername.c_str());
      if (ldfunad == nullptr)
        {
          BXO_BACKTRACELOG("load_objects_create_payload for object " << pob << " of payload kind " << pykindob
                           << " failed to dlsym " << loadername << " : " << dlerror());
          throw std::runtime_error("BxoLoader::load_objects_create_payload missing loader fun");
        }
      auto ldfun = reinterpret_cast<BxoPayload::loader_create_sigt*>(ldfunad);
      BxoPayload* payl= (*ldfun)(pob.get(),this);
      pob->load_set_payload(payl,*this);
    }
} // end of BxoLoader::load_objects_create_payload



void
BxoLoader::load_objects_fill_payload(void)
{
  QSqlQuery query(*_ld_sqldb);
  constexpr const char*sqlsel = "SELECT ob_id, ob_paylcont FROM t_objects WHERE ob_paylcont IS NOT \"\" ";
  enum { ResixId, ResixPaylcont, Resix_LAST };
  if (!query.exec(sqlsel))
    {
      BXO_BACKTRACELOG("load_objects_fill_payload Sql query failure: " <<  _ld_sqldb->lastError().text().toStdString());
      throw std::runtime_error("BxoLoader::load_objects_fill_payload query failure");
    }
  while (query.next())
    {
      std::string idstr = query.value(ResixId).toString().toStdString();
      auto pob = find_loadedobj(idstr);
      if (!pob)
        {
          BXO_BACKTRACELOG("load_objects_fill_payload cant find object " << idstr);
          throw std::runtime_error("BxoLoader::load_objects_fill_payload missing object");
        }
      if (!pob->payload())
        {
          BXO_BACKTRACELOG("load_objects_fill_payload object " << pob << " without payload");;
          throw std::runtime_error("BxoLoader::load_objects_fill_payload object without payload");
        }
      std::string jsonstr = query.value(ResixPaylcont).toString().toStdString();
      Json::Reader jrd(Json::Features::strictMode());
      BxoJson jv;
      if (!jrd.parse(jsonstr,jv,false))
        {
          BXO_BACKTRACELOG("load_objects_fill_payload Json parse failure for " << idstr
                           << ": " << jrd.getFormattedErrorMessages()
                           << std::endl << "jsonstr=" << jsonstr
                           << std::endl);
          throw std::runtime_error("BxoLoader::load_objects_fill_payload Json parse failure");
        }
      pob->payload()->load_payload_content(jv,*this);
    }
} // end of BxoLoader::load_objects_fill_payload




std::string BxoDumper::_defaultdumpdir_;

std::string
BxoDumper::generate_temporary_suffix(void)
{
  char sbuf[80];
  memset(sbuf, 0, sizeof(sbuf));
  snprintf(sbuf, sizeof(sbuf), "+r%x-%x_p%d.tmp~",
           (unsigned)BxoRandom::random_nonzero_32u(),
           (unsigned)BxoRandom::random_nonzero_32u(),
           (int)getpid());
  return std::string {sbuf};
}//end BxoDumper::generate_temporary_suffix


BxoDumper::BxoDumper(const std::string&dirn)
  : _du_queryinsobj(nullptr),
    _du_sqldb(nullptr),
    _du_state(DuStop),
    _du_startelapsedtime(bxo_elapsed_real_time ()),
    _du_startprocesstime(bxo_process_cpu_time ()),
    _du_dirname(dirn),
    _du_tempsuffix(generate_temporary_suffix()),
    _du_objset(),
    _du_scanque()
{
} // end of BxoDumper::BxoDumper

BxoDumper::~BxoDumper()
{
  delete _du_queryinsobj;
  _du_queryinsobj = nullptr;
  delete _du_sqldb;
  _du_sqldb = nullptr;
  _du_state = DuStop;
  _du_objset.clear();
  _du_scanque.clear();
} // end of BxoDumper::~BxoDumper


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
  BXO_BACKTRACELOG("scan_all proset=" << proset);
  proset.scan_dump(*this);
  int nbscan = 0;
  while (!_du_scanque.empty())
    {
      auto scf = _du_scanque.front();
      nbscan++;
      BXO_ASSERT(scf, "empty object to scan nbscan=" << nbscan);
      BXO_VERBOSELOG("nbscan#" << nbscan << " scf=" << scf << ":" << scf->strid());
      _du_scanque.pop_front();
      scf->scan_content_dump(*this);
    }
  while (!_du_todoafterscan.empty())
    {
      auto tdf = _du_todoafterscan.front();
      _du_todoafterscan.pop_front();
      tdf.first(*this,tdf.second);
    }
  BXO_ASSERT(nbscan>0, "BxoDumper::scan_all bad nbscan:" << nbscan);
} // end of BxoDumper::scan_all




void
BxoDumper::initialize_data_schema()
{
  BXO_ASSERT(_du_sqldb != nullptr, "no _du_sqldb");
  QSqlQuery query(*_du_sqldb);
  if (!query.exec("CREATE TABLE IF NOT EXISTS t_params "
                  "(par_name VARCHAR(35) PRIMARY KEY ASC NOT NULL UNIQUE,"
                  "  par_value TEXT NOT NULL)"))
    {
      BXO_BACKTRACELOG("initialize_data_schema Sql query failure t_params: " <<  _du_sqldb->lastError().text().toStdString());
      throw std::runtime_error("BxoLoader::initialize_data_schema query failure t_params");
    }
  if (!query.exec("CREATE TABLE IF NOT EXISTS t_objects "
                  "(ob_id VARCHAR(20) PRIMARY KEY ASC NOT NULL UNIQUE,"
                  " ob_mtime DATETIME,"
                  " ob_jsoncont TEXT NOT NULL,"
                  " ob_classid VARCHAR(20) NOT NULL,"
                  " ob_paylkid VARCHAR(20) NOT NULL,"
                  " ob_paylcont TEXT NOT NULL,"
                  " ob_paylmod VARCHAR(20) NOT NULL)"))
    {
      BXO_BACKTRACELOG("initialize_data_schema Sql query failure t_objects: " <<  _du_sqldb->lastError().text().toStdString());
      throw std::runtime_error("BxoLoader::initialize_data_schema query failure t_objects");
    }
  if (!query.exec("CREATE TABLE IF NOT EXISTS t_names "
                  "(nam_str PRIMARY KEY ASC NOT NULL UNIQUE,"
                  " nam_oid  VARCHAR(20) NOT NULL UNIQUE)"))
    {
      BXO_BACKTRACELOG("initialize_data_schema Sql query failure t_names: " <<  _du_sqldb->lastError().text().toStdString());
      throw std::runtime_error("BxoLoader::initialize_data_schema query failure t_names");
    }
  if (!query.exec("CREATE TABLE IF NOT EXISTS t_modules "
                  "(mod_oid VARCHAR(20) PRIMARY KEY ASC NOT NULL UNIQUE)"))
    {
      BXO_BACKTRACELOG("initialize_data_schema Sql query failure t_modules: " <<  _du_sqldb->lastError().text().toStdString());
      throw std::runtime_error("BxoLoader::initialize_data_schema query failure t_modules");
    }
  if (!query.exec("CREATE UNIQUE INDEX IF NOT EXISTS "
                  " x_namedid ON t_names (nam_oid)"))
    {
      BXO_BACKTRACELOG("initialize_data_schema Sql query failure x_namedid: " <<  _du_sqldb->lastError().text().toStdString());
      throw std::runtime_error("BxoLoader::initialize_data_schema query failure x_namedid");
    }
} // end BxoDumper::initialize_data_schema



void
BxoDumper::emit_all()
{
  _du_state = DuEmit;
  BXO_ASSERT(_du_sqldb != nullptr, "no dump sqldb");
  std::map<std::string, BxoObject*> mapname;
  std::set<std::shared_ptr<BxoObject>,BxoLessObjSharedPtr> moduset;
  _du_queryinsobj = new QSqlQuery(*_du_sqldb);
  _du_queryinsobj->prepare(insert_object_sql);
  // emit all dumpable objects
  BXO_ASSERT(!_du_objset.empty(), "empty _du_objset");
  for (BxoObject*pob : _du_objset)
    {
      BXO_ASSERT(pob != nullptr, "null pob");
      BXO_VERBOSELOG("BxoDumper::emit_all pob:" << pob << " of id " << pob->strid());
      auto modob = emit_object_row_module(pob);
      auto obnam = pob->name();
      if (!obnam.empty())
        mapname.insert({obnam,pob});
      if (modob)
        moduset.insert(modob);
    }
  delete _du_queryinsobj;
  _du_queryinsobj = nullptr;
  // emit the names
  {
    QSqlQuery insnamquery(*_du_sqldb);
    insnamquery.prepare("INSERT INTO t_names (nam_str, nam_oid) VALUES(?, ?)");
    enum { InsnamStrIx, InsnamIdIx, Insnam_Last };
    for (auto p: mapname)
      {
        insnamquery.bindValue((int)InsnamStrIx, p.first.c_str());
        insnamquery.bindValue((int)InsnamIdIx, p.second->strid().c_str());
        if (!insnamquery.exec())
          {
            BXO_BACKTRACELOG("emit_all: SQL failure for name insertion name="
                             << p.first << " id=" << p.second->strid()
                             << " : " << _du_sqldb->lastError().text().toStdString());
            throw std::runtime_error("BxoDumper::emit_all SQL failure for name insertion");
          }
      }
  }
  // emit the modules
  {
    QSqlQuery insmodquery(*_du_sqldb);
    insmodquery.prepare("INSERT INTO t_modules (mod_oid) VALUES(?)");
    enum { InsmodIdIx, Insmod_Last };
    for (auto modob : moduset)
      {
        BXO_ASSERT(modob != nullptr, "null modob");
        insmodquery.bindValue((int)InsmodIdIx, modob->strid().c_str());
        if (!insmodquery.exec())
          {
            BXO_BACKTRACELOG("emit_all: SQL failure for module insertion id=" << modob->strid()
                             <<  " : " << _du_sqldb->lastError().text().toStdString());
            throw std::runtime_error("BxoDumper::emit_all SQL failure for module insertion");
          }
      }
  }
} // end BxoDumper::emit_all


std::string
BxoDumper::output_path(const std::string& filpath)
{
  if (filpath.empty() || filpath[0] == '/'
      || filpath.find("..") != std::string::npos
      || !(isalnum(filpath[0])|| filpath[0]=='_')
      || filpath[filpath.size()-1]=='/')
    {
      BXO_BACKTRACELOG("output_path: invalid filpath " << filpath);
      throw std::runtime_error("BxoDumper::output_path invalid filpath");
    }
  if (_du_outfilset.find(filpath) != _du_outfilset.end())
    {
      BXO_BACKTRACELOG("output_path: duplicate filpath " << filpath);
      throw std::runtime_error("BxoDumper::output_path duplicate filpath");
    }
  _du_outfilset.insert(filpath);
  return _du_dirname + "/" + filpath + _du_tempsuffix;
} // end of BxoDumper::output_path




void
BxoDumper::full_dump(void)
{
  if (::access(_du_dirname.c_str(), F_OK))
    {
      if (mkdir(_du_dirname.c_str(), 0750))
        {
          BXO_BACKTRACELOG("full_dump mkdir " << _du_dirname
                           << " failed: " << strerror(errno));
          throw std::runtime_error("BxoDumper::full_dump mkdir failed");
        }
    }
  {
    auto timestampath = output_path(std::string("_BxoDumpTimeStamp"));
    FILE* filtims = fopen(timestampath.c_str(), "w");
    if (!filtims)
      {
        BXO_BACKTRACELOG("full_dump failed to fopen timestamp " << timestampath
                         << " : " << strerror(errno));
        throw std::runtime_error("BxoDumper::full_dump timestamp fopen failed");
      }
    time_t nowt = time(nullptr);
    struct tm nowtm = {};
    localtime_r(&nowt, &nowtm);
    char nowtimbuf[72];
    memset (nowtimbuf, 0, sizeof(nowtimbuf));
    strftime(nowtimbuf, sizeof(nowtimbuf), "%c", &nowtm);
    fprintf(filtims, "Bxo-dump: %s\n", nowtimbuf);
    fprintf(filtims, "Bxo-timestamp: %s\n", basixmo_timestamp);
    fflush(filtims);
    fprintf(filtims, "Bxo-dir: %s\n", basixmo_directory);
    fprintf(filtims, "Bxo-lastgitcommit: %s\n", basixmo_lastgitcommit);
    if (fclose(filtims))
      {
        BXO_BACKTRACELOG("full_dump failed to fclose timestamp " << timestampath
                         << " : " << strerror(errno));
        throw std::runtime_error("BxoDumper::full_dump timestamp fclose failed");
      };
    filtims = nullptr;
  }
  BXO_ASSERT(_du_sqldb == nullptr, "got an sqldb");
  auto sqlitepath = output_path(std::string(basixmo_statebase)+".sqlite");
  _du_sqldb = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", "bxodumper"));
  _du_sqldb->setDatabaseName(QString(sqlitepath.c_str()));
  if (!_du_sqldb->open())
    {
      BXO_BACKTRACELOG("full_dump " << sqlitepath
                       << " failed to open: " << _du_sqldb->lastError().text().toStdString());
      throw std::runtime_error("BxoDumper::full_dump open failutr");
    }
  scan_all();
  BXO_ASSERT(!_du_objset.empty(), "empty _du_objset");
  initialize_data_schema();
  emit_all();
  long nbobj = _du_objset.size();
  int nbfil = 0;
  _du_objset.clear();
  delete _du_queryinsobj;
  _du_queryinsobj = nullptr;
  _du_sqldb->close();
  while (!_du_outfilset.empty())
    {
      std::string outpath;
      {
        outpath = *_du_outfilset.begin();
      }
      _du_outfilset.erase(outpath);
      nbfil++;
      rename_temporary(outpath);
    }
  delete _du_sqldb;
  _du_sqldb = nullptr;
  {
    QProcess dumpproc;
    QStringList dumpargs;
    dumpargs << (_du_dirname+"/"+basixmo_statebase+".sqlite").c_str() << (_du_dirname+"/"+basixmo_statebase+".sql").c_str();
    dumpproc.start((std::string {basixmo_directory} + "/" + BXO_DUMP_SCRIPT).c_str(), dumpargs);
    dumpproc.waitForFinished(-1);
    if (dumpproc.exitStatus() != QProcess::NormalExit || dumpproc.exitCode() != 0)
      {
        BXO_BACKTRACELOG("dump script " << BXO_DUMP_SCRIPT << " failed in " << _du_dirname);
        throw std::runtime_error("BxoDumper::full_dump dump script failed");
      }
  }
  double elaptim = bxo_elapsed_real_time() - _du_startelapsedtime;
  double cputim = bxo_process_cpu_time () - _du_startprocesstime;
  printf("\n"
         "Dumped %ld objects & %d files into %s/ in %.3f elapsed, %.4f cpu seconds (%.3f elapsed, %.3f cpu µs/obj)\n",
         nbobj, nbfil, _du_dirname.c_str(),
         elaptim, cputim, 1.0e6*(elaptim/nbobj), 1.0e6*(cputim/nbobj));
  fflush(nullptr);
} // end BxoDumper::full_dump

bool
BxoDumper::same_file_content(const char*path1, const char*path2)
{
  BXO_ASSERT(path1 != nullptr, "null path1");
  BXO_ASSERT(path2 != nullptr, "null path2");
  bool samefilecont = false;
  struct ::stat stat1;
  struct ::stat stat2;
  memset (&stat1, 0, sizeof(stat1));
  memset (&stat2, 0, sizeof(stat2));
  if (!stat(path1, &stat1) && !stat(path2, &stat2)
      && stat1.st_size == stat2.st_size)
    {
      FILE*f1 = fopen(path1, "r");
      if (!f1) return false;
      FILE*f2 = fopen(path2, "r");
      if (!f2)
        {
          fclose(f1);
          return false;
        }
      samefilecont = true;
      while (samefilecont)
        {
          int c1 = fgetc(f1);
          int c2 = fgetc(f2);
          if (c1 != c2) samefilecont = false;
          else if (c1 == EOF) break;
        };
      if (f1) fclose(f1);
      if (f2) fclose(f2);
    }
  return samefilecont;
} // end BxoDumper::same_file_content


void
BxoDumper::rename_temporary(const std::string&filpath)
{
  std::string fullpath = _du_dirname + "/" + filpath;
  std::string tempath = _du_dirname + "/" +filpath+_du_tempsuffix;
  std::string backupath = _du_dirname + "/" +filpath+"~";
  if (same_file_content(tempath.c_str(), fullpath.c_str()))
    {
      if (::remove(tempath.c_str()))
        {
          BXO_BACKTRACELOG("rename_temporary: failed to remove " << tempath
                           << " of same content as " << fullpath
                           << ":" << strerror(errno));
          throw std::runtime_error("BxoDumper::rename_temporary failed to remove temporary");
        };
    }
  else
    {
      (void) ::rename(fullpath.c_str(), backupath.c_str());
      if (::rename(tempath.c_str(), fullpath.c_str()))
        {
          BXO_BACKTRACELOG("rename_temporary: failed to rename "
                           << tempath << " -> " << fullpath
                           << " : " << strerror(errno));
          throw std::runtime_error("BxoDumper::rename_temporary failed to rename");
        }
    }
} // end BxoDumper::rename_temporary



std::shared_ptr<BxoObject>
BxoDumper::emit_object_row_module(BxoObject*pob)
{
  std::shared_ptr<BxoObject> modob;
  BXO_ASSERT(pob != nullptr && is_dumpable(pob), "non dumpable object");
  BXO_ASSERT(_du_queryinsobj != nullptr, "missing queryinsobj");
  _du_queryinsobj->bindValue((int)InsobIdIx, pob->strid().c_str());
  _du_queryinsobj->bindValue((int)InsobMtimIx, (qlonglong) pob->mtime());
  {
    const BxoJson& jcont= pob->json_for_content(*this);
    Json::StyledWriter jwr;
    _du_queryinsobj->bindValue((int)InsobJsoncontIx, jwr.write(jcont).c_str());
  }
  auto pcla = pob->class_obj();
  if (pcla && is_dumpable(pcla))
    _du_queryinsobj->bindValue((int)InsobClassidIx, pcla->strid().c_str());
  else
    _du_queryinsobj->bindValue((int)InsobClassidIx, "");
  auto payl = pob->payload();
  bool pydumpable = false;
  if (payl)
    {
      auto pykindob = payl->kind_ob();
      if (pykindob && is_dumpable(pykindob))
        {
          std::string loadername = std::string {BxoPayload::loader_prefix} + pykindob->strid();
          void* ldfun = dlsym(bxo_dlh, loadername.c_str());
          if (ldfun != nullptr)
            {
              pydumpable = true;
              _du_queryinsobj->bindValue((int)InsobPaylkindIx, pykindob->strid().c_str());
            }
          else // unlikely, is probably symptom of something wrong
            {
              BXO_BACKTRACELOG("emit_object_row: cannot dlsym " << loadername << " : " << dlerror()
                               << " for payload of " << pob << " of kind " << pykindob);
              // we dont throw any runtime exception
              pydumpable = false;
            }
        };
    };
  if (pydumpable)
    {
      const BxoJson&jpy = payl->emit_payload_content(*this);
      Json::StyledWriter jwr;
      _du_queryinsobj->bindValue((int)InsobPaylcontIx, jwr.write(jpy).c_str());
      modob = payl->module_ob();
      if (modob && is_dumpable(modob))
        _du_queryinsobj->bindValue((int)InsobPaylmodIx, modob->strid().c_str());
      else
        _du_queryinsobj->bindValue((int)InsobPaylmodIx, "");
    }
  else
    {
      _du_queryinsobj->bindValue((int)InsobPaylcontIx, "");
      _du_queryinsobj->bindValue((int)InsobPaylmodIx, "");
      _du_queryinsobj->bindValue((int)InsobPaylkindIx, "");
    }
  if (!_du_queryinsobj->exec())
    {
      BXO_BACKTRACELOG("emit_object_row: SQL failure for " <<  pob->strid()
                       << " :" <<  _du_sqldb->lastError().text().toStdString());
      throw std::runtime_error("BxoDumper::emit_object_row SQL failure");
    }
  return modob;
} // end of BxoDumper::emit_object_row_module

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
      auto kob = _payl->kind_ob();
      if (kob && du.scan_dumpable(kob.get()))
        {
          _payl->scan_payload_content(du);
        }
    }
} // end BxoObject::scan_content_dump

BxoJson
BxoObject::json_for_content(BxoDumper&du) const
{
  BxoJson job {Json::objectValue};
  auto nm = name();
  if (!nm.empty())
    job["@name"] = nm;
  {
    std::set<std::shared_ptr<BxoObject>,BxoLessObjSharedPtr> atset;
    BxoJson jattrs {Json::arrayValue};
    for (const auto& p: _attrh)
      {
        auto atob = p.first;
        if (!du.is_dumpable(atob)) continue;
        atset.insert(atob);
      }
    for (const auto pob: atset)
      {
        const std::shared_ptr<BxoObject> cpob = pob;
        auto pv = _attrh.find(cpob);
        if (pv == _attrh.end()) continue;
        const BxoVal& aval = pv->second;
        BxoJson jpair {Json::objectValue};
        jpair["at"] = pob->strid();
        jpair["va"] = aval.to_json(du);
        jattrs.append (jpair);
      }
    job["attrs"] = jattrs;
  }
  {
    BxoJson jcomps {Json::arrayValue};
    for (auto vcomp : _compv)
      {
        jcomps.append(vcomp.to_json(du));
      }
    job["comps"] = jcomps;
  }
  return job;
} //end BxoObject::json_for_content


void
BxoObject::load_content(const BxoJson&jv, BxoLoader&ld)
{
  if (jv.isMember("attrs"))
    {
      auto jattrs = jv["attrs"];
      if (jattrs.isArray())
        {
          auto nbat = jattrs.size();
          _attrh.reserve(5*nbat/4+1);
          for (int ix=0; ix<(int)nbat; ix++)
            {
              const BxoJson& jpair = jattrs[ix];
              if (!jpair.isObject()) continue;
              const BxoJson& jat = jpair["at"];
              if (!jat.isString()) continue;
              auto pobat = ld.find_loadedobj(jat.asString());
              if (!pobat) continue;
              const BxoJson& jva = jpair["va"];
              BxoVal aval = BxoVal::from_json(ld,jva);
              _attrh.insert({pobat,aval});
            }
        }
    }
  if (jv.isMember("comps"))
    {
      auto jcomps = jv["comps"];
      if (jcomps.isArray())
        {
          auto nbcomp = jcomps.size();
          _compv.reserve(5*nbcomp/4+1);
          for (int ix=0; ix<(int)nbcomp; ix++)
            {
              const BxoJson& jcomp = jcomps[ix];
              BxoVal cval = BxoVal::from_json(ld,jcomp);
              _compv.push_back(cval);
            }
        }
    }
} // end BxoObject::load_content

void
BxoObject::touch_load(time_t mtim, BxoLoader&)
{
  _mtime = (time_t) mtim;
} // end BxoObject::touch_load


void
BxoObject::load_set_class(std::shared_ptr<BxoObject> obclass, BxoLoader&)
{
  _classob = obclass;
} // end of BxoObject::load_set_class

void
BxoObject::load_set_payload(BxoPayload*payl, BxoLoader&)
{
  BXO_ASSERT(payl && payl->owner() == this, "bad payl for " << this);
  _payl.reset(payl);
} // end of BxoObject::load_set_payload
