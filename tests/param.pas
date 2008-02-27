(*
 * param.pas - Test parameter passing.
 *
 * Copyright (C) 2004  Southern Storm Software, Pty Ltd.
 *
 * This file is part of the libjit library.
 *
 * The libjit library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * The libjit library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the libjit library.  If not, see
 * <http://www.gnu.org/licenses/>.
 *)

program local;

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

{ Test large numbers of 32-bit int parameters, which should stress
  both register and stack parameter passing }
procedure param_int
	(p1,   p2,  p3,  p4,  p5,  p6,  p7,  p8,
	 p9,  p10, p11, p12, p13, p14, p15, p16,
	 p17, p18, p19, p20, p21, p22, p23, p24,
	 p25, p26, p27, p28, p29, p30, p31, p32: integer);
begin
	run("param_int",
	     (p1 = 1)  and  (p2 = 2)  and  (p3 = 3)  and  (p4 = 4) and
		 (p5 = 5)  and  (p6 = 6)  and  (p7 = 7)  and  (p8 = 8) and
		 (p9 = 9)  and (p10 = 10) and (p11 = 11) and (p12 = 12) and
		(p13 = 13) and (p14 = 14) and (p15 = 15) and (p16 = 16) and
		(p17 = 17) and (p18 = 18) and (p19 = 19) and (p20 = 20) and
		(p21 = 21) and (p22 = 22) and (p23 = 23) and (p24 = 24) and
		(p25 = 25) and (p26 = 26) and (p27 = 27) and (p28 = 28) and
		(p29 = 29) and (p30 = 30) and (p31 = 31) and (p32 = 32));
end;
procedure param_int_fastcall
	(p1,   p2,  p3,  p4,  p5,  p6,  p7,  p8,
	 p9,  p10, p11, p12, p13, p14, p15, p16,
	 p17, p18, p19, p20, p21, p22, p23, p24,
	 p25, p26, p27, p28, p29, p30, p31, p32: integer) fastcall;
begin
	run("param_int_fastcall",
	     (p1 = 1)  and  (p2 = 2)  and  (p3 = 3)  and  (p4 = 4) and
		 (p5 = 5)  and  (p6 = 6)  and  (p7 = 7)  and  (p8 = 8) and
		 (p9 = 9)  and (p10 = 10) and (p11 = 11) and (p12 = 12) and
		(p13 = 13) and (p14 = 14) and (p15 = 15) and (p16 = 16) and
		(p17 = 17) and (p18 = 18) and (p19 = 19) and (p20 = 20) and
		(p21 = 21) and (p22 = 22) and (p23 = 23) and (p24 = 24) and
		(p25 = 25) and (p26 = 26) and (p27 = 27) and (p28 = 28) and
		(p29 = 29) and (p30 = 30) and (p31 = 31) and (p32 = 32));
end;
procedure param_int_stdcall
	(p1,   p2,  p3,  p4,  p5,  p6,  p7,  p8,
	 p9,  p10, p11, p12, p13, p14, p15, p16,
	 p17, p18, p19, p20, p21, p22, p23, p24,
	 p25, p26, p27, p28, p29, p30, p31, p32: integer) stdcall;
begin
	run("param_int_stdcall",
	     (p1 = 1)  and  (p2 = 2)  and  (p3 = 3)  and  (p4 = 4) and
		 (p5 = 5)  and  (p6 = 6)  and  (p7 = 7)  and  (p8 = 8) and
		 (p9 = 9)  and (p10 = 10) and (p11 = 11) and (p12 = 12) and
		(p13 = 13) and (p14 = 14) and (p15 = 15) and (p16 = 16) and
		(p17 = 17) and (p18 = 18) and (p19 = 19) and (p20 = 20) and
		(p21 = 21) and (p22 = 22) and (p23 = 23) and (p24 = 24) and
		(p25 = 25) and (p26 = 26) and (p27 = 27) and (p28 = 28) and
		(p29 = 29) and (p30 = 30) and (p31 = 31) and (p32 = 32));
end;

{ Test large numbers of 64-bit int parameters, which should stress
  both register and stack parameter passing.  On a 32-bit system,
  parameters will be passed in pairs.  If the test platform has an
  odd number of word registers, then this will also test long
  splitting on 32-bit systems }
procedure param_long
	(p1,   p2,  p3,  p4,  p5,  p6,  p7,  p8,
	 p9,  p10, p11, p12, p13, p14, p15, p16,
	 p17, p18, p19, p20, p21, p22, p23, p24,
	 p25, p26, p27, p28, p29, p30, p31, p32: longint);
begin
	run("param_long",
	     (p1 = 1)  and  (p2 = 2)  and  (p3 = 3)  and  (p4 = 4) and
		 (p5 = 5)  and  (p6 = 6)  and  (p7 = 7)  and  (p8 = 8) and
		 (p9 = 9)  and (p10 = 10) and (p11 = 11) and (p12 = 12) and
		(p13 = 13) and (p14 = 14) and (p15 = 15) and (p16 = 16) and
		(p17 = 17) and (p18 = 18) and (p19 = 19) and (p20 = 20) and
		(p21 = 21) and (p22 = 22) and (p23 = 23) and (p24 = 24) and
		(p25 = 25) and (p26 = 26) and (p27 = 27) and (p28 = 28) and
		(p29 = 29) and (p30 = 30) and (p31 = 31) and (p32 = 32));
end;
procedure param_long_fastcall
	(p1,   p2,  p3,  p4,  p5,  p6,  p7,  p8,
	 p9,  p10, p11, p12, p13, p14, p15, p16,
	 p17, p18, p19, p20, p21, p22, p23, p24,
	 p25, p26, p27, p28, p29, p30, p31, p32: longint) fastcall;
