// file value.cc - value functions

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


bool BxoVal::less(const BxoVal&r) const
{
  if (_kind < r._kind) return true;
  if (_kind > r._kind) return false;
  switch (_kind)
    {
    case BxoVKind::NoneK:
      return false;
    case BxoVKind::IntK:
      return _int < r._int;
    case BxoVKind::StringK:
      return _str->less_than_string(*(r._str));
    case BxoVKind::ObjectK:
      return _obj->less(*(r._obj));
    case BxoVKind::SetK:
      return _set->less_than_set(*(r._set));
    case BxoVKind::TupleK:
      return _tup->less_than_tuple(*(r._tup));
    }
  return false;
} // end BxoVal::less

bool BxoVal::less_equal(const BxoVal&r) const
{
  if (_kind < r._kind) return true;
  if (_kind > r._kind) return false;
  switch (_kind)
    {
    case BxoVKind::NoneK:
      return true;
    case BxoVKind::IntK:
      return _int <= r._int;
    case BxoVKind::StringK:
      return _str->less_equal_string(*r._str);
    case BxoVKind::ObjectK:
      return _obj->less_equal(*r._obj);
    case BxoVKind::SetK:
      return _set->less_equal_set(*r._set);
      break;
    case BxoVKind::TupleK:
      return _tup->less_equal_tuple(*r._tup);
    }
  return false;
} // end BxoVal::less_equal
