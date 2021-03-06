// file main.cc - the main program and basic utilities

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
#include <cxxabi.h>
#include <QProcess>
#include <QCoreApplication>
#include <QApplication>
#include <QCommandLineParser>
#include "sqlite3.h"

thread_local BxoRandom BxoRandom::_rand_thr_;
bool bxo_verboseflag;
void* bxo_dlh;


void bxo_abort(void)
{
  fflush(NULL);
  abort();
} // end of bxo_abort

char *
bxo_strftime_centi (char *buf, size_t len, const char *fmt, double ti)
{
  struct tm tm;
  time_t tim = (time_t) ti;
  memset (&tm, 0, sizeof (tm));
  if (!buf || !fmt || !len)
    return NULL;
  strftime (buf, len, fmt, localtime_r (&tim, &tm));
  char *dotundund = strstr (buf, ".__");
  if (dotundund)
    {
      double ind = 0.0;
      double fra = modf (ti, &ind);
      char minibuf[16];
      memset (minibuf, 0, sizeof (minibuf));
      snprintf (minibuf, sizeof (minibuf), "%.02f", fra);
      strncpy (dotundund, strchr (minibuf, '.'), 3);
    }
  return buf;
} // end bxo_strftime_centi

std::string
bxo_demangled_typename(const std::type_info &ti)
{
  int dstat = -1;
  char*dnam = abi::__cxa_demangle(ti.name(), 0, 0, &dstat);
  if (dstat == 0 && dnam != nullptr)
    {
      std::string ns {dnam};
      free (dnam);
      return ns;
    }
  if (dnam) free(dnam), dnam=nullptr;
  return "??";
} // end bxo_demangled_typename

/************************* backtrace *************************/

/* A callback function passed to the backtrace_full function.  */

#define BXO_MAX_CALLBACK_DEPTH 64
static int
bxo_bt_callback (void *data, uintptr_t pc, const char *filename, int lineno,
                 const char *function)
{
  int *pcount = (int *) data;

  /* If we don't have any useful information, don't print
     anything.  */
  if (filename == NULL && function == NULL)
    return 0;

  /* Print up to BXO_MAX_CALLBACK_DEPTH functions.    */
  if (*pcount >= BXO_MAX_CALLBACK_DEPTH)
    {
      /* Returning a non-zero value stops the backtrace.  */
      fprintf (stderr, "...etc...\n");
      return 1;
    }
  ++*pcount;

  int demstatus = -1;
  char* demfun = abi::__cxa_demangle(function, nullptr, nullptr, &demstatus);
  if (demstatus != 0)
    {
      if (demfun)
        free(demfun);
      demfun = nullptr;
    };
  fprintf (stderr, "Basixmo[0x%lx] %s\n\t%s:%d\n",
           (unsigned long) pc,
           demfun?demfun:(function == NULL ? "???" : function),
           filename == NULL ? "???" : filename, lineno);
  if (demfun)
    {
      free(demfun);
      demfun = nullptr;
    }
  return 0;
}                               /* end bxo_bt_callback */

/* An error callback function passed to the backtrace_full function.  This is
   called if backtrace_full has an error.  */

static void
bxo_bt_err_callback (void *data BXO_UNUSED, const char *msg, int errnum)
{
  if (errnum < 0)
    {
      /* This means that no debug info was available.  Just quietly
         skip printing backtrace info.  */
      return;
    }
  fprintf (stderr, "%s%s%s\n", msg, errnum == 0 ? "" : ": ",
           errnum == 0 ? "" : strerror (errnum));
}                               /* end bxo_bt_err_callback */


void bxo_backtracestr_at (const char*fil, int lin, const std::string&str)
{
  double nowti = bxo_clock_time (CLOCK_REALTIME);
  char thrname[24];
  char buf[256];
  char timbuf[64];
  memset (buf, 0, sizeof (buf));
  memset (thrname, 0, sizeof (thrname));
  memset (timbuf, 0, sizeof (timbuf));
  pthread_getname_np (pthread_self (), thrname, sizeof (thrname) - 1);
  fflush (NULL);
  bxo_strftime_centi (timbuf, sizeof(timbuf), "%Y-%b-%d %H:%M:%S.__ %Z", nowti);
  fprintf (stderr, "BASIXMO BACKTRACE @%s:%d <%s:%d> %s\n* %s\n",
           fil, lin, thrname, (int) bxo_gettid (), timbuf, str.c_str());
  fflush (NULL);
  struct backtrace_state *btstate =
    backtrace_create_state (NULL, 0, bxo_bt_err_callback, NULL);
  if (btstate != NULL)
    {
      int count = 0;
      backtrace_full (btstate, 1, bxo_bt_callback, bxo_bt_err_callback,
                      (void *) &count);
    }
} // end of bxo_backtracestr_at



static struct timespec start_realtime_ts_bxo;