begin
	run("param_long_fastcall",
	     (p1 = 1)  and  (p2 = 2)  and  (p3 = 3)  and  (p4 = 4) and
		 (p5 = 5)  and  (p6 = 6)  and  (p7 = 7)  and  (p8 = 8) and
		 (p9 = 9)  and (p10 = 10) and (p11 = 11) and (p12 = 12) and
		(p13 = 13) and (p14 = 14) and (p15 = 15) and (p16 = 16) and
		(p17 = 17) and (p18 = 18) and (p19 = 19) and (p20 = 20) and
		(p21 = 21) and (p22 = 22) and (p23 = 23) and (p24 = 24) and
		(p25 = 25) and (p26 = 26) and (p27 = 27) and (p28 = 28) and
		(p29 = 29) and (p30 = 30) and (p31 = 31) and (p32 = 32));
end;
procedure param_long_stdcall
	(p1,   p2,  p3,  p4,  p5,  p6,  p7,  p8,
	 p9,  p10, p11, p12, p13, p14, p15, p16,
	 p17, p18, p19, p20, p21, p22, p23, p24,
	 p25, p26, p27, p28, p29, p30, p31, p32: longint) stdcall;
begin
	run("param_long_stdcall",
	     (p1 = 1)  and  (p2 = 2)  and  (p3 = 3)  and  (p4 = 4) and
		 (p5 = 5)  and  (p6 = 6)  and  (p7 = 7)  and  (p8 = 8) and
		 (p9 = 9)  and (p10 = 10) and (p11 = 11) and (p12 = 12) and
		(p13 = 13) and (p14 = 14) and (p15 = 15) and (p16 = 16) and
		(p17 = 17) and (p18 = 18) and (p19 = 19) and (p20 = 20) and
		(p21 = 21) and (p22 = 22) and (p23 = 23) and (p24 = 24) and
		(p25 = 25) and (p26 = 26) and (p27 = 27) and (p28 = 28) and
		(p29 = 29) and (p30 = 30) and (p31 = 31) and (p32 = 32));
end;

{ Test long splitting on 32-bit systems that have an even
  number of word registers }
procedure param_int_long
	(p1: integer; p2,  p3,  p4,  p5,  p6,  p7,  p8,
	 p9,  p10, p11, p12, p13, p14, p15, p16,
	 p17, p18, p19, p20, p21, p22, p23, p24,
	 p25, p26, p27, p28, p29, p30, p31, p32: longint);
begin
	run("param_int_long",
	     (p1 = 1)  and  (p2 = 2)  and  (p3 = 3)  and  (p4 = 4) and
		 (p5 = 5)  and  (p6 = 6)  and  (p7 = 7)  and  (p8 = 8) and
		 (p9 = 9)  and (p10 = 10) and (p11 = 11) and (p12 = 12) and
		(p13 = 13) and (p14 = 14) and (p15 = 15) and (p16 = 16) and
		(p17 = 17) and (p18 = 18) and (p19 = 19) and (p20 = 20) and
		(p21 = 21) and (p22 = 22) and (p23 = 23) and (p24 = 24) and
		(p25 = 25) and (p26 = 26) and (p27 = 27) and (p28 = 28) and
		(p29 = 29) and (p30 = 30) and (p31 = 31) and (p32 = 32));
end;
procedure param_int_long_fastcall
	(p1: integer; p2,  p3,  p4,  p5,  p6,  p7,  p8,
	 p9,  p10, p11, p12, p13, p14, p15, p16,
	 p17, p18, p19, p20, p21, p22, p23, p24,
	 p25, p26, p27, p28, p29, p30, p31, p32: longint) fastcall;
begin
	run("param_int_long_fastcall",
	     (p1 = 1)  and  (p2 = 2)  and  (p3 = 3)  and  (p4 = 4) and
		 (p5 = 5)  and  (p6 = 6)  and  (p7 = 7)  and  (p8 = 8) and
		 (p9 = 9)  and (p10 = 10) and (p11 = 11) and (p12 = 12) and
		(p13 = 13) and (p14 = 14) and (p15 = 15) and (p16 = 16) and
		(p17 = 17) and (p18 = 18) and (p19 = 19) and (p20 = 20) and
		(p21 = 21) and (p22 = 22) and (p23 = 23) and (p24 = 24) and
		(p25 = 25) and (p26 = 26) and (p27 = 27) and (p28 = 28) and
		(p29 = 29) and (p30 = 30) and (p31 = 31) and (p32 = 32));
end;
procedure param_int_long_stdcall
	(p1: integer; p2,  p3,  p4,  p5,  p6,  p7,  p8,
	 p9,  p10, p11, p12, p13, p14, p15, p16,
	 p17, p18, p19, p20, p21, p22, p23, p24,
	 p25, p26, p27, p28, p29, p30, p31, p32: longint) stdcall;
begin
	run("param_int_long_stdcall",
	     (p1 = 1)  and  (p2 = 2)  and  (p3 = 3)  and  (p4 = 4) and
		 (p5 = 5)  and  (p6 = 6)  and  (p7 = 7)  and  (p8 = 8) and
		 (p9 = 9)  and (p10 = 10) and (p11 = 11) and (p12 = 12) and
		(p13 = 13) and (p14 = 14) and (p15 = 15) and (p16 = 16) and
		(p17 = 17) and (p18 = 18) and (p19 = 19) and (p20 = 20) and
		(p21 = 21) and (p22 = 22) and (p23 = 23) and (p24 = 24) and
		(p25 = 25) and (p26 = 26) and (p27 = 27) and (p28 = 28) and
		(p29 = 29) and (p30 = 30) and (p31 = 31) and (p32 = 32));
end;

