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
#include <QProcess>
#include <QCoreApplication>
#include <QApplication>
#include <QCommandLineParser>

thread_local BxoRandom BxoRandom::_rand_thr_;
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


  fprintf (stderr, "Basixmo[0x%lx] %s\n\t%s:%d\n",
           (unsigned long) pc,
           function == NULL ? "???" : function,
           filename == NULL ? "???" : filename, lineno);

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

int
main (int argc_main, char **argv_main)
{
  clock_gettime (CLOCK_REALTIME, &start_realtime_ts_bxo);
  BxoObject::initialize_predefined_objects();
  check_updated_binary_bxo();
  bool nogui = false;
  for (int ix=1; ix<argc_main; ix++)
    if (!strcmp("--no-gui", argv_main[ix]) || !strcmp("-N", argv_main[ix]) || !strcmp("--batch", argv_main[ix]))
      nogui = true;
  QCoreApplication* app = nogui?new QCoreApplication(argc_main, argv_main):new QApplication(argc_main, argv_main);
  app->setApplicationName("Basixmo");
  app->setApplicationVersion(basixmo_lastgitcommit);
  QCommandLineParser cmdlinparser;
  cmdlinparser.setApplicationDescription("Basile's Experimental Monitor");
  QCommandLineOption noguioption(QStringList() << "N" << "no-gui",
                                 "dont start the Qt graphical interface");
  QCommandLineOption dumpdiroption(QStringList() << "D" << "dump-dir",
                                   "Use <directory> for dumps (but dont dump if a dash - is given)",
                                   "directory");
  QCommandLineOption infooption("info",
                                "give various info");
  cmdlinparser.addHelpOption();
  cmdlinparser.addVersionOption();
  cmdlinparser.addOption(noguioption);
  cmdlinparser.addOption(dumpdiroption);
  cmdlinparser.addOption(infooption);
  cmdlinparser.process(*app);
  if (cmdlinparser.isSet(infooption))
    {
      show_size_bxo();
    }
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