static void
check_updated_binary_bxo(void)
{
  // should run make -C basixmo_directory -q BXO_PROGBINARY
  QProcess makeproc;
  QStringList makeargs;
  makeargs << "-C" << basixmo_directory << "-q" << BXO_PROGBINARY;
  makeproc.start("make",makeargs);
  makeproc.waitForFinished(-1);
  if (makeproc.exitStatus() != QProcess::NormalExit || makeproc.exitCode() != 0)
    {
      BXO_BACKTRACELOG("check_updated_binary binary  " << BXO_PROGBINARY << " in " << basixmo_directory << " is obsolete");
      exit(EXIT_FAILURE);
    }
} // end check_updated_binary_bxo

static void show_size_bxo(void)
{
  printf("sizeof BxoVal : %zd (align %zd)\n",
         sizeof(BxoVal), alignof(BxoVal));
  printf("sizeof BxoObject : %zd (align %zd)\n",
         sizeof(BxoObject), alignof(BxoObject));
  printf("sizeof BxoSequence : %zd (align %zd)\n",
         sizeof(BxoSequence), alignof(BxoSequence));
  printf("sizeof BxoString : %zd (align %zd)\n",
         sizeof(BxoString), alignof(BxoString));
  printf("sizeof shared_ptr<BxoObject> : %zd (align %zd)\n",
         sizeof(std::shared_ptr<BxoObject>), alignof(std::shared_ptr<BxoObject>));
  printf("sizeof unique_ptr<BxoSequence> : %zd (align %zd)\n",
         sizeof(std::unique_ptr<BxoSequence>), alignof(std::unique_ptr<BxoSequence>));
  printf("sizeof weak_ptr<BxoObject> : %zd (align %zd)\n",
         sizeof(std::weak_ptr<BxoObject>), alignof(std::weak_ptr<BxoObject>));
} // end show_size_bxo





// for SQLITE_CONFIG_LOG
static void
bxo_sqlite_errorlog (void *pdata BXO_UNUSED, int errcode, const char *msg)
{
  BXO_BACKTRACELOG("Sqlite Error errcode="<< errcode << " msg=" << msg);
} // end bxo_sqlite_errorlog

int
main (int argc_main, char **argv_main)
{
  clock_gettime (CLOCK_REALTIME, &start_realtime_ts_bxo);
  check_updated_binary_bxo();
  bxo_dlh = dlopen(nullptr, RTLD_NOW|RTLD_GLOBAL);
  if (!bxo_dlh)
    {
      fprintf(stderr, "%s failed to dlopen main program (%s)\n",
              argv_main[0], dlerror());
      exit(EXIT_FAILURE);
    }
  bool nogui = false;
  for (int ix=1; ix<argc_main; ix++)
    {
      if (!strcmp("--no-gui", argv_main[ix]) || !strcmp("-N", argv_main[ix]) || !strcmp("--batch", argv_main[ix]))
        nogui = true;
      if (!strcmp("-V", argv_main[ix]) || !strcmp("--verbose",argv_main[ix]))
        bxo_verboseflag = true;
    }
  sqlite3_config (SQLITE_CONFIG_LOG, bxo_sqlite_errorlog, NULL);
  BxoObject::initialize_predefined_objects();
  QCoreApplication* app = nogui?new QCoreApplication(argc_main, argv_main):new QApplication(argc_main, argv_main);
  app->setApplicationName("Basixmo");
  app->setOrganizationName("gcc-melt.org");
  app->setApplicationVersion(basixmo_lastgitcommit);
  QCommandLineParser cmdlinparser;
  cmdlinparser.setApplicationDescription("Basile's Experimental Monitor");
  QCommandLineOption noguioption(QStringList() << "N" << "no-gui",
                                 "dont start the Qt graphical interface");
  QCommandLineOption dumpdiroption(QStringList() << "D" << "dump-dir",
                                   "Use <directory> for dumps (but dont dump if a dash - is given)",
                                   "directory");
  QCommandLineOption loaddiroption("load-dir",
                                   "Use <directory> for load",
                                   "directory");
  QCommandLineOption infooption("info",
                                "give various info");
  QCommandLineOption verboseoption(QStringList() <<"V" << "verbose",
                                   "give verbose debug output");
  cmdlinparser.addHelpOption();
  cmdlinparser.addVersionOption();
  cmdlinparser.addOption(noguioption);
  cmdlinparser.addOption(dumpdiroption);
  cmdlinparser.addOption(loaddiroption);
  cmdlinparser.addOption(infooption);
  cmdlinparser.addOption(verboseoption);
  cmdlinparser.process(*app);
  if (cmdlinparser.isSet(infooption))
    {
      show_size_bxo();
    }
  if (cmdlinparser.isSet(verboseoption))
    bxo_verboseflag = true;
  if (cmdlinparser.isSet(dumpdiroption))
    {
      auto dumpdirstr = cmdlinparser.value(dumpdiroption).toStdString();
      if (dumpdirstr.empty() || dumpdirstr=="-")
        BxoDumper::set_default_dump_dir("");
      else
        BxoDumper::set_default_dump_dir(dumpdirstr);
    }
  if (cmdlinparser.isSet(loaddiroption))
    {
      auto loaddirstr = cmdlinparser.value(loaddiroption).toStdString();
      BXO_VERBOSELOG("before loading from " << loaddirstr);
      BxoLoader loader {loaddirstr};
      loader.load();
    }
  else
    {
      BXO_VERBOSELOG("before loading");
      BxoLoader loader;
      loader.load();
    }
  BXO_VERBOSELOG("all_names=(" << BxoOut([=](std::ostream&out)
  {
    auto an = BxoObject::all_names();
    for (std::string ns: an)
      {
        auto ob = BxoObject::find_named_objptr(ns);
        out << " " << ns << ":" << ob->strid();
        BXO_ASSERT(ob->name() == ns, "for ns=" << ns << " ob=" << ob << " with bad name " << ob->name());
      }
  }) << ")");
  BXO_VERBOSELOG("comment,payload_hashset pair is "
                 << BxoVSet(BXO_VARPREDEF(comment),BXO_VARPREDEF(payload_hashset)));
  if (!nogui)
    {
      bxo_gui_init(dynamic_cast<QApplication*>(app));
      BXO_VERBOSELOG("before exec app=" << app << " of " << bxo_demangled_typename(typeid(*app)));
      app->exec();
      bxo_gui_stop(dynamic_cast<QApplication*>(app));
      BXO_VERBOSELOG("after exec app=" << app);
    }
  else
    {
      fprintf(stderr, "no Graphical User Interface for Basixmo process %d\n",
              (int)getpid());
      fflush(nullptr);
    }
  if (!BxoDumper::default_dump_dir().empty())
    {
      fprintf(stderr, "dumping into %s\n", BxoDumper::default_dump_dir().c_str());
      BxoDumper du(BxoDumper::default_dump_dir());
      du.full_dump();
    }
  printf("Basixmo ending pid %d (%.4f elapsed, %.4f process cpu seconds)\n",
         (int)getpid(), bxo_elapsed_real_time (), bxo_process_cpu_time ());
  fflush(nullptr);
} // end of main