{ Test passing 32-bit float values in registers and on the stack }
procedure param_float32
	(p1,   p2,  p3,  p4,  p5,  p6,  p7,  p8,
	 p9,  p10, p11, p12, p13, p14, p15, p16,
	 p17, p18, p19, p20, p21, p22, p23, p24,
	 p25, p26, p27, p28, p29, p30, p31, p32: shortreal);
begin
	run("param_float32",
	     (p1 = 1)  and  (p2 = 2)  and  (p3 = 3)  and  (p4 = 4) and
		 (p5 = 5)  and  (p6 = 6)  and  (p7 = 7)  and  (p8 = 8) and
		 (p9 = 9)  and (p10 = 10) and (p11 = 11) and (p12 = 12) and
		(p13 = 13) and (p14 = 14) and (p15 = 15) and (p16 = 16) and
		(p17 = 17) and (p18 = 18) and (p19 = 19) and (p20 = 20) and
		(p21 = 21) and (p22 = 22) and (p23 = 23) and (p24 = 24) and
		(p25 = 25) and (p26 = 26) and (p27 = 27) and (p28 = 28) and
		(p29 = 29) and (p30 = 30) and (p31 = 31) and (p32 = 32));
end;
procedure param_float32_fastcall
	(p1,   p2,  p3,  p4,  p5,  p6,  p7,  p8,
	 p9,  p10, p11, p12, p13, p14, p15, p16,
	 p17, p18, p19, p20, p21, p22, p23, p24,
	 p25, p26, p27, p28, p29, p30, p31, p32: shortreal) fastcall;
begin
	run("param_float32_fastcall",
	     (p1 = 1)  and  (p2 = 2)  and  (p3 = 3)  and  (p4 = 4) and
		 (p5 = 5)  and  (p6 = 6)  and  (p7 = 7)  and  (p8 = 8) and
		 (p9 = 9)  and (p10 = 10) and (p11 = 11) and (p12 = 12) and
		(p13 = 13) and (p14 = 14) and (p15 = 15) and (p16 = 16) and
		(p17 = 17) and (p18 = 18) and (p19 = 19) and (p20 = 20) and
		(p21 = 21) and (p22 = 22) and (p23 = 23) and (p24 = 24) and
		(p25 = 25) and (p26 = 26) and (p27 = 27) and (p28 = 28) and
		(p29 = 29) and (p30 = 30) and (p31 = 31) and (p32 = 32));
end;
procedure param_float32_stdcall
	(p1,   p2,  p3,  p4,  p5,  p6,  p7,  p8,
	 p9,  p10, p11, p12, p13, p14, p15, p16,
	 p17, p18, p19, p20, p21, p22, p23, p24,
	 p25, p26, p27, p28, p29, p30, p31, p32: shortreal) stdcall;
begin
	run("param_float32_stdcall",
	     (p1 = 1)  and  (p2 = 2)  and  (p3 = 3)  and  (p4 = 4) and
		 (p5 = 5)  and  (p6 = 6)  and  (p7 = 7)  and  (p8 = 8) and
		 (p9 = 9)  and (p10 = 10) and (p11 = 11) and (p12 = 12) and
		(p13 = 13) and (p14 = 14) and (p15 = 15) and (p16 = 16) and
		(p17 = 17) and (p18 = 18) and (p19 = 19) and (p20 = 20) and
		(p21 = 21) and (p22 = 22) and (p23 = 23) and (p24 = 24) and
		(p25 = 25) and (p26 = 26) and (p27 = 27) and (p28 = 28) and
		(p29 = 29) and (p30 = 30) and (p31 = 31) and (p32 = 32));
end;

{ Test passing 64-bit float values in registers and on the stack }
procedure param_float64
	(p1,   p2,  p3,  p4,  p5,  p6,  p7,  p8,
	 p9,  p10, p11, p12, p13, p14, p15, p16,
	 p17, p18, p19, p20, p21, p22, p23, p24,
	 p25, p26, p27, p28, p29, p30, p31, p32: real);
begin
	run("param_float64",
	     (p1 = 1)  and  (p2 = 2)  and  (p3 = 3)  and  (p4 = 4) and
		 (p5 = 5)  and  (p6 = 6)  and  (p7 = 7)  and  (p8 = 8) and
		 (p9 = 9)  and (p10 = 10) and (p11 = 11) and (p12 = 12) and
		(p13 = 13) and (p14 = 14) and (p15 = 15) and (p16 = 16) and
		(p17 = 17) and (p18 = 18) and (p19 = 19) and (p20 = 20) and
		(p21 = 21) and (p22 = 22) and (p23 = 23) and (p24 = 24) and
		(p25 = 25) and (p26 = 26) and (p27 = 27) and (p28 = 28) and
		(p29 = 29) and (p30 = 30) and (p31 = 31) and (p32 = 32));
end;
procedure param_float64_fastcall
	(p1,   p2,  p3,  p4,  p5,  p6,  p7,  p8,
	 p9,  p10, p11, p12, p13, p14, p15, p16,
	 p17, p18, p19, p20, p21, p22, p23, p24,
	 p25, p26, p27, p28, p29, p30, p31, p32: real) fastcall;
begin
	run("param_float64_fastcall",
	     (p1 = 1)  and  (p2 = 2)  and  (p3 = 3)  and  (p4 = 4) and
		 (p5 = 5)  and  (p6 = 6)  and  (p7 = 7)  and  (p8 = 8) and
		 (p9 = 9)  and (p10 = 10) and (p11 = 11) and (p12 = 12) and
		(p13 = 13) and (p14 = 14) and (p15 = 15) and (p16 = 16) and
		(p17 = 17) and (p18 = 18) and (p19 = 19) and (p20 = 20) and
		(p21 = 21) and (p22 = 22) and (p23 = 23) and (p24 = 24) and
		(p25 = 25) and (p26 = 26) and (p27 = 27) and (p28 = 28) and
		(p29 = 29) and (p30 = 30) and (p31 = 31) and (p32 = 32));
