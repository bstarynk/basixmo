// file guiqt.cc - the Graphical User Interface using Qt5

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
#include <QObject>
#include <QMainWindow>
#include <QTextEdit>
#include <QToolBar>
#include <QMenuBar>
#include <QApplication>
#include <QMessageBox>
#include <QCloseEvent>

class BxoMainWindowPayl :public QMainWindow,  public BxoPayload
{
  Q_OBJECT
  QTextEdit *_textedit;
public:
  BxoMainWindowPayl(BxoObject*own);
  ~BxoMainWindowPayl();
  virtual std::shared_ptr<BxoObject> kind_ob() const
  {
    return BXO_VARPREDEF(payload_main_window);
  };
  virtual std::shared_ptr<BxoObject> module_ob() const
  {
    return nullptr;
  };
  /// actually, main window payloads are transient, because we don't
  /// define any bxoload_6Yd83xiypqdhqcztq function; so none of these
  /// functions would be called
  virtual void load_payload_content(const BxoJson&, BxoLoader&)
  {
    throw std::logic_error("BxoMainWindowPayl::load_payload_content");
  }
  virtual const BxoJson emit_payload_content(BxoDumper&) const
  {
    throw std::logic_error("BxoMainWindowPayl::emit_payload_content");
  };
  virtual void scan_payload_content(BxoDumper&) const
  {
    throw std::logic_error("BxoMainWindowPayl::scan_payload_content");
  };
  virtual void closeEvent(QCloseEvent*ev);
};        // end BxoMainWindowPayl


BxoMainWindowPayl::BxoMainWindowPayl(BxoObject*own):
  QMainWindow(), BxoPayload(*own,PayloadTag {}), _textedit(nullptr)
{
  _textedit = new QTextEdit;
  setCentralWidget(_textedit);
  auto mb = menuBar();
  auto filemenu = mb->addMenu("&File");
  filemenu->addAction("&Dump",[=]
  {
    if (BxoDumper::default_dump_dir().empty())
      BxoDumper::set_default_dump_dir(".");
    BXO_BACKTRACELOG("file dump into " << BxoDumper::default_dump_dir());
    BxoDumper du(BxoDumper::default_dump_dir());
    du.full_dump();
    BXO_VERBOSELOG("done dump into " << BxoDumper::default_dump_dir());
  });
  filemenu->addAction("save and e&Xit",[=]
  {
    if (BxoDumper::default_dump_dir().empty())
      BxoDumper::set_default_dump_dir(".");
    BXO_BACKTRACELOG("save and exit file into " << BxoDumper::default_dump_dir());
    BxoDumper du(BxoDumper::default_dump_dir());
    du.full_dump();
    QApplication::exit();
    BXO_VERBOSELOG("done final dump into " << BxoDumper::default_dump_dir());
  });
  filemenu->addAction("&Quit",[=]
  {
    BXO_BACKTRACELOG("quit file without dump");
    int ret = QMessageBox::question((QWidget*)this,
    QString{"Quit?"},
    QString{"Quit Basixmo (without dump)?"},
    QMessageBox::Ok | QMessageBox::Cancel,
    QMessageBox::Cancel);
    if (ret == QMessageBox::Ok)
      QApplication::exit();
  });
}/// end BxoMainWindowPayl::BxoMainWindowPayl

void
BxoMainWindowPayl::closeEvent(QCloseEvent*clev)
{
  BXO_BACKTRACELOG("close main window owner=" << owner());
  int ret = QMessageBox::question((QWidget*)this,
                                  QString {"close and Quit?"},
                                  QString {"close and Quit Basixmo (without dump)?"},
                                  QMessageBox::Ok | QMessageBox::Cancel,
                                  QMessageBox::Cancel);
  if (ret == QMessageBox::Ok)
    clev->accept();
  else
    clev->ignore();
} // end BxoMainWindowPayl::closeEvent


BxoMainWindowPayl::~BxoMainWindowPayl()
{
  BXO_BACKTRACELOG("owner=" << owner());
  delete _textedit;
  _textedit = nullptr;
} // end BxoMainWindowPayl::~BxoMainWindowPayl


// bxoglob_the_system
void bxo_gui_init(QApplication*qapp)
{
  BXO_ASSERT(qapp != nullptr, "no QApplication");
  BXO_BACKTRACELOG("bxo_gui_init start");
  auto mainwinob = BxoObject::make_objref();
  BXO_VERBOSELOG("empty mainwinob=" << mainwinob);
  mainwinob->put_payload<BxoMainWindowPayl>();
  BXO_VERBOSELOG("mainwinob=" << mainwinob);
  auto mainwin = mainwinob->dyncast_payload<BxoMainWindowPayl>();
  mainwin->show();
  mainwin->resize(300,200);
  BXO_VERBOSELOG("shown mainwin=" << mainwin << " of " << bxo_demangled_typename(typeid(*mainwin)));
  auto theguihset = BXO_VARPREDEF(the_GUI)->dyncast_payload<BxoHashsetPayload>();
  theguihset->add(mainwinob);
  BXO_VERBOSELOG("bxo_gui_init end");
} // end bxo_gui_init

void bxo_gui_stop(QApplication*qapp)
{
  BXO_BACKTRACELOG("bxo_gui_stop start qapp=" << qapp);
  BXO_ASSERT(qapp != nullptr, "no qapp");
  auto theguihset = BXO_VARPREDEF(the_GUI)->dyncast_payload<BxoHashsetPayload>();
  {
    auto pset = theguihset->vset().get_set();
    BXO_VERBOSELOG("pset length=" << pset->length());
    for (auto pob: *pset)
      {
        pob->dyncast_payload<BxoMainWindowPayl>()->close();
        pob->reset_payload();
      }
  }
  theguihset->clear();
  BXO_VERBOSELOG("bxo_gui_stop end");
} // end bxo_gui_stop

#include "guiqt.moc.h"
