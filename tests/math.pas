(*
 * math.pas - Test mathematical operators.
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

procedure runi(msg: String; value, expected, delta: Integer);
begin
	run(msg, Abs(value - expected) <= delta);
end;

procedure runl(msg: String; value, expected, delta: LongInt);
begin
	run(msg, Abs(value - expected) <= delta);
end;

procedure runui(msg: String; value, expected, delta: Cardinal);
begin
	run(msg, Abs(value - expected) <= delta);
end;

procedure runul(msg: String; value, expected, delta: LongCard);
begin
	run(msg, Abs(value - expected) <= delta);
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

function fneg(f: ShortReal): ShortReal;
begin
	fneg := -f;
end;

function dneg(f: Real): Real;
begin
	dneg := -f;
end;

function nfneg(f: LongReal): LongReal;
begin
	nfneg := -f;
end;

procedure run_conversion_tests;
var
	f: ShortReal;
	d: Real;
	nf: LongReal;
	i: Integer;
	u: Cardinal;
	l: LongInt;
	ul: LongCard;
begin

	{ Tests for conversions from ShortReal to other types }
	f:= 1.5;
	run("math_conv_float32_int 1.5", Integer(f) = 1);
	f:= ShortReal(-1.5);
	run("math_conv_float32_int -1.5", Integer(f) = -1);
	f:= 1.5;
	run("math_conv_float32_uint 1.5", Cardinal(f) = Cardinal(1));
	{ The float mantissa is less than 32 bit so precision might get lost }
	f:= ShortReal(81234500H);
	run("math_conv_float32_uint 81234500H", Cardinal(f) = Cardinal(81234500H));
	f:= 1.5;
	run("math_conv_float32_long 1.5", LongInt(f) = LongInt(1));
	f:= ShortReal(-1.5);
	run("math_conv_float32_long -1.5", LongInt(f) = LongInt(-1));
	f:= 1.5;
	run("math_conv_float32_ulong 1.5", LongCard(f) = LongCard(1));
	{ The 32-bit float fraction is less than 32 bit so precision might get lost }
	f:= ShortReal(81234500H);
	run("math_conv_float32_ulong 81234500H", LongCard(f) = LongCard(81234500H));

	{ Tests for conversions from Real to other types }
	d := 1.5;
	run("math_conv_float64_int 1.5", Integer(d) = 1);
	d := Real(-1.5);
	run("math_conv_float64_int -1.5", Integer(d) = -1);
	d := 1.5;
	run("math_conv_float64_uint 1.5", Cardinal(d) = Cardinal(1));
	d := Real(81234567H);
	run("math_conv_float64_uint 81234567H", Cardinal(d) = Cardinal(81234567H));
	d := 1.5;
	run("math_conv_float64_long 1.5", LongInt(d) = LongInt(1));
	d := Real(-1.5);
	run("math_conv_float64_long -1.5", LongInt(d) = LongInt(-1));
	d := 1.5;
	run("math_conv_float64_ulong 1.5", LongCard(d) = LongCard(1));
	{ The 64-bit float fraction is less than 64 bit so precision might get lost }
	d := Real(8123456789ABC000H);
	run("math_conv_float64_ulong 8123456789ABC000H", LongCard(d) = LongCard(8123456789ABC000H));

	{ Tests for conversions from LongReal to other types }
	nf := 1.5;
	run("math_conv_nfloat_int 1.5", Integer(nf) = 1);
	nf := LongReal(-1.5);
	run("math_conv_nfloat_int -1.5", Integer(nf) = -1);
	nf := 1.5;
	run("math_conv_nfloat_uint 1.5", Cardinal(nf) = Cardinal(1));
	nf := LongReal(81234567H);
	run("math_conv_nfloat_uint 81234567H", Cardinal(nf) = Cardinal(81234567H));
	nf := 1.5;
	run("math_conv_nfloat_long 1.5", LongInt(nf) = LongInt(1));
	nf := LongReal(-1.5);
	run("math_conv_nfloat_long -1.5", LongInt(nf) = LongInt(-1));
	nf := 1.5;
	run("math_conv_nfloat_ulong 1.5", LongCard(nf) = LongCard(1));
	{ The native float fraction might less than 64 bit so precision might get lost }
	nf:= LongReal(8123456789ABC000H);
	run("math_conv_nfloat_ulong 8123456789ABC000H", LongCard(nf) = LongCard(8123456789ABC000H));

	{ Tests for conversions from integer types to ShortReal }
	i := 1;
	runf("math_conv_int_float32 1", ShortReal(i), ShortReal(1.0), 0.00001);
	i := -1;
	runf("math_conv_int_float32 -1", ShortReal(i), ShortReal(-1.0), 0.00001);
	u := 1;
	runf("math_conv_uint_float32 1", ShortReal(u), ShortReal(1.0), 0.00001);
	u := 81234500H;
	runf("math_conv_uint_float32 81234500H", ShortReal(u), ShortReal(81234500H), 0.00001);
	l := 1;
	runf("math_conv_long_float32 1", ShortReal(l), ShortReal(1.0), 0.00001);
	l := -1;
	runf("math_conv_long_float32 -1", ShortReal(l), ShortReal(-1.0), 0.00001);
	ul := 1;
	runf("math_conv_ulong_float32 1", ShortReal(ul), ShortReal(1.0), 0.00001);
	ul := 8123450000000000H;
	runf("math_conv_ulong_float32 8123450000000000H", ShortReal(ul), ShortReal(8123450000000000H), 0.00001);
		
	{ Tests for conversions from integer types to Real }
	i := 1;
	rund("math_conv_int_float64 1", Real(i), Real(1.0), 0.00001);
	i := -1;
	rund("math_conv_int_float64 -1", Real(i), Real(-1.0), 0.00001);
	u := 1;
	rund("math_conv_uint_float64 1", Real(u), Real(1.0), 0.00001);
	u := 81234567H;
	rund("math_conv_uint_float64 81234567H", Real(u), Real(81234567H), 0.00001);
	l := 1;
	rund("math_conv_long_float64 1", Real(l), Real(1.0), 0.00001);
	l := -1;
	rund("math_conv_long_float64 -1", Real(l), Real(-1.0), 0.00001);
	ul := 1;
	rund("math_conv_ulong_float64 1", Real(ul), Real(1.0), 0.00001);
	ul := 8123456789ABC000H;
	rund("math_conv_ulong_float64 8123456789ABC000H", Real(ul), Real(8123456789ABC000H), 0.00001);

	{ Tests for conversions from integer types to LomgReal }
	i := 1;
	runn("math_conv_int_nfloat 1", LongReal(i), LongReal(1.0), 0.00001);
	i := -1;
	runn("math_conv_int_nfloat -1", LongReal(i), LongReal(-1.0), 0.00001);
	u := 1;
	runn("math_conv_uint_nfloat 1", LongReal(u), LongReal(1.0), 0.00001);
	u := 81234567H;
	runn("math_conv_uint_nfloat 81234567H", LongReal(u), LongReal(81234567H), 0.00001);
	l := 1;
	runn("math_conv_long_nfloat 1", LongReal(l), LongReal(1.0), 0.00001);
	l := -1;
	runn("math_conv_long_nfloat -1", LongReal(l), LongReal(-1.0), 0.00001);
	ul := 1;
	runn("math_conv_ulong_nfloat 1", LongReal(ul), LongReal(1.0), 0.00001);
	ul := 8123456789ABC000H;
	runn("math_conv_ulong_nfloat 8123456789ABC000H", LongReal(ul), LongReal(8123456789ABC000H), 0.00001);
	
end;

procedure run_tests;
var
	b: Byte;
	s: ShortInt;
	f: ShortReal;
	d: Real;
	n: LongReal;
	i1, i2: Integer;
	l1, l2: LongInt;
	ui1, ui2: Cardinal;
	ul1, ul2: LongCard;
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
	{ Integer versions }
	runi("math_ci_abs_6", Abs(6), 6, 0);
	runi("math_ci_abs_m6", Abs(-6), 6, 0);
	runi("math_ci_max_1_6", Max(1, 6), 6, 0);
	runi("math_ci_max_m1_6", Max(-1, 6), 6, 0);
	runi("math_ci_max_1_m6", Max(1, -6), 1, 0);
	runi("math_ci_max_m1_m6", Max(-1, -6), -1, 0);
	runi("math_ci_min_1_6", Min(1, 6), 1, 0);
	runi("math_ci_min_m1_6", Min(-1, 6), -1, 0);
	runi("math_ci_min_1_m6", Min(1, -6), -6, 0);
	runi("math_ci_min_m1_m6", Min(-1, -6), -6, 0);
	runi("math_ci_sign_m6", Sign(-6), -1, 0);
	runi("math_ci_sign_6", Sign(6), 1, 0);
	runi("math_ci_sign_0", Sign(0), 0, 0);

	
	runi("math_i_div_6_2", 6 / 2, 3, 0);
	runi("math_i_div_9_3", 9 / 3, 3, 0);
	runi("math_i_div_9_2", 9 / 2, 4, 0);
	runi("math_i_div_m9_2", -9 / 2, -4, 0);
	runi("math_i_div_m9_3", -9 / 3, -3, 0);
	runi("math_i_div_m9_m2", -9 / (-2), 4, 0);
	runi("math_i_div_m9_m3", -9 / (-3), 3, 0);
	runi("math_i_mod_6_2", 6 mod 2, 0, 0);
	runi("math_i_mod_9_3", 9 mod 3, 0, 0);
	runi("math_i_mod_9_2", 9 mod 2, 1, 0);
	runi("math_i_mod_m9_2", -9 mod 2, -1, 0);
	runi("math_i_mod_m9_3", -9 mod 3, 0, 0);
	runi("math_i_mod_m9_m2", -9 mod (-2), -1, 0);
	runi("math_i_mod_m9_m3", -9 mod (-3), 0, 0);
	i1 := 6;
	runi("math_i_abs_6", Abs(i1), 6, 0);
	i1 := -6;
	runi("math_i_abs_m6", Abs(i1), 6, 0);
	i1 := 1;
	i2 := 6;
	runi("math_i_max_1_6", Max(i1, i2), 6, 0);
	i1 := -1;
	runi("math_i_max_m1_6", Max(i1, i2), 6, 0);
	i1 := 1;
	i2 := -6;
	runi("math_i_max_1_m6", Max(i1, i2), 1, 0);
	i1 := -1;
	runi("math_i_max_m1_m6", Max(i1, i2), -1, 0);
	i1 := 1;
	i2 := 6;
	runi("math_i_min_1_6", Min(i1, i2), 1, 0);
	i1 := -1;
	runi("math_i_min_m1_6", Min(i1, i2), -1, 0);
	i1 := 1;
	i2 := -6;
	runi("math_i_min_1_m6", Min(i1, i2), -6, 0);
	i1 := -1;
	runi("math_i_min_m1_m6", Min(i1, i2), -6, 0);
	i1 := -6;
	runi("math_i_sign_m6", Sign(i1), -1, 0);
	i1 := 6;
	runi("math_i_sign_6", Sign(i1), 1, 0);
	i1 := 0;
	runi("math_i_sign_0", Sign(i1), 0, 0);

	{ LongInt versions }
	runl("math_cl_abs_6", Abs(600000000h), 600000000h, 0);
	runl("math_cl_abs_m6", Abs(-600000000h), 600000000h, 0);
	runl("math_cl_max_1_6", Max(100000000h, 600000000h), 600000000h, 0);
	runl("math_cl_max_m1_6", Max(-100000000h, 600000000h), 600000000h, 0);
	runl("math_cl_max_1_m6", Max(100000000h, -600000000h), 100000000h, 0);
	runl("math_cl_max_m1_m6", Max(-100000000h, -600000000h), -100000000h, 0);
	runl("math_cl_min_1_6", Min(100000000h, 600000000h), 100000000h, 0);
	runl("math_cl_min_m1_6", Min(-100000000h, 600000000h), -100000000h, 0);
	runl("math_cl_min_1_m6", Min(100000000h, -600000000h), -600000000h, 0);
	runl("math_cl_min_m1_m6", Min(-100000000h, -600000000h), -600000000h, 0);
	runi("math_cl_sign_m6", Sign(-600000000h), -1, 0);
	runi("math_cl_sign_6", Sign(600000000h), 1, 0);
	runi("math_cl_sign_0", Sign(0), 0, 0);

	l1 := 6;
	runl("math_l_div_6_2", l1 / 2, 3, 0);
	l1 := 9;
	runl("math_l_div_9_3", l1 / 3, 3, 0);
	runl("math_l_div_9_2", l1 / 2, 4, 0);
	l1 := -9;
	runl("math_l_div_m9_2", l1 / 2, -4, 0);
	runl("math_l_div_m9_3", l1 / 3, -3, 0);
	runl("math_l_div_m9_m2", l1 / (-2), 4, 0);
	runl("math_l_div_m9_m3", l1 / (-3), 3, 0);
	l1 := 6;
	runl("math_l_mod_6_2", l1 mod 2, 0, 0);
	l1 := 9;
	runl("math_l_mod_9_3", l1 mod 3, 0, 0);
	runl("math_l_mod_9_2", l1 mod 2, 1, 0);
	l1 := -9;
	runl("math_l_mod_m9_2", l1 mod 2, -1, 0);
	runl("math_l_mod_m9_3", l1 mod 3, 0, 0);
	runl("math_l_mod_m9_m2", l1 mod (-2), -1, 0);
	runl("math_l_mod_m9_m3", l1 mod (-3), 0, 0);
	l1 := 6;
	runl("math_l_abs_6", Abs(l1), 6, 0);
	l1 := -6;
	runl("math_l_abs_m6", Abs(l1), 6, 0);
	l1 := 1;
	l2 := 6;
	runl("math_l_max_1_6", Max(l1, l2), 6, 0);
	l1 := -1;
	runl("math_l_max_m1_6", Max(l1, l2), 6, 0);
	l1 := 1;
	l2 := -6;
	runl("math_l_max_1_m6", Max(l1, l2), 1, 0);
	l1 := -1;
	runl("math_l_max_m1_m6", Max(l1, l2), -1, 0);
	l1 := 1;
	l2 := 6;
	runl("math_l_min_1_6", Min(l1, l2), 1, 0);
	l1 := -1;
	runl("math_l_min_m1_6", Min(l1, l2), -1, 0);
	l1 := 1;
	l2 := -6;
	runl("math_l_min_1_m6", Min(l1, l2), -6, 0);
	l1 := -1;
	runl("math_l_min_m1_m6", Min(l1, l2), -6, 0);
	l1 := -6;
	runi("math_l_sign_m6", Sign(l1), -1, 0);
	l1 := 6;
	runi("math_l_sign_6", Sign(l1), 1, 0);
	l1 := 0;
	runi("math_l_sign_0", Sign(l1), 0, 0);

	{ Unsigned integer versions }
	ui1 := 1;
	ui2 := 6;
	runui("math_ui_max_1_6", Max(ui1, ui2), 6, 0);
	ui1 := 0ffffffffh;
	ui2 := 0fffffff1h;
	runui("math_ui_max_ffffffff_fffffff1", Max(ui1, ui2), 0ffffffffh, 0);
	ui1 := 1;
	ui2 := 6;
	runui("math_ui_min_1_6", Min(ui1, ui2), 1, 0);
	ui1 := 0ffffffffh;
	ui2 := 0fffffff1h;
	runui("math_ui_min_ffffffff_fffffff1", Min(ui1, ui2), 0fffffff1h, 0);

	{ Unsigned long versions }
	ul1 := 1;
	ul2 := 6;
	runul("math_ul_max_1_6", Max(ul1, ul2), 6, 0);
	ul1 := 0ffffffffffffffffh;
	ul2 := 0fffffffffffffff1h;
	runul("math_ui_max_ffffffffffffffff_fffffffffffffff1", Max(ul1, ul2), 0ffffffffffffffffh, 0);
	ul1 := 1;
	ul2 := 6;
	runul("math_ul_min_1_6", Min(ul1, ul2), 1, 0);
	ul1 := 0ffffffffffffffffh;
	ul2 := 0fffffffffffffff1h;
	runui("math_ul_min_ffffffffffffffff_fffffffffffffff1", Min(ul1, ul2), 0fffffffffffffff1h, 0);

	{ short real versions }
	runf("math_f_abs_1.5", Abs(ShortReal(1.5)), ShortReal(1.5), 0.00001);
	runf("math_f_abs_m1.5", Abs(ShortReal(-1.5)), ShortReal(1.5), 0.00001);
	runf("math_f_neg_1.5", fneg(1.5), ShortReal(-1.5), 0.00001);
	runf("math_f_neg_m1.5", fneg(-1.5), ShortReal(1.5), 0.00001);
	runf("math_f_max_1_6", Max(ShortReal(1.0), ShortReal(6.0)), ShortReal(6.0), 0.00001);
	runf("math_f_min_1_6", Min(ShortReal(1.0), ShortReal(6.0)), ShortReal(1.0), 0.00001);
	runf("math_f_sin_0", Sin(ShortReal(0.0)), ShortReal(0.0), 0.00001);
	runf("math_f_sin_pi_2", Sin(ShortReal(pi / 2)), ShortReal(1.0), 0.00001);
	f := ShortReal(pi);
	runf("math_f_sin_pi", Sin(f), ShortReal(0.0), 0.00001);
	runf("math_f_cos_0", Cos(ShortReal(0.0)), ShortReal(1.0), 0.00001);
	runf("math_f_sqrt_1", Sqrt(ShortReal(1.0)), ShortReal(1.0), 0.00001);
	runf("math_f_sqrt_2", Sqrt(ShortReal(2.0)), ShortReal(1.4142), 0.0001);
	f := Sqrt(ShortReal(-1.0));
	run("math_f_sqrt_m1", IsNaN(f));
	runf("math_f_ceil_1.5", Ceil(ShortReal(1.5)), ShortReal(2.0), 0.00001);
	runf("math_f_ceil_m1.5", Ceil(ShortReal(-1.5)), ShortReal(-1.0), 0.00001);
	runf("math_f_floor_1.5", Floor(ShortReal(1.5)), ShortReal(1.0), 0.00001);
	runf("math_f_floor_m1.5", Floor(ShortReal(-1.5)), ShortReal(-2.0), 0.00001);
	runf("math_f_rint_1.5", Rint(ShortReal(1.5)), ShortReal(2.0), 0.00001);
	runf("math_f_rint_2.5", Rint(ShortReal(2.5)), ShortReal(2.0), 0.00001);
	runf("math_f_round_1.5", Round(ShortReal(1.5)), ShortReal(2.0), 0.00001);
	runf("math_f_round_2.5", Round(ShortReal(2.5)), ShortReal(3.0), 0.00001);
	runf("math_f_trunc_1.5", Trunc(ShortReal(1.5)), ShortReal(1.0), 0.00001);
	runf("math_f_trunc_2.5", Trunc(ShortReal(2.5)), ShortReal(2.0), 0.00001);
	runf("math_f_trunc_m1.5", Trunc(ShortReal(-1.5)), ShortReal(-1.0), 0.00001);

	{ real versions }
	rund("math_d_abs_1.5", Abs(Real(1.5)), Real(1.5), 0.00001);
	rund("math_d_abs_m1.5", Abs(Real(-1.5)), Real(1.5), 0.00001);
	rund("math_d_neg_1.5", dneg(1.5), Real(-1.5), 0.00001);
	rund("math_d_neg_m1.5", dneg(-1.5), Real(1.5), 0.00001);
	rund("math_d_max_1_6", Max(Real(1.0), Real(6.0)), Real(6.0), 0.00001);
	rund("math_d_min_1_6", Min(Real(1.0), Real(6.0)), Real(1.0), 0.00001);
	rund("math_d_sin_0", Sin(Real(0.0)), Real(0.0), 0.00001);
	rund("math_d_sin_pi_2", Sin(Real(pi / 2)), Real(1.0), 0.00001);
	d := Real(pi);
	rund("math_d_sin_pi", Sin(d), Real(0.0), 0.00001);
	rund("math_d_cos_0", Cos(Real(0.0)), Real(1.0), 0.00001);
	rund("math_d_sqrt_1", Sqrt(Real(1.0)), Real(1.0), 0.00001);
	rund("math_d_sqrt_2", Sqrt(Real(2.0)), Real(1.4142), 0.0001);
	d := Sqrt(Real(-1.0));
	run("math_d_sqrt_m1", IsNaN(d));
	rund("math_d_ceil_1.5", Ceil(Real(1.5)), Real(2.0), 0.00001);
	rund("math_d_ceil_m1.5", Ceil(Real(-1.5)), Real(-1.0), 0.00001);
	rund("math_d_floor_1.5", Floor(Real(1.5)), Real(1.0), 0.00001);
	rund("math_d_floor_m1.5", Floor(Real(-1.5)), Real(-2.0), 0.00001);
	rund("math_d_rint_1.5", Rint(Real(1.5)), Real(2.0), 0.00001);
	rund("math_d_rint_2.5", Rint(Real(2.5)), Real(2.0), 0.00001);
	rund("math_d_round_1.5", Round(Real(1.5)), Real(2.0), 0.00001);
	rund("math_d_round_2.5", Round(Real(2.5)), Real(3.0), 0.00001);
	rund("math_d_trunc_1.5", Trunc(Real(1.5)), Real(1.0), 0.00001);
	rund("math_d_trunc_2.5", Trunc(Real(2.5)), Real(2.0), 0.00001);
	rund("math_d_trunc_m1.5", Trunc(Real(-1.5)), Real(-1.0), 0.00001);

	{ long real versions }
	runn("math_n_abs_1.5", Abs(LongReal(1.5)), LongReal(1.5), 0.00001);
	runn("math_n_abs_m1.5", Abs(LongReal(-1.5)), LongReal(1.5), 0.00001);
	runn("math_n_neg_1.5", nfneg(1.5), LongReal(-1.5), 0.00001);
	runn("math_n_neg_m1.5", nfneg(-1.5), LongReal(1.5), 0.00001);
	runn("math_n_max_1_6", Max(LongReal(1.0), LongReal(6.0)), LongReal(6.0), 0.00001);
	runn("math_n_min_1_6", Min(LongReal(1.0), LongReal(6.0)), LongReal(1.0), 0.00001);
	runn("math_n_sin_0", Sin(LongReal(0.0)), LongReal(0.0), 0.00001);
	runn("math_n_sin_pi_2", Sin(LongReal(pi / 2)), LongReal(1.0), 0.00001);
	n := LongReal(pi);
	runn("math_n_sin_pi", Sin(n), LongReal(0.0), 0.00001);
	runn("math_n_cos_0", Cos(LongReal(0.0)), LongReal(1.0), 0.00001);
	runn("math_n_sqrt_1", Sqrt(LongReal(1.0)), LongReal(1.0), 0.00001);
	runn("math_n_sqrt_2", Sqrt(LongReal(2.0)), LongReal(1.4142), 0.0001);
	n := Sqrt(LongReal(-1.0));
	run("math_n_sqrt_m1", IsNaN(n));
	runn("math_n_ceil 1.5", Ceil(LongReal(1.5)), LongReal(2.0), 0.00001);
	runn("math_n_ceil -1.5", Ceil(LongReal(-1.5)), LongReal(-1.0), 0.00001);
	runn("math_n_floor 1.5", Floor(LongReal(1.5)), LongReal(1.0), 0.00001);
	runn("math_n_floor -1.5", Floor(LongReal(-1.5)), LongReal(-2.0), 0.00001);
	runn("math_n_rint 1.5", Rint(LongReal(1.5)), LongReal(2.0), 0.00001);
	runn("math_n_rint 2.5", Rint(LongReal(2.5)), LongReal(2.0), 0.00001);
	runn("math_n_round 1.5", Round(LongReal(1.5)), LongReal(2.0), 0.00001);
	runn("math_n_round 2.5", Round(LongReal(2.5)), LongReal(3.0), 0.00001);
	runn("math_n_trunc 1.5", Trunc(LongReal(1.5)), LongReal(1.0), 0.00001);
	runn("math_n_trunc 2.5", Trunc(LongReal(2.5)), LongReal(2.0), 0.00001);
	runn("math_n_trunc -1.5", Trunc(LongReal(-1.5)), LongReal(-1.0), 0.00001);

	run_conversion_tests;
end;

begin
	failed := False;
	run_tests;
	if failed then begin
		Terminate(1);
	end;
end.