end;
procedure param_float64_stdcall
	(p1,   p2,  p3,  p4,  p5,  p6,  p7,  p8,
	 p9,  p10, p11, p12, p13, p14, p15, p16,
	 p17, p18, p19, p20, p21, p22, p23, p24,
	 p25, p26, p27, p28, p29, p30, p31, p32: real) stdcall;
begin
	run("param_float64_stdcall",
	     (p1 = 1)  and  (p2 = 2)  and  (p3 = 3)  and  (p4 = 4) and
		 (p5 = 5)  and  (p6 = 6)  and  (p7 = 7)  and  (p8 = 8) and
		 (p9 = 9)  and (p10 = 10) and (p11 = 11) and (p12 = 12) and
		(p13 = 13) and (p14 = 14) and (p15 = 15) and (p16 = 16) and
		(p17 = 17) and (p18 = 18) and (p19 = 19) and (p20 = 20) and
		(p21 = 21) and (p22 = 22) and (p23 = 23) and (p24 = 24) and
		(p25 = 25) and (p26 = 26) and (p27 = 27) and (p28 = 28) and
		(p29 = 29) and (p30 = 30) and (p31 = 31) and (p32 = 32));
end;

{ Test passing native float values in registers and on the stack }
procedure param_nfloat
	(p1,   p2,  p3,  p4,  p5,  p6,  p7,  p8,
	 p9,  p10, p11, p12, p13, p14, p15, p16,
	 p17, p18, p19, p20, p21, p22, p23, p24,
	 p25, p26, p27, p28, p29, p30, p31, p32: longreal);
begin
	run("param_nfloat",
	     (p1 = 1)  and  (p2 = 2)  and  (p3 = 3)  and  (p4 = 4) and
		 (p5 = 5)  and  (p6 = 6)  and  (p7 = 7)  and  (p8 = 8) and
		 (p9 = 9)  and (p10 = 10) and (p11 = 11) and (p12 = 12) and
		(p13 = 13) and (p14 = 14) and (p15 = 15) and (p16 = 16) and
		(p17 = 17) and (p18 = 18) and (p19 = 19) and (p20 = 20) and
		(p21 = 21) and (p22 = 22) and (p23 = 23) and (p24 = 24) and
		(p25 = 25) and (p26 = 26) and (p27 = 27) and (p28 = 28) and
		(p29 = 29) and (p30 = 30) and (p31 = 31) and (p32 = 32));
end;
procedure param_nfloat_fastcall
	(p1,   p2,  p3,  p4,  p5,  p6,  p7,  p8,
	 p9,  p10, p11, p12, p13, p14, p15, p16,
	 p17, p18, p19, p20, p21, p22, p23, p24,
	 p25, p26, p27, p28, p29, p30, p31, p32: longreal) fastcall;
begin
	run("param_nfloat_fastcall",
	     (p1 = 1)  and  (p2 = 2)  and  (p3 = 3)  and  (p4 = 4) and
		 (p5 = 5)  and  (p6 = 6)  and  (p7 = 7)  and  (p8 = 8) and
		 (p9 = 9)  and (p10 = 10) and (p11 = 11) and (p12 = 12) and
		(p13 = 13) and (p14 = 14) and (p15 = 15) and (p16 = 16) and
		(p17 = 17) and (p18 = 18) and (p19 = 19) and (p20 = 20) and
		(p21 = 21) and (p22 = 22) and (p23 = 23) and (p24 = 24) and
		(p25 = 25) and (p26 = 26) and (p27 = 27) and (p28 = 28) and
		(p29 = 29) and (p30 = 30) and (p31 = 31) and (p32 = 32));
end;
procedure param_nfloat_stdcall
	(p1,   p2,  p3,  p4,  p5,  p6,  p7,  p8,
	 p9,  p10, p11, p12, p13, p14, p15, p16,
	 p17, p18, p19, p20, p21, p22, p23, p24,
	 p25, p26, p27, p28, p29, p30, p31, p32: longreal) stdcall;
begin
	run("param_nfloat_stdcall",
	     (p1 = 1)  and  (p2 = 2)  and  (p3 = 3)  and  (p4 = 4) and
		 (p5 = 5)  and  (p6 = 6)  and  (p7 = 7)  and  (p8 = 8) and
		 (p9 = 9)  and (p10 = 10) and (p11 = 11) and (p12 = 12) and
		(p13 = 13) and (p14 = 14) and (p15 = 15) and (p16 = 16) and
		(p17 = 17) and (p18 = 18) and (p19 = 19) and (p20 = 20) and
		(p21 = 21) and (p22 = 22) and (p23 = 23) and (p24 = 24) and
		(p25 = 25) and (p26 = 26) and (p27 = 27) and (p28 = 28) and
		(p29 = 29) and (p30 = 30) and (p31 = 31) and (p32 = 32));
end;

{ Test passing both int and float64 parameters }
procedure param_int_float64
	(p1,   p2,  p3,  p4,  p5,  p6,  p7,  p8,
	 p9,  p10, p11, p12, p13, p14, p15, p16,
	 p17, p18, p19, p20, p21, p22, p23, p24,
	 p25, p26, p27, p28, p29, p30, p31, p32: integer;
	 q1,   q2,  q3,  q4,  q5,  q6,  q7,  q8,
	 q9,  q10, q11, q12, q13, q14, q15, q16,
	 q17, q18, q19, q20, q21, q22, q23, q24,
	 q25, q26, q27, q28, q29, q30, q31, q32: real);
