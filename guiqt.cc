// file guiqt.cc - the Graphical User Interface using Qt5

/**   Copyright (C)  2016 - 2017 Basile Starynkevitch

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
#include <QSettings>
#include <QTextEdit>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QGraphicsView>
#include <QGraphicsLinearLayout>
#include <QGraphicsLayoutItem>
#include <QGraphicsItemGroup>
#include <QToolBar>
#include <QMenuBar>
#include <QApplication>
#include <QMessageBox>
#include <QCloseEvent>

class BxoMainWindowPayl;
class BxoMainGraphicsScenePayl;

class BxoMainWindowPayl :public QMainWindow,  public BxoPayload
{
  friend class BxoMainGraphicsScenePayl;
  Q_OBJECT
  QGraphicsView* _graview;
  std::shared_ptr<BxoObject> _grascenob;
  bool _askclose;
protected:
  void fill_menu(void);
public:
  BxoMainWindowPayl(BxoObject*own);
  BxoMainWindowPayl(BxoObject*own, std::shared_ptr<BxoObject> grascenob);
  BxoMainWindowPayl(BxoObject*own, BxoMainGraphicsScenePayl*grascenpayl);
  ~BxoMainWindowPayl();
  virtual std::shared_ptr<BxoObject> kind_ob() const
  {
    return BXO_VARPREDEF(payload_main_window);
  };
  virtual std::shared_ptr<BxoObject> module_ob() const
  {
    return nullptr;
  };
  inline std::shared_ptr<BxoObject> grascen_ob() const;
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
  void ask_when_closing(bool askme)
  {
    _askclose = askme;
  };
};        // end BxoMainWindowPayl



class BxoAnyShow
{
private:
protected:
  struct ShowTag {};
  inline BxoAnyShow(struct ShowTag, BxoAnyShow* parent=nullptr);
public:
  inline BxoMainGraphicsScenePayl* gscene() const;
  inline void put_in_gscene(BxoMainGraphicsScenePayl*gsp);
  inline void remove_from_gscene(void);
  virtual ~BxoAnyShow();
  virtual BxoVal bval(void) const
  {
    return nullptr;
  };
  virtual QGraphicsItem* gitem(void) const  =0;
};        // end BxoAnyShow



class BxoAnyObjrefShow: public BxoAnyShow
{
  std::shared_ptr<BxoObject> _obref;
protected:
  // subclasses should implement virtual methods of BxoAnyShow
  virtual BxoVal bval(void) const
  {
    return BxoVObj(_obref);
  };
  BxoAnyObjrefShow(const std::shared_ptr<BxoObject>&obp, struct ShowTag tg, BxoAnyShow* parent=nullptr)
    : BxoAnyShow(tg,parent), _obref(obp) {};
};        // end BxoAnyObjrefShow


/// show an object reference for a named object
class BxoNamedObjrefShow : public BxoAnyObjrefShow
{
  QGraphicsSimpleTextItem _nametxit;
  BxoNamedObjrefShow(const std::shared_ptr<BxoObject>&obp, struct ShowTag tg, BxoAnyShow* parent=nullptr);
public:
  static BxoAnyObjrefShow*make(BxoObject*ob, BxoMainGraphicsScenePayl*gscen);
  virtual QGraphicsItem* gitem(void) const
  {
    return const_cast<QGraphicsItem*>(static_cast<const QGraphicsItem*>(&_nametxit));
  };
};        // end class BxoNamedObjrefShow


/// show an object reference for an anonymous object without comment
class BxoAnonObjrefShow :   public BxoAnyObjrefShow
{
  QGraphicsSimpleTextItem _idtxit;
  BxoAnonObjrefShow(const std::shared_ptr<BxoObject>&obp, struct ShowTag tg, BxoAnyShow* parent=nullptr);
public:
  static BxoAnyObjrefShow*make(BxoObject*ob, BxoMainGraphicsScenePayl*gscen);
  virtual QGraphicsItem* gitem(void) const
  {
    return const_cast<QGraphicsItem*>(static_cast<const QGraphicsItem*>(&_idtxit));
  };
};        // end class BxoAnonObjrefShow



/// show an object reference for a commented anonymous object
class BxoCommentedObjrefShow :   public BxoAnyObjrefShow
{
  QGraphicsItemGroup _groupit;
  QGraphicsSimpleTextItem _idtxit;
  QGraphicsSimpleTextItem _commtxit;
  BxoCommentedObjrefShow(const std::shared_ptr<BxoObject>&obp, struct ShowTag tg, BxoAnyShow* parent=nullptr);
public:
  static BxoAnyObjrefShow*make(BxoObject*ob, BxoMainGraphicsScenePayl*gscen);
  virtual QGraphicsItem* gitem(void) const
  {
    return const_cast<QGraphicsItem*>(static_cast<const QGraphicsItem*>(&_groupit));
  };
};        // end BxoCommentedObjrefShow

class BxoShownObjectGroup : public BxoAnyObjrefShow
{
  QGraphicsItemGroup _groupit;
public:
  static BxoAnyObjrefShow*make(BxoObject*ob, BxoMainGraphicsScenePayl*gscen);
  virtual QGraphicsItem* gitem(void) const
  {
    return const_cast<QGraphicsItem*>(static_cast<const QGraphicsItem*>(&_groupit));
  };
};        // end BxoShownObjectGroup


class BxoMainGraphicsScenePayl :public QGraphicsScene,  public BxoPayload
{
  friend class BxoMainWindowPayl;
  friend class BxoAnyObjrefShow;
  std::map<std::shared_ptr<BxoObject>,BxoShownObjectGroup,BxoAlphaLessObjSharedPtr>
  _shownobjmap;
  std::unordered_multimap<std::shared_ptr<BxoObject>,BxoAnyObjrefShow*,BxoHashObjSharedPtr> _objoccmultimap;
  QGraphicsLinearLayout _layout;
  Q_OBJECT
  static std::unique_ptr<QBrush> _nilbrush_;
  static std::unique_ptr<QFont> _nilfont_;
  static std::unique_ptr<QBrush> _intbrush_;
  static std::unique_ptr<QFont> _intfont_;
  static std::unique_ptr<QBrush> _smallstringbrush_;
  static std::unique_ptr<QFont> _smallstringfont_;
  static std::unique_ptr<QColor> _bigstringcolor_;
  static std::unique_ptr<QFont> _bigstringfont_;
  static std::unique_ptr<QBrush> _namebrush_;
  static std::unique_ptr<QFont> _namefont_;
protected:
  void add_obshow(BxoAnyObjrefShow*osh);
  void remove_obshow(BxoAnyObjrefShow*osh);
public:
  static void initialize(QApplication*);
  static void finalize(QApplication*);
  BxoMainGraphicsScenePayl(BxoObject*own);
  ~BxoMainGraphicsScenePayl();
  bool is_shown_objref(const std::shared_ptr<BxoObject>pob) const
  {
    return pob && _shownobjmap.find(pob) != _shownobjmap.end();
  }
  virtual std::shared_ptr<BxoObject> kind_ob() const
  {
    return BXO_VARPREDEF(payload_main_graphics_scene);
  };
  virtual std::shared_ptr<BxoObject> module_ob() const
  {
    return nullptr;
  };
  QGraphicsItem* value_gitem(const BxoVal&val, int depth);
  // display an object, possibly with its internal content;
  // *pshowncontent would be set to true if the content is shown at
  // this occurrence
  BxoAnyObjrefShow* objref_gitem(const std::shared_ptr<BxoObject>pob, int depth, bool*pshowncontent =nullptr);
  // display an object and its content
  BxoAnyObjrefShow* objcontent_gitem(BxoObject*obp, int depth);
  /// actually, main graphics scene payloads are transient, so none of these
  /// functions would be called
  virtual void load_payload_content(const BxoJson&, BxoLoader&)
  {
    throw std::logic_error("BxoMainGraphicsScenePayl::load_payload_content");
  }
  virtual const BxoJson emit_payload_content(BxoDumper&) const
  {
    throw std::logic_error("BxoMainGraphicsScenePayl::emit_payload_content");
  };
  virtual void scan_payload_content(BxoDumper&) const
  {
    throw std::logic_error("BxoMainGraphicsScenePayl::scan_payload_content");
  };
};        // end BxoMainGraphicsScenePayl











void
BxoMainWindowPayl::fill_menu(void)
{
  auto mb = menuBar();
  auto filemenu = mb->addMenu("&File");
  filemenu->addAction("&Dump",[=]
  {
    if (BxoDumper::default_dump_dir().empty())
      BxoDumper::set_default_dump_dir(".");
    BXO_BACKTRACELOG("*menu file dump into " << BxoDumper::default_dump_dir());
    BxoDumper du(BxoDumper::default_dump_dir());
    du.full_dump();
    BXO_VERBOSELOG("menu done dump into " << BxoDumper::default_dump_dir());
  });
  filemenu->addAction("save and e&Xit",[=]
  {
    if (BxoDumper::default_dump_dir().empty())
      BxoDumper::set_default_dump_dir(".");
    BXO_BACKTRACELOG("*menu save and exit file into " << BxoDumper::default_dump_dir());
    BxoDumper du(BxoDumper::default_dump_dir());
    du.full_dump();
    QApplication::exit();
    BXO_VERBOSELOG("menu done final dump into " << BxoDumper::default_dump_dir());
  });
  filemenu->addAction("&Quit",[=]
  {
    BXO_BACKTRACELOG("*menu quit file without dump");
    int ret = QMessageBox::question((QWidget*)this,
    QString{"Quit?"},
    QString{"Quit Basixmo (without dump)?"},
    QMessageBox::Ok | QMessageBox::Cancel,
    QMessageBox::Cancel);
    if (ret == QMessageBox::Ok)
      QApplication::exit();
  });
  auto objmenu = mb->addMenu("&Object");
  objmenu->addAction("&Show", [=]
  {
    BXO_BACKTRACELOG("*menu object show");
#warning incomplete menu object show
  });
} // end BxoMainWindowPayl::fill_menu



BxoMainWindowPayl::BxoMainWindowPayl(BxoObject*own)
  : QMainWindow(), BxoPayload(*own,PayloadTag {}), _graview(nullptr), _askclose(true)
{
  BxoMainGraphicsScenePayl*grscenpy = nullptr;
  _grascenob = BxoObject::make_objref();
  grscenpy = _grascenob->put_payload<BxoMainGraphicsScenePayl>();
  _graview = new QGraphicsView(grscenpy);
  setCentralWidget(_graview);
  fill_menu();
}/// end BxoMainWindowPayl::BxoMainWindowPayl



BxoMainWindowPayl::BxoMainWindowPayl(BxoObject*own, std::shared_ptr<BxoObject> grascenob)
  : QMainWindow(), BxoPayload(*own,PayloadTag {}), _graview(nullptr), _askclose(true)
{
  BxoMainGraphicsScenePayl*grscenpy = nullptr;
  if (!grascenob)
    _grascenob = grascenob = BxoObject::make_objref();
  if (!grascenob->payload())
    grscenpy = _grascenob->put_payload<BxoMainGraphicsScenePayl>();
  else if (!(grscenpy=grascenob->dyncast_payload<BxoMainGraphicsScenePayl>()))
    {
      BXO_BACKTRACELOG("BxoMainWindowPayl owned by " << own << " with bad grascenob " << grascenob
                       << " of payload kind " << grascenob->payload()->kind_ob());
      throw std::runtime_error("BxoMainWindowPayl with bad grascenob");
    }
  _grascenob = grascenob;
  _graview = new QGraphicsView(grscenpy);
  setCentralWidget(_graview);
  fill_menu();

} // end of BxoMainWindowPayl::BxoMainWindowPayl


BxoMainWindowPayl::BxoMainWindowPayl(BxoObject*own, BxoMainGraphicsScenePayl*grascenpy)
  : QMainWindow(), BxoPayload(*own,PayloadTag {}), _graview(nullptr), _askclose(true)
{
  if (!grascenpy)
    {
      BXO_BACKTRACELOG("BxoMainWindowPayl no grascenpy owner=" << own);
      throw std::runtime_error("BxoMainWindowPayl without grascenpy");
    }
  _grascenob.reset(grascenpy->owner());
  _graview = new QGraphicsView(grascenpy);
  setCentralWidget(_graview);
  fill_menu();
} // end of BxoMainWindowPayl::BxoMainWindowPayl


void
BxoMainWindowPayl::closeEvent(QCloseEvent*clev)
{
  BXO_BACKTRACELOG("MainWin.closeEvent close main window owner=" << owner() << " askclose is " << (_askclose?"true":"false"));
  if (!_askclose)
    {
      BXO_VERBOSELOG("MainWin.closeEvent dont ask for closing main window " << this << " owner=" << owner());
      clev->accept();
      QMainWindow::closeEvent(clev);
      return;
    }
  int ret = QMessageBox::question((QWidget*)this,
                                  QString {"close and Quit?"},
                                  QString {"close and Quit Basixmo (without dump)?"},
                                  QMessageBox::Ok | QMessageBox::Cancel,
                                  QMessageBox::Cancel);
  if (ret == QMessageBox::Ok)
    {
      BXO_VERBOSELOG("closing main window " << this << " owner=" << owner());
      clev->accept();
      QMainWindow::closeEvent(clev);
    }
  else
    {
      BXO_VERBOSELOG("keeping (NOT closing) main window " << this << " owner=" << owner());
      clev->ignore();
    }
} // end BxoMainWindowPayl::closeEvent


BxoMainWindowPayl::~BxoMainWindowPayl()
{
  BXO_BACKTRACELOG("owner=" << owner());
  delete _graview;
  _graview = nullptr;
} // end BxoMainWindowPayl::~BxoMainWindowPayl


std::shared_ptr<BxoObject>
BxoMainWindowPayl::grascen_ob() const
{
  if (_grascenob && _grascenob->dyncast_payload<BxoMainGraphicsScenePayl>())
    return _grascenob;
  else
    return nullptr;
}

BxoAnyShow::BxoAnyShow(struct ShowTag, BxoAnyShow* parent)
{
  if (parent)
    {
    }
};        // end BxoAnyShow::BxoAnyShow

void
BxoAnyShow::put_in_gscene(BxoMainGraphicsScenePayl*gsp)
{
  auto cgs = gscene();
  if (cgs == gsp) return;
  if (cgs)
    cgs->removeItem((QGraphicsItem*)this);
  if (gsp)
    gsp->addItem((QGraphicsItem*)this);
};        // end BxoAnyShow::put_in_gscene

void
BxoAnyShow::remove_from_gscene(void)
{
  auto cgs = gscene();
  if (cgs)
    cgs->removeItem((QGraphicsItem*)this);
};        // end BxoAnyShow::remove_from_gscene

BxoAnyShow::~BxoAnyShow()
{
};        // end BxoAnyShow::~BxoAnyShow

BxoMainGraphicsScenePayl::~BxoMainGraphicsScenePayl()
{
  BXO_BACKTRACELOG("owner=" << owner());
} // end BxoMainGraphicsScenePayl::~BxoMainGraphicsScenePayl


BxoMainGraphicsScenePayl::BxoMainGraphicsScenePayl(BxoObject*own)
  : QGraphicsScene(), BxoPayload(*own,PayloadTag {}),
_shownobjmap(), _layout(Qt::Vertical)
{
} // end BxoMainGraphicsScenePayl::BxoMainGraphicsScenePayl

void
BxoMainGraphicsScenePayl::add_obshow(BxoAnyObjrefShow*osh)
{
  BXO_ASSERT(osh, "no obrefshow");
#if 0
  std::shared_ptr<BxoObject> obp = osh->obptr()->shared_from_this();
  _objoccmultimap.insert({obp,osh});
#endif
} // end of BxoMainGraphicsScenePayl::add_obshow

void
BxoMainGraphicsScenePayl::remove_obshow(BxoAnyObjrefShow*osh)
{
  BXO_ASSERT(osh, "no obrefshow");
#if 0
  std::shared_ptr<BxoObject> obp = osh->obptr()->shared_from_this();
  auto r = _objoccmultimap.equal_range(obp);
  for (auto it = r.first; it != r.second; it++)
    {
      if (it->second == osh)
        {
          _objoccmultimap.erase(it);
          return;
        }
    }
  BXO_ASSERT(obp, "corrupted _objoccmultimap");
#endif
} // end of BxoMainGraphicsScenePayl::remove_obshow




std::unique_ptr<QBrush> BxoMainGraphicsScenePayl::_nilbrush_;
std::unique_ptr<QFont> BxoMainGraphicsScenePayl::_nilfont_;
std::unique_ptr<QBrush> BxoMainGraphicsScenePayl::_intbrush_;
std::unique_ptr<QFont> BxoMainGraphicsScenePayl::_intfont_;
std::unique_ptr<QBrush> BxoMainGraphicsScenePayl::_smallstringbrush_;
std::unique_ptr<QFont> BxoMainGraphicsScenePayl::_smallstringfont_;
std::unique_ptr<QBrush> BxoMainGraphicsScenePayl::_namebrush_;
std::unique_ptr<QFont> BxoMainGraphicsScenePayl::_namefont_;
std::unique_ptr<QColor> BxoMainGraphicsScenePayl::_bigstringcolor_;
std::unique_ptr<QFont> BxoMainGraphicsScenePayl::_bigstringfont_;


void
BxoMainGraphicsScenePayl::initialize(QApplication*qapp)
{
  BXO_ASSERT(qapp, "missing application");
  QSettings settings;
  _nilbrush_.reset(new QBrush(QColor(settings.value("scene/nilcolor", "Navy").toString())));
  _nilfont_.reset(new QFont(settings.value("scene/nilfont","Courier Bold 12").toString()));
  _intbrush_.reset(new QBrush(QColor(settings.value("scene/intcolor", "DarkGreen").toString())));
  _intfont_.reset(new QFont(settings.value("scene/intfont","Courier 12").toString()));
  _smallstringbrush_.reset(new QBrush(QColor(settings.value("scene/smallstringcolor", "Firerick").toString())));
  _smallstringfont_.reset(new QFont(settings.value("scene/smallstringfont","Andale Mono 11").toString()));
  _bigstringcolor_.reset(new QColor(settings.value("scene/bigstringcolor", "Peru").toString()));
  _bigstringfont_.reset(new QFont(settings.value("scene/bigstringfont","Andale Mono 11").toString()));

} // end BxoMainGraphicsScenePayl::initialize



void
BxoMainGraphicsScenePayl::finalize(QApplication*qapp)
{
  BXO_ASSERT(qapp, "missing application");
  _nilbrush_.reset();
  _nilfont_.reset();
  _intbrush_.reset();
  _intfont_.reset();
  _smallstringfont_.reset();
  _smallstringbrush_.reset();
  _bigstringfont_.reset();
  _bigstringcolor_.reset();
} // end of BxoMainGraphicsScenePayl::finalize



BxoMainGraphicsScenePayl* BxoAnyShow::gscene() const
{
  auto it = gitem();
  if (it != nullptr)
    return dynamic_cast<BxoMainGraphicsScenePayl*>(it->scene());
  return nullptr;
};

QGraphicsItem*
BxoMainGraphicsScenePayl::value_gitem(const BxoVal&val, int depth)
{
  bool got_set = false;
  switch (val.kind())
    {
    case BxoVKind::NoneK:
    {
      auto qit = new QGraphicsSimpleTextItem("~");
      qit->setFont(*_nilfont_);
      qit->setBrush(*_nilbrush_);
      return qit;
    }
    case BxoVKind::IntK:
    {
      char intbuf[32];
      memset (intbuf, 0, sizeof(intbuf));
      snprintf(intbuf, sizeof(intbuf), "%lld", (long long)val.as_int());
      auto qit = new QGraphicsSimpleTextItem(intbuf);
      qit->setFont(*_intfont_);
      qit->setBrush(*_intbrush_);
      return qit;
    }
    case BxoVKind::StringK:
    {
      constexpr const unsigned smallstringlength = 64;
      auto str = val.as_string();
      if (str.size() < smallstringlength)
        {
          auto qit = new QGraphicsSimpleTextItem(str.c_str());
          qit->setFont(*_smallstringfont_);
          qit->setBrush(*_smallstringbrush_);
          return qit;
        }
      else  //big string
        {
          auto qit = new QGraphicsTextItem(str.c_str());
          qit->setFont(*_bigstringfont_);
          qit->setDefaultTextColor(*_bigstringcolor_);
          QFontMetrics fm(*_bigstringfont_);
          qit->setTextWidth(fm.averageCharWidth()*4*smallstringlength/3);
          return qit;
        }
    }
    case BxoVKind::ObjectK:
    {
      auto shob = objref_gitem(val.as_object(), depth);
      return shob->gitem();
    }
    case BxoVKind::SetK:
      got_set = true;
    // failthru
    case BxoVKind::TupleK:
    {
      auto seq = val.get_sequence();
      unsigned len = seq->length();
    }
    }
} // end  BxoMainGraphicsScenePayl::value_gitem




BxoAnyObjrefShow*
BxoMainGraphicsScenePayl::objref_gitem(const std::shared_ptr<BxoObject>pob, int depth, bool*pshown)
{
  BXO_ASSERT(pob != nullptr, "objref_gitem: no pob");
  if (pob->is_named())
    {
      auto osh = BxoNamedObjrefShow::make(pob.get(),this);
      return osh;
    }
#if 0
  else if (depth <= 0 || is_shown_objref(pob))
    {
      if (pshown) *pshown = false;
      auto comstr = pob->get_attr(BXO_VARPREDEF(comment)).to_string();
      if (comstr.empty())
        {
          return BxoAnonObjrefShow::make(pob.get(),this);
        }
      else
        {
          return BxoCommentedObjrefShow::make(pob.get(), comstr, this);
        }
    }
  else
    {
      if (pshown) *pshown = true;
      return objcontent_gitem(pob.get(), depth);
    }
#endif
} // end  BxoMainGraphicsScenePayl::objref_gitem



BxoAnyObjrefShow*
BxoMainGraphicsScenePayl::objcontent_gitem(BxoObject*obp, int depth)
{
  BXO_ASSERT(obp != nullptr, "no obp");
}

// bxoglob_the_system
void bxo_gui_init(QApplication*qapp)
{
  BXO_ASSERT(qapp != nullptr, "no QApplication");
  BXO_BACKTRACELOG("bxo_gui_init start");
  BxoMainGraphicsScenePayl::initialize(qapp);
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



void
bxo_gui_stop(QApplication*qapp)
{
  BXO_BACKTRACELOG("bxo_gui_stop start qapp=" << qapp);
  BXO_ASSERT(qapp != nullptr, "no qapp");
  auto theguihset = BXO_VARPREDEF(the_GUI)->dyncast_payload<BxoHashsetPayload>();
  {
    auto pset = theguihset->vset().get_set();
    BXO_VERBOSELOG("bxo_gui_stop pset length=" << pset->length() << "; pset=" << pset);
    for (auto pob: *pset)
      {
        BXO_VERBOSELOG("bxo_gui_stop loop pob=" << pob);
        auto winp = pob->dyncast_payload<BxoMainWindowPayl>();
        BXO_ASSERT(winp != nullptr, "no winp");
        BXO_VERBOSELOG("bxo_gui_stop winp=" << winp << " pob=" << pob);
        winp->ask_when_closing(false);
        winp->close();
        pob->reset_payload();
      }
  }
  theguihset->clear();
  BxoMainGraphicsScenePayl::finalize(qapp);
  BXO_VERBOSELOG("bxo_gui_stop end");
} // end bxo_gui_stop


BxoNamedObjrefShow::BxoNamedObjrefShow(const std::shared_ptr<BxoObject>&obp, struct ShowTag tg, BxoAnyShow* parent)
  : BxoAnyObjrefShow(obp,tg,parent)
{
}

BxoAnyObjrefShow*
BxoNamedObjrefShow::make(BxoObject*obp, BxoMainGraphicsScenePayl* grascen)
{
  BXO_ASSERT(obp, "no obp");
  BXO_ASSERT(grascen, "no grascen");
  return new BxoNamedObjrefShow(obp->shared_from_this(), ShowTag {});
} // end BxoNamedObjrefShow::make


////////////////
#include "guiqt.moc.h"
//// eof guiqt.cc
