(*
 * coerce.pas - Test type coercion of the primitive operators.
 *
 * Copyright (C) 2004  Southern Storm Software, Pty Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *)

program coerce;

var
	failed: Boolean;

procedure run(msg: String; value: Boolean);
begin
	Write(msg);
	Write(" ... ");
	if value then begin
		WriteLn("ok");
	end else begin
		WriteLn("failed");
		failed := True;
	end;
end;

procedure run_tests;
var
	b: Byte;
	s: ShortInt;
begin
	b := 3;
	s := 67;
	run("coerce_byte_short", SameType(Integer, b / s));
	run("coerce_int_byte", SameType(Integer, 3 + b));
	run("coerce_byte_uint", SameType(Cardinal, b * 080000000H));
	run("coerce_int_short", SameType(Integer, 3 + s));
	run("coerce_short_uint", SameType(Integer, s * 080000000H));
	run("coerce_int_int", SameType(Integer, 3 + 4));
	run("coerce_int_uint", SameType(Integer, 3 - 0FFFFFFFFH));
	run("coerce_uint_int", SameType(Integer, 0FFFFFFFFH mod 3));
	run("coerce_uint_uint", SameType(Cardinal, 080000000H + 0FFFFFFFFH));
	run("coerce_int_long", SameType(LongInt, 3 / 07FFFFFFFFFFFH));
	run("coerce_long_int", SameType(LongInt, 07FFFFFFFFFFFH * 3));
	run("coerce_long_uint", SameType(LongInt, 07FFFFFFFFFFFH * 0FFFFFFFFH));
	run("coerce_uint_ulong",
		SameType(LongCard, 0FFFFFFFFH + 08000000000000000H));
end;

begin
	failed := False;
	run_tests;
	if failed then begin
		Terminate(1);
	end;
end.