begin
	run("param_int_float64",
	     (p1 = 1)  and  (p2 = 2)  and  (p3 = 3)  and  (p4 = 4) and
		 (p5 = 5)  and  (p6 = 6)  and  (p7 = 7)  and  (p8 = 8) and
		 (p9 = 9)  and (p10 = 10) and (p11 = 11) and (p12 = 12) and
		(p13 = 13) and (p14 = 14) and (p15 = 15) and (p16 = 16) and
		(p17 = 17) and (p18 = 18) and (p19 = 19) and (p20 = 20) and
		(p21 = 21) and (p22 = 22) and (p23 = 23) and (p24 = 24) and
		(p25 = 25) and (p26 = 26) and (p27 = 27) and (p28 = 28) and
		(p29 = 29) and (p30 = 30) and (p31 = 31) and (p32 = 32) and
	     (q1 = 1)  and  (q2 = 2)  and  (q3 = 3)  and  (q4 = 4) and
		 (q5 = 5)  and  (q6 = 6)  and  (q7 = 7)  and  (q8 = 8) and
		 (q9 = 9)  and (q10 = 10) and (q11 = 11) and (q12 = 12) and
		(q13 = 13) and (q14 = 14) and (q15 = 15) and (q16 = 16) and
		(q17 = 17) and (q18 = 18) and (q19 = 19) and (q20 = 20) and
		(q21 = 21) and (q22 = 22) and (q23 = 23) and (q24 = 24) and
		(q25 = 25) and (q26 = 26) and (q27 = 27) and (q28 = 28) and
		(q29 = 29) and (q30 = 30) and (q31 = 31) and (q32 = 32));
end;
procedure param_int_float64_fastcall
	(p1,   p2,  p3,  p4,  p5,  p6,  p7,  p8,
	 p9,  p10, p11, p12, p13, p14, p15, p16,
	 p17, p18, p19, p20, p21, p22, p23, p24,
	 p25, p26, p27, p28, p29, p30, p31, p32: integer;
	 q1,   q2,  q3,  q4,  q5,  q6,  q7,  q8,
	 q9,  q10, q11, q12, q13, q14, q15, q16,
	 q17, q18, q19, q20, q21, q22, q23, q24,
	 q25, q26, q27, q28, q29, q30, q31, q32: real) fastcall;
begin
	run("param_int_float64_fastcall",
	     (p1 = 1)  and  (p2 = 2)  and  (p3 = 3)  and  (p4 = 4) and
		 (p5 = 5)  and  (p6 = 6)  and  (p7 = 7)  and  (p8 = 8) and
		 (p9 = 9)  and (p10 = 10) and (p11 = 11) and (p12 = 12) and
		(p13 = 13) and (p14 = 14) and (p15 = 15) and (p16 = 16) and
		(p17 = 17) and (p18 = 18) and (p19 = 19) and (p20 = 20) and
		(p21 = 21) and (p22 = 22) and (p23 = 23) and (p24 = 24) and
		(p25 = 25) and (p26 = 26) and (p27 = 27) and (p28 = 28) and
		(p29 = 29) and (p30 = 30) and (p31 = 31) and (p32 = 32) and
	     (q1 = 1)  and  (q2 = 2)  and  (q3 = 3)  and  (q4 = 4) and
		 (q5 = 5)  and  (q6 = 6)  and  (q7 = 7)  and  (q8 = 8) and
		 (q9 = 9)  and (q10 = 10) and (q11 = 11) and (q12 = 12) and
		(q13 = 13) and (q14 = 14) and (q15 = 15) and (q16 = 16) and
		(q17 = 17) and (q18 = 18) and (q19 = 19) and (q20 = 20) and
		(q21 = 21) and (q22 = 22) and (q23 = 23) and (q24 = 24) and
		(q25 = 25) and (q26 = 26) and (q27 = 27) and (q28 = 28) and
		(q29 = 29) and (q30 = 30) and (q31 = 31) and (q32 = 32));
end;
procedure param_int_float64_stdcall
	(p1,   p2,  p3,  p4,  p5,  p6,  p7,  p8,
	 p9,  p10, p11, p12, p13, p14, p15, p16,
	 p17, p18, p19, p20, p21, p22, p23, p24,
	 p25, p26, p27, p28, p29, p30, p31, p32: integer;
	 q1,   q2,  q3,  q4,  q5,  q6,  q7,  q8,
	 q9,  q10, q11, q12, q13, q14, q15, q16,
	 q17, q18, q19, q20, q21, q22, q23, q24,
	 q25, q26, q27, q28, q29, q30, q31, q32: real) stdcall;
begin
	run("param_int_float64_stdcall",
	     (p1 = 1)  and  (p2 = 2)  and  (p3 = 3)  and  (p4 = 4) and
		 (p5 = 5)  and  (p6 = 6)  and  (p7 = 7)  and  (p8 = 8) and
		 (p9 = 9)  and (p10 = 10) and (p11 = 11) and (p12 = 12) and
		(p13 = 13) and (p14 = 14) and (p15 = 15) and (p16 = 16) and
		(p17 = 17) and (p18 = 18) and (p19 = 19) and (p20 = 20) and
		(p21 = 21) and (p22 = 22) and (p23 = 23) and (p24 = 24) and
		(p25 = 25) and (p26 = 26) and (p27 = 27) and (p28 = 28) and
		(p29 = 29) and (p30 = 30) and (p31 = 31) and (p32 = 32) and
	     (q1 = 1)  and  (q2 = 2)  and  (q3 = 3)  and  (q4 = 4) and
		 (q5 = 5)  and  (q6 = 6)  and  (q7 = 7)  and  (q8 = 8) and
		 (q9 = 9)  and (q10 = 10) and (q11 = 11) and (q12 = 12) and
		(q13 = 13) and (q14 = 14) and (q15 = 15) and (q16 = 16) and
		(q17 = 17) and (q18 = 18) and (q19 = 19) and (q20 = 20) and
		(q21 = 21) and (q22 = 22) and (q23 = 23) and (q24 = 24) and
		(q25 = 25) and (q26 = 26) and (q27 = 27) and (q28 = 28) and
		(q29 = 29) and (q30 = 30) and (q31 = 31) and (q32 = 32));