double
bxo_elapsed_real_time (void)
{
  struct timespec curts = { 0, 0 };
  clock_gettime (CLOCK_REALTIME, &curts);
  return 1.0 * (curts.tv_sec - start_realtime_ts_bxo.tv_sec)
         + 1.0e-9 * (curts.tv_nsec - start_realtime_ts_bxo.tv_nsec);
}

double
bxo_process_cpu_time (void)
{
  struct timespec curts = { 0, 0 };
  clock_gettime (CLOCK_PROCESS_CPUTIME_ID, &curts);
  return 1.0 * (curts.tv_sec) + 1.0e-9 * (curts.tv_nsec);
}

double
bxo_thread_cpu_time (void)
{
  struct timespec curts = { 0, 0 };
  clock_gettime (CLOCK_THREAD_CPUTIME_ID, &curts);
  return 1.0 * (curts.tv_sec) + 1.0e-9 * (curts.tv_nsec);
}


#define BASE_YEAR_BXO 2016
void BxoGplv3LicenseOut::out(std::ostream&os) const
{
  constexpr const char* copowner = "Basile Starynkevitch & later the FSF";
  time_t now = 0;
  time (&now);
  struct tm nowtm;
  memset (&nowtm, 0, sizeof (nowtm));
  localtime_r (&now, &nowtm);
  os << _prefix << " **generated** file " << _file << " - DO NOT EDIT!! " << _suffix << '\n';
  os << _prefix << ' ' << _suffix << '\n';
  if (1900+nowtm.tm_year != BASE_YEAR_BXO)
    os << _prefix << " Copyright (C) " << BASE_YEAR_BXO << " - "
       << 1900+nowtm.tm_year << " " << copowner << ' ' << _suffix << '\n';
  else
    os << _prefix << " Copyright (C) " << BASE_YEAR_BXO << " " << copowner << ' ' <<_suffix << '\n';
  os << _prefix << " This generated file " << _file << " is part of BASIXMO " << _suffix << '\n';
  os << _prefix << ' ' << _suffix << '\n';
  os << _prefix << " BASIXMO is free software; you can redistribute it and/or modify " << _suffix << '\n';
  os << _prefix << " it under the terms of the GNU General Public License as published by " << _suffix << '\n';
  os << _prefix << " the Free Software Foundation; either version 3, or (at your option) " << _suffix << '\n';
  os << _prefix << " any later version. " << _suffix << '\n';
  os << _prefix << ' ' << _suffix << '\n';
  os << _prefix << " BASIXMO is distributed in the hope that it will be useful, " << _suffix << '\n';
  os << _prefix << " but WITHOUT ANY WARRANTY; without even the implied warranty of " << _suffix << '\n';
  os << _prefix << " MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the " << _suffix << '\n';
  os << _prefix << " GNU General Public License for more details. " << _suffix << '\n';
  os << _prefix << " You should have received a copy of the GNU General Public License " << _suffix << '\n';
  os << _prefix << " along with BASIXMO; see the file COPYING3.   If not see " << _suffix << '\n';
  os << _prefix << " <http://www.gnu.org/licenses/>. " << _suffix << std::endl;
} // end BxoGplv3LicenseOut::out
