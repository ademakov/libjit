(*
 * math.pas - Test mathematical operators.
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

const
	pi = 3.14159265358979323846;

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

procedure runf(msg: String; value, expected, delta: ShortReal);
begin
	run(msg, Abs(value - expected) <= delta);
end;

procedure rund(msg: String; value, expected, delta: Real);
begin
	run(msg, Abs(value - expected) <= delta);
end;

procedure runn(msg: String; value, expected, delta: LongReal);
begin
	run(msg, Abs(value - expected) <= delta);
end;

procedure run_tests;
var
	b: Byte;
	s: ShortInt;
	f: ShortReal;
	d: Real;
	n: LongReal;
begin
	{ Initialize some values of odd type sizes }
	b := 3;
	s := 67;
	f := 24.0;
	d := 56.0;
	n := 123.5;

	{ Test coercion rules for mathematical operators }
	run("math_coerce_byte", SameType(LongReal, Sin(b)));
	run("math_coerce_short", SameType(LongReal, Cos(s)));
	run("math_coerce_int", SameType(LongReal, 3 pow 5));
	run("math_coerce_uint", SameType(LongReal, Atan2(3, 080000005H)));
	run("math_coerce_long", SameType(LongReal, Round(0100000000H)));
	run("math_coerce_ulong", SameType(LongReal, Log(08000000000000000H)));
	run("math_coerce_float32", SameType(ShortReal, Log10(f)));
	run("math_coerce_float64", SameType(Real, Atan(d)));
	run("math_coerce_nfloat", SameType(LongReal, Abs(n)));

	{ Test that the operators give sane answers }
	runf("math_sin_0", Sin(ShortReal(0.0)), ShortReal(0.0), 0.00001);
	runf("math_sin_pi_2", Sin(ShortReal(pi / 2)), ShortReal(1.0), 0.00001);
	f := ShortReal(pi);
	runf("math_sin_pi", Sin(f), ShortReal(0.0), 0.00001);
	runf("math_cos_0", Cos(ShortReal(0.0)), ShortReal(1.0), 0.00001);
	runf("math_sqrt_1", Sqrt(ShortReal(1.0)), ShortReal(1.0), 0.00001);
	runf("math_sqrt_2", Sqrt(ShortReal(2.0)), ShortReal(1.4142), 0.0001);
	f := Sqrt(ShortReal(-1.0));
	run("math_sqrt_m1", IsNaN(f));
	rund("math_ceil_1.5", Ceil(Real(1.5)), Real(2.0), 0.00001);
	rund("math_ceil_m1.5", Ceil(Real(-1.5)), Real(-1.0), 0.00001);
	rund("math_floor_1.5", Floor(Real(1.5)), Real(1.0), 0.00001);
	rund("math_floor_m1.5", Floor(Real(-1.5)), Real(-2.0), 0.00001);
	rund("math_rint_1.5", Rint(Real(1.5)), Real(2.0), 0.00001);
	rund("math_rint_2.5", Rint(Real(2.5)), Real(2.0), 0.00001);
	rund("math_round_1.5", Round(Real(1.5)), Real(2.0), 0.00001);
	rund("math_round_2.5", Round(Real(2.5)), Real(3.0), 0.00001);
	runn("math_max_1_6", Max(1.0, 6.0), 6.0, 0.00001);
	runn("math_min_1_6", Min(1.0, 6.0), 1.0, 0.00001);
end;

begin
	failed := False;
	run_tests;
	if failed then begin
		Terminate(1);
	end;
end.