end;

{ Test passing both int and float64 parameters, reversed from above }
procedure param_float64_int
	(p1,   p2,  p3,  p4,  p5,  p6,  p7,  p8,
	 p9,  p10, p11, p12, p13, p14, p15, p16,
	 p17, p18, p19, p20, p21, p22, p23, p24,
	 p25, p26, p27, p28, p29, p30, p31, p32: real;
	 q1,   q2,  q3,  q4,  q5,  q6,  q7,  q8,
	 q9,  q10, q11, q12, q13, q14, q15, q16,
	 q17, q18, q19, q20, q21, q22, q23, q24,
	 q25, q26, q27, q28, q29, q30, q31, q32: integer);
begin
	run("param_float64_int",
	     (p1 = 1)  and  (p2 = 2)  and  (p3 = 3)  and  (p4 = 4) and
		 (p5 = 5)  and  (p6 = 6)  and  (p7 = 7)  and  (p8 = 8) and
		 (p9 = 9)  and (p10 = 10) and (p11 = 11) and (p12 = 12) and
		(p13 = 13) and (p14 = 14) and (p15 = 15) and (p16 = 16) and
		(p17 = 17) and (p18 = 18) and (p19 = 19) and (p20 = 20) and
		(p21 = 21) and (p22 = 22) and (p23 = 23) and (p24 = 24) and
		(p25 = 25) and (p26 = 26) and (p27 = 27) and (p28 = 28) and
		(p29 = 29) and (p30 = 30) and (p31 = 31) and (p32 = 32) and
	     (q1 = 1)  and  (q2 = 2)  and  (q3 = 3)  and  (q4 = 4) and
		 (q5 = 5)  and  (q6 = 6)  and  (q7 = 7)  and  (q8 = 8) and
		 (q9 = 9)  and (q10 = 10) and (q11 = 11) and (q12 = 12) and
		(q13 = 13) and (q14 = 14) and (q15 = 15) and (q16 = 16) and
		(q17 = 17) and (q18 = 18) and (q19 = 19) and (q20 = 20) and
		(q21 = 21) and (q22 = 22) and (q23 = 23) and (q24 = 24) and
		(q25 = 25) and (q26 = 26) and (q27 = 27) and (q28 = 28) and
		(q29 = 29) and (q30 = 30) and (q31 = 31) and (q32 = 32));
end;
procedure param_float64_int_fastcall
	(p1,   p2,  p3,  p4,  p5,  p6,  p7,  p8,
	 p9,  p10, p11, p12, p13, p14, p15, p16,
	 p17, p18, p19, p20, p21, p22, p23, p24,
	 p25, p26, p27, p28, p29, p30, p31, p32: real;
	 q1,   q2,  q3,  q4,  q5,  q6,  q7,  q8,
	 q9,  q10, q11, q12, q13, q14, q15, q16,
	 q17, q18, q19, q20, q21, q22, q23, q24,
	 q25, q26, q27, q28, q29, q30, q31, q32: integer) fastcall;
begin
	run("param_float64_int_fastcall",
	     (p1 = 1)  and  (p2 = 2)  and  (p3 = 3)  and  (p4 = 4) and
		 (p5 = 5)  and  (p6 = 6)  and  (p7 = 7)  and  (p8 = 8) and
		 (p9 = 9)  and (p10 = 10) and (p11 = 11) and (p12 = 12) and
		(p13 = 13) and (p14 = 14) and (p15 = 15) and (p16 = 16) and
		(p17 = 17) and (p18 = 18) and (p19 = 19) and (p20 = 20) and
		(p21 = 21) and (p22 = 22) and (p23 = 23) and (p24 = 24) and
		(p25 = 25) and (p26 = 26) and (p27 = 27) and (p28 = 28) and
		(p29 = 29) and (p30 = 30) and (p31 = 31) and (p32 = 32) and
	     (q1 = 1)  and  (q2 = 2)  and  (q3 = 3)  and  (q4 = 4) and
		 (q5 = 5)  and  (q6 = 6)  and  (q7 = 7)  and  (q8 = 8) and
		 (q9 = 9)  and (q10 = 10) and (q11 = 11) and (q12 = 12) and
		(q13 = 13) and (q14 = 14) and (q15 = 15) and (q16 = 16) and
		(q17 = 17) and (q18 = 18) and (q19 = 19) and (q20 = 20) and
		(q21 = 21) and (q22 = 22) and (q23 = 23) and (q24 = 24) and
		(q25 = 25) and (q26 = 26) and (q27 = 27) and (q28 = 28) and
		(q29 = 29) and (q30 = 30) and (q31 = 31) and (q32 = 32));
end;
procedure param_float64_int_stdcall
	(p1,   p2,  p3,  p4,  p5,  p6,  p7,  p8,
	 p9,  p10, p11, p12, p13, p14, p15, p16,
	 p17, p18, p19, p20, p21, p22, p23, p24,
	 p25, p26, p27, p28, p29, p30, p31, p32: real;
	 q1,   q2,  q3,  q4,  q5,  q6,  q7,  q8,
	 q9,  q10, q11, q12, q13, q14, q15, q16,
	 q17, q18, q19, q20, q21, q22, q23, q24,
	 q25, q26, q27, q28, q29, q30, q31, q32: integer) stdcall;
