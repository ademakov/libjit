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
	failed: boolean;

procedure run(msg: string; value: boolean);
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
begin
	run("coerce_int_int", SameType(Integer, 3 + 4));
	run("coerce_int_uint", SameType(Integer, 3 + 0FFFFFFFFH));
	run("coerce_uint_uint", SameType(Cardinal, 080000000H + 0FFFFFFFFH));
	run("coerce_int_long", SameType(LongInt, 3 / 07FFFFFFFFFFFH));
	run("coerce_long_int", SameType(LongInt, 07FFFFFFFFFFFH * 3));
end;

begin
	failed := False;
	run_tests;
	if failed then begin
		Terminate(1);
	end;
end.
