-- basixmo_state.sql dump 2017 Jan 23 from basixmo_state.sqlite dumped by ./basixmo-dump-state.sh .....

 --   Copyright (C) 2017 Basile Starynkevitch.
 --  This sqlite3 dump file basixmo_state.sql is part of BASIXMO.
 --
 --  BASIXMO is free software; you can redistribute it and/or modify
 --  it under the terms of the GNU General Public License as published by
 --  the Free Software Foundation; either version 3, or (at your option)
 --  any later version.
 --
 --  BASIXMO is distributed in the hope that it will be useful,
 --  but WITHOUT ANY WARRANTY; without even the implied warranty of
 --  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 --  GNU General Public License for more details.
 --  You should have received a copy of the GNU General Public License
 --  along with BASIXMO; see the file COPYING3.   If not see
 --  <http://www.gnu.org/licenses/>.

PRAGMA foreign_keys=OFF;
BEGIN TRANSACTION;
CREATE TABLE t_params (par_name VARCHAR(35) PRIMARY KEY ASC NOT NULL UNIQUE,  par_value TEXT NOT NULL);
CREATE TABLE t_objects (ob_id VARCHAR(20) PRIMARY KEY ASC NOT NULL UNIQUE, ob_mtime DATETIME, ob_jsoncont TEXT NOT NULL, ob_classid VARCHAR(20) NOT NULL, ob_paylkid VARCHAR(20) NOT NULL, ob_paylcont TEXT NOT NULL, ob_paylmod VARCHAR(20) NOT NULL);
INSERT INTO "t_objects" VALUES('_6Yd83xiypqdhqcztq',1477720082,'{
   "@name" : "payload_main_window",
   "attrs" : [
      {
         "at" : "_4xS1CSbRUFBW6PJiJ",
         "va" : "for main window payload"
      }
   ],
   "comps" : []
}
','','','','');
INSERT INTO "t_objects" VALUES('_0BV96V94PJIn9si1K',0,'{
   "@name" : "the_system",
   "attrs" : [
      {
         "at" : "_4xS1CSbRUFBW6PJiJ",
         "va" : "describes the entire system"
      }
   ],
   "comps" : []
}
','','_7yw2rDBxjs1XBu2ee','{
   "@owner" : "_0BV96V94PJIn9si1K",
   "globalpath" : "_bxo_global.h",
   "predefpath" : "_bxo_predef.h",
   "system" : true
}
','');
INSERT INTO "t_objects" VALUES('_2A45sdKj6wkrVJIrv',1478171098,'{
   "@name" : "payload_main_graphics_scene",
   "attrs" : [
      {
         "at" : "_4xS1CSbRUFBW6PJiJ",
         "va" : "main graphics scene transient payload"
      }
   ],
   "comps" : []
}
','','','','');
INSERT INTO "t_objects" VALUES('_8261sbF1f9ohzu2Iu',1473049630,'{
   "@name" : "payload_hashset",
   "attrs" : [
      {
         "at" : "_4xS1CSbRUFBW6PJiJ",
         "va" : "for hashset payload"
      }
   ],
   "comps" : []
}
','','','','');
INSERT INTO "t_objects" VALUES('_7yw2rDBxjs1XBu2ee',1477571803,'{
   "@name" : "payload_system",
   "attrs" : [
      {
         "at" : "_4xS1CSbRUFBW6PJiJ",
         "va" : "payload of the_system"
      }
   ],
   "comps" : []
}
','','','','');
INSERT INTO "t_objects" VALUES('_4xS1CSbRUFBW6PJiJ',1472212346,'{
   "@name" : "comment",
   "attrs" : [
      {
         "at" : "_4xS1CSbRUFBW6PJiJ",
         "va" : "for comments, often a string"
      }
   ],
   "comps" : []
}
','','','','');
INSERT INTO "t_objects" VALUES('_6Pc0DV2SCkMSiNlcf',1474462452,'{
   "@name" : "the_GUI",
   "attrs" : [
      {
         "at" : "_4xS1CSbRUFBW6PJiJ",
         "va" : "for the graphical user interface"
      }
   ],
   "comps" : []
}
','','_8261sbF1f9ohzu2Iu','{
   "@owner" : "_6Pc0DV2SCkMSiNlcf",
   "hashset" : []
}
','');
INSERT INTO "t_objects" VALUES('_5JG8lVw6jwlUT7PLK',1473050875,'{
   "@name" : "payload_assoval",
   "attrs" : [
      {
         "at" : "_4xS1CSbRUFBW6PJiJ",
         "va" : "for assovaldata payload"
      }
   ],
   "comps" : []
}
','','','','');
CREATE TABLE t_names (nam_str PRIMARY KEY ASC NOT NULL UNIQUE, nam_oid  VARCHAR(20) NOT NULL UNIQUE);
INSERT INTO "t_names" VALUES('comment','_4xS1CSbRUFBW6PJiJ');
INSERT INTO "t_names" VALUES('payload_assoval','_5JG8lVw6jwlUT7PLK');
INSERT INTO "t_names" VALUES('payload_hashset','_8261sbF1f9ohzu2Iu');
INSERT INTO "t_names" VALUES('payload_main_graphics_scene','_2A45sdKj6wkrVJIrv');
INSERT INTO "t_names" VALUES('payload_main_window','_6Yd83xiypqdhqcztq');
INSERT INTO "t_names" VALUES('payload_system','_7yw2rDBxjs1XBu2ee');
INSERT INTO "t_names" VALUES('the_GUI','_6Pc0DV2SCkMSiNlcf');
INSERT INTO "t_names" VALUES('the_system','_0BV96V94PJIn9si1K');
CREATE TABLE t_modules (mod_oid VARCHAR(20) PRIMARY KEY ASC NOT NULL UNIQUE);
CREATE UNIQUE INDEX x_namedid ON t_names (nam_oid);
COMMIT;
-- basixmo-dump-state end dump basixmo_state.sqlite