begin
	run("param_float64_int_stdcall",
	     (p1 = 1)  and  (p2 = 2)  and  (p3 = 3)  and  (p4 = 4) and
		 (p5 = 5)  and  (p6 = 6)  and  (p7 = 7)  and  (p8 = 8) and
		 (p9 = 9)  and (p10 = 10) and (p11 = 11) and (p12 = 12) and
		(p13 = 13) and (p14 = 14) and (p15 = 15) and (p16 = 16) and
		(p17 = 17) and (p18 = 18) and (p19 = 19) and (p20 = 20) and
		(p21 = 21) and (p22 = 22) and (p23 = 23) and (p24 = 24) and
		(p25 = 25) and (p26 = 26) and (p27 = 27) and (p28 = 28) and
		(p29 = 29) and (p30 = 30) and (p31 = 31) and (p32 = 32) and
	     (q1 = 1)  and  (q2 = 2)  and  (q3 = 3)  and  (q4 = 4) and
		 (q5 = 5)  and  (q6 = 6)  and  (q7 = 7)  and  (q8 = 8) and
		 (q9 = 9)  and (q10 = 10) and (q11 = 11) and (q12 = 12) and
		(q13 = 13) and (q14 = 14) and (q15 = 15) and (q16 = 16) and
		(q17 = 17) and (q18 = 18) and (q19 = 19) and (q20 = 20) and
		(q21 = 21) and (q22 = 22) and (q23 = 23) and (q24 = 24) and
		(q25 = 25) and (q26 = 26) and (q27 = 27) and (q28 = 28) and
		(q29 = 29) and (q30 = 30) and (q31 = 31) and (q32 = 32));
end;

{ Test passing both int and float 64 parameters, alternatively mixed }
procedure param_int_float64_mixed
	( p1: integer;  q1: real;  p2: integer;  q2: real;
	  p3: integer;  q3: real;  p4: integer;  q4: real;
	  p5: integer;  q5: real;  p6: integer;  q6: real;
	  p7: integer;  q7: real;  p8: integer;  q8: real;
	  p9: integer;  q9: real; p10: integer; q10: real;
	 p11: integer; q11: real; p12: integer; q12: real;
	 p13: integer; q13: real; p14: integer; q14: real;
	 p15: integer; q15: real; p16: integer; q16: real;
	 p17: integer; q17: real; p18: integer; q18: real;
	 p19: integer; q19: real; p20: integer; q20: real;
	 p21: integer; q21: real; p22: integer; q22: real;
	 p23: integer; q23: real; p24: integer; q24: real;
	 p25: integer; q25: real; p26: integer; q26: real;
	 p27: integer; q27: real; p28: integer; q28: real;
	 p29: integer; q29: real; p30: integer; q30: real;
	 p31: integer; q31: real; p32: integer; q32: real);
begin
	run("param_int_float64_mixed",
		 (p1 =  1) and  (q1 =  2) and  (p2 =  3) and  (q2 =  4) and
		 (p3 =  5) and  (q3 =  6) and  (p4 =  7) and  (q4 =  8) and
		 (p5 =  9) and  (q5 = 10) and  (p6 = 11) and  (q6 = 12) and
		 (p7 = 13) and  (q7 = 14) and  (p8 = 15) and  (q8 = 16) and
		 (p9 = 17) and  (q9 = 18) and (p10 = 19) and (q10 = 20) and
		(p11 = 21) and (q11 = 22) and (p12 = 23) and (q12 = 24) and
		(p13 = 25) and (q13 = 26) and (p14 = 27) and (q14 = 28) and
		(p15 = 29) and (q15 = 30) and (p16 = 31) and (q16 = 32) and
		(p17 =  1) and (q17 =  2) and (p18 =  3) and (q18 =  4) and
		(p19 =  5) and (q19 =  6) and (p20 =  7) and (q20 =  8) and
		(p21 =  9) and (q21 = 10) and (p22 = 11) and (q22 = 12) and
		(p23 = 13) and (q23 = 14) and (p24 = 15) and (q24 = 16) and
		(p25 = 17) and (q25 = 18) and (p26 = 19) and (q26 = 20) and
		(p27 = 21) and (q27 = 22) and (p28 = 23) and (q28 = 24) and
		(p29 = 25) and (q29 = 26) and (p30 = 27) and (q30 = 28) and
		(p31 = 29) and (q31 = 30) and (p32 = 31) and (q32 = 32));
end;
procedure param_int_float64_mixed_fastcall
	( p1: integer;  q1: real;  p2: integer;  q2: real;
	  p3: integer;  q3: real;  p4: integer;  q4: real;
	  p5: integer;  q5: real;  p6: integer;  q6: real;
	  p7: integer;  q7: real;  p8: integer;  q8: real;
	  p9: integer;  q9: real; p10: integer; q10: real;
	 p11: integer; q11: real; p12: integer; q12: real;
	 p13: integer; q13: real; p14: integer; q14: real;
	 p15: integer; q15: real; p16: integer; q16: real;
	 p17: integer; q17: real; p18: integer; q18: real;
	 p19: integer; q19: real; p20: integer; q20: real;
	 p21: integer; q21: real; p22: integer; q22: real;
	 p23: integer; q23: real; p24: integer; q24: real;
	 p25: integer; q25: real; p26: integer; q26: real;
	 p27: integer; q27: real; p28: integer; q28: real;
	 p29: integer; q29: real; p30: integer; q30: real;
	 p31: integer; q31: real; p32: integer; q32: real) fastcall;
