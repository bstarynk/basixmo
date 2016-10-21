-- basixmo_state.sql dump 2016 Oct 20 from basixmo_state.sqlite dumped by ./basixmo-dump-state.sh .....

 --   Copyright (C) 2016 Basile Starynkevitch.
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
CREATE TABLE t_objects (ob_id VARCHAR(20) PRIMARY KEY ASC NOT NULL UNIQUE,
                        ob_mtime DATETIME,
			ob_jsoncont TEXT NOT NULL,
			ob_classid VARCHAR(20) NOT NULL,
			ob_paylkid VARCHAR(20) NOT NULL,
			ob_paylcont TEXT NOT NULL,
			ob_paylmod VARCHAR(20) NOT NULL);
CREATE TABLE t_names (nam_str PRIMARY KEY ASC NOT NULL UNIQUE,
                      nam_oid VARCHAR(20) NOT NULL UNIQUE);
CREATE TABLE t_modules (mod_oid VARCHAR(20) PRIMARY KEY ASC NOT NULL UNIQUE);
CREATE UNIQUE INDEX x_namedid ON t_names (nam_oid);
COMMIT;
-- basixmo-dump-state end dump basixmo_state.sqlite