begin
	run("param_int_float64_mixed_fastcall",
		 (p1 =  1) and  (q1 =  2) and  (p2 =  3) and  (q2 =  4) and
		 (p3 =  5) and  (q3 =  6) and  (p4 =  7) and  (q4 =  8) and
		 (p5 =  9) and  (q5 = 10) and  (p6 = 11) and  (q6 = 12) and
		 (p7 = 13) and  (q7 = 14) and  (p8 = 15) and  (q8 = 16) and
		 (p9 = 17) and  (q9 = 18) and (p10 = 19) and (q10 = 20) and
		(p11 = 21) and (q11 = 22) and (p12 = 23) and (q12 = 24) and
		(p13 = 25) and (q13 = 26) and (p14 = 27) and (q14 = 28) and
		(p15 = 29) and (q15 = 30) and (p16 = 31) and (q16 = 32) and
		(p17 =  1) and (q17 =  2) and (p18 =  3) and (q18 =  4) and
		(p19 =  5) and (q19 =  6) and (p20 =  7) and (q20 =  8) and
		(p21 =  9) and (q21 = 10) and (p22 = 11) and (q22 = 12) and
		(p23 = 13) and (q23 = 14) and (p24 = 15) and (q24 = 16) and
		(p25 = 17) and (q25 = 18) and (p26 = 19) and (q26 = 20) and
		(p27 = 21) and (q27 = 22) and (p28 = 23) and (q28 = 24) and
		(p29 = 25) and (q29 = 26) and (p30 = 27) and (q30 = 28) and
		(p31 = 29) and (q31 = 30) and (p32 = 31) and (q32 = 32));
end;
procedure param_int_float64_mixed_stdcall
	( p1: integer;  q1: real;  p2: integer;  q2: real;
	  p3: integer;  q3: real;  p4: integer;  q4: real;
	  p5: integer;  q5: real;  p6: integer;  q6: real;
	  p7: integer;  q7: real;  p8: integer;  q8: real;
	  p9: integer;  q9: real; p10: integer; q10: real;
	 p11: integer; q11: real; p12: integer; q12: real;
	 p13: integer; q13: real; p14: integer; q14: real;
	 p15: integer; q15: real; p16: integer; q16: real;
	 p17: integer; q17: real; p18: integer; q18: real;
	 p19: integer; q19: real; p20: integer; q20: real;
	 p21: integer; q21: real; p22: integer; q22: real;
	 p23: integer; q23: real; p24: integer; q24: real;
	 p25: integer; q25: real; p26: integer; q26: real;
	 p27: integer; q27: real; p28: integer; q28: real;
	 p29: integer; q29: real; p30: integer; q30: real;
	 p31: integer; q31: real; p32: integer; q32: real) stdcall;
begin
	run("param_int_float64_mixed_stdcall",
		 (p1 =  1) and  (q1 =  2) and  (p2 =  3) and  (q2 =  4) and
		 (p3 =  5) and  (q3 =  6) and  (p4 =  7) and  (q4 =  8) and
		 (p5 =  9) and  (q5 = 10) and  (p6 = 11) and  (q6 = 12) and
		 (p7 = 13) and  (q7 = 14) and  (p8 = 15) and  (q8 = 16) and
		 (p9 = 17) and  (q9 = 18) and (p10 = 19) and (q10 = 20) and
		(p11 = 21) and (q11 = 22) and (p12 = 23) and (q12 = 24) and
		(p13 = 25) and (q13 = 26) and (p14 = 27) and (q14 = 28) and
		(p15 = 29) and (q15 = 30) and (p16 = 31) and (q16 = 32) and
		(p17 =  1) and (q17 =  2) and (p18 =  3) and (q18 =  4) and
		(p19 =  5) and (q19 =  6) and (p20 =  7) and (q20 =  8) and
		(p21 =  9) and (q21 = 10) and (p22 = 11) and (q22 = 12) and
		(p23 = 13) and (q23 = 14) and (p24 = 15) and (q24 = 16) and
		(p25 = 17) and (q25 = 18) and (p26 = 19) and (q26 = 20) and
		(p27 = 21) and (q27 = 22) and (p28 = 23) and (q28 = 24) and
		(p29 = 25) and (q29 = 26) and (p30 = 27) and (q30 = 28) and
		(p31 = 29) and (q31 = 30) and (p32 = 31) and (q32 = 32));
end;

procedure run_tests;
begin
	param_int
		(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32);
	param_int_fastcall
		(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32);
	param_int_stdcall
		(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32);

	param_long
		(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32);
	param_long_fastcall
		(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32);
	param_long_stdcall
		(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32);

	param_int_long
		(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32);
	param_int_long_fastcall
		(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32);
	param_int_long_stdcall
		(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32);

	param_float32
		(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32);
	param_float32_fastcall
		(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32);
	param_float32_stdcall
		(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32);

	param_float64
		(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32);
	param_float64_fastcall
		(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32);
	param_float64_stdcall
		(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32);

	param_nfloat
		(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32);
	param_nfloat_fastcall
		(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32);
	param_nfloat_stdcall
		(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32);

	param_int_float64
		(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
		 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32);
	param_int_float64_fastcall
		(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
		 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32);
	param_int_float64_stdcall
		(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
		 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32);

	param_float64_int
		(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
		 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32);
	param_float64_int_fastcall
		(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
		 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32);
	param_float64_int_stdcall
		(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
		 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32);

	param_int_float64_mixed
		(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
		 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32);
	param_int_float64_mixed_fastcall
		(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
		 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32);
	param_int_float64_mixed_stdcall
		(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
		 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16,
		 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32);
end;

begin
	failed := False;
	run_tests;
	if failed then begin
		Terminate(1);
	end;
end.
