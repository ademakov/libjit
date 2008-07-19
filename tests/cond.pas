(*
 * cond.pas - Test conditions and branches.
 *
 * Copyright (C) 2008  Southern Storm Software, Pty Ltd.
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

program cond;

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

procedure runcheck(msg: String; value, expected: Boolean);
begin
	if value then begin
		if expected then begin
			run(msg, True);
		end else begin
			run(msg, False);
		end;
	end else begin
		if expected then begin
			run(msg, False);
		end else begin
			run(msg, True);
		end;
	end;
end;

procedure const_integer_tests;
var
	i1: Integer;
begin
	runcheck("cond_ci_eq_0_0", (0 = 0), True);
	i1 := 1;
	runcheck("cond_ci_eq_1_0", (i1 = 0), False);
	runcheck("cond_ci_eq_1_1", (i1 = 1), True);
	i1 := 0;
	runcheck("cond_ci_eq_0_1", (i1 = 1), False);

	runcheck("cond_ci_ne_0_0", (0 <> 0), False);
	i1 := 1;
	runcheck("cond_ci_ne_1_0", (i1 <> 0), True);
	runcheck("cond_ci_ne_1_1", (i1 <> 1), False);
	i1 := 0;
	runcheck("cond_ci_ne_0_1", (i1 <> 1), True);

	runcheck("cond_ci_gt_0_0", (0 > 0), False);
	i1 := 1;
	runcheck("cond_ci_gt_1_0", (i1 > 0), True);
	runcheck("cond_ci_gt_1_1", (i1 > 1), False);
	i1 := 2;
	runcheck("cond_ci_gt_2_1", (i1 > 1), True);
	i1 := 0;
	runcheck("cond_ci_gt_0_1", (i1 > 1), False);
	i1 := -1;
	runcheck("cond_ci_gt_m1_0", (i1 > 0), False);
	runcheck("cond_ci_gt_m1_1", (i1 > 1), False);
	runcheck("cond_ci_gt_m1_m2", (i1 > -2), True);
	i1 := -2;
	runcheck("cond_ci_gt_m2_m1", (i1 > -1), False);

	runcheck("cond_ci_ge_0_0", (0 >= 0), True);
	i1 := 1;
	runcheck("cond_ci_ge_1_0", (i1 >= 0), True);
	runcheck("cond_ci_ge_1_1", (i1 >= 1), True);
	i1 := 2;
	runcheck("cond_ci_ge_2_1", (i1 >= 1), True);
	i1 := 0;
	runcheck("cond_ci_ge_0_1", (i1 >= 1), False);
	i1 := -1;
	runcheck("cond_ci_ge_m1_0", (i1 >= 0), False);
	runcheck("cond_ci_ge_m1_1", (i1 >= 1), False);
	runcheck("cond_ci_ge_m1_m1", (i1 >= -1), True);
	runcheck("cond_ci_ge_m1_m2", (i1 >= -2), True);
	i1 := -2;
	runcheck("cond_ci_ge_m2_m1", (i1 >= -1), False);

	runcheck("cond_ci_lt_0_0", (0 < 0), False);
	i1 := 1;
	runcheck("cond_ci_lt_1_0", (i1 < 0), False);
	runcheck("cond_ci_lt_1_1", (i1 < 1), False);
	i1 := 2;
	runcheck("cond_ci_lt_2_1", (i1 < 1), False);
	i1 := 0;
	runcheck("cond_ci_lt_0_1", (i1 < 1), True);
	i1 := -1;
	runcheck("cond_ci_lt_m1_0", (i1 < 0), True);
	runcheck("cond_ci_lt_m1_1", (i1 < 1), True);
	runcheck("cond_ci_lt_m1_m1", (i1 < -1), False);
	runcheck("cond_ci_lt_m1_m2", (i1 < -2), False);
	i1 := -2;
	runcheck("cond_ci_lt_m2_m1", (i1 < -1), True);

	runcheck("cond_ci_le_0_0", (0 <= 0), True);
	i1 := 1;
	runcheck("cond_ci_le_1_0", (i1 <= 0), False);
	runcheck("cond_ci_le_1_1", (i1 <= 1), True);
	i1 := 2;
	runcheck("cond_ci_le_2_1", (i1 <= 1), False);
	i1 := 0;
	runcheck("cond_ci_le_0_1", (i1 <= 1), True);
	i1 := -1;
	runcheck("cond_ci_le_m1_0", (i1 <= 0), True);
	runcheck("cond_ci_le_m1_1", (i1 <= 1), True);
	runcheck("cond_ci_le_m1_m1", (i1 <= -1), True);
	runcheck("cond_ci_le_m1_m2", (i1 <= -2), False);
	i1 := -2;
	runcheck("cond_ci_le_m2_m1", (i1 <= -1), True);
end;

function integer_eq(i1, i2: Integer): Boolean;
begin
	integer_eq := (i1 = i2);
end;

function integer_ne(i1, i2: Integer): Boolean;
begin
	integer_ne := (i1 <> i2);
end;

function integer_ge(i1, i2: Integer): Boolean;
begin
	integer_ge := (i1 >= i2);
end;

function integer_gt(i1, i2: Integer): Boolean;
begin
	integer_gt := (i1 > i2);
end;

function integer_le(i1, i2: Integer): Boolean;
begin
	integer_le := (i1 <= i2);
end;

function integer_lt(i1, i2: Integer): Boolean;
begin
	integer_lt := (i1 < i2);
end;

procedure integer_tests;
begin
	runcheck("cond_i_eq_0_0", integer_eq(0, 0), True);
	runcheck("cond_i_eq_1_1", integer_eq(1, 1), True);
	runcheck("cond_i_eq_1_0", integer_eq(1, 0), False);
	runcheck("cond_i_eq_1_0", integer_eq(0, 1), False);
	runcheck("cond_i_ne_0_0", integer_ne(0, 0), False);
	runcheck("cond_i_ne_1_1", integer_ne(1, 1), False);
	runcheck("cond_i_ne_1_0", integer_ne(1, 0), True);
	runcheck("cond_i_ne_0_1", integer_ne(0, 1), True);
	runcheck("cond_i_gt_0_0", integer_gt(0, 0), False);
	runcheck("cond_i_gt_1_1", integer_gt(1, 1), False);
	runcheck("cond_i_gt_1_0", integer_gt(1, 0), True);
	runcheck("cond_i_gt_0_1", integer_gt(0, 1), False);
	runcheck("cond_i_gt_m1_0", integer_gt(-1, 0), False);
	runcheck("cond_i_gt_0_m1", integer_gt(0, -1), True);
	runcheck("cond_i_gt_m1_m1", integer_gt(-1, -1), False);
	runcheck("cond_i_ge_0_0", integer_ge(0, 0), True);
	runcheck("cond_i_ge_1_1", integer_ge(1, 1), True);
	runcheck("cond_i_ge_1_0", integer_ge(1, 0), True);
	runcheck("cond_i_ge_0_1", integer_ge(0, 1), False);
	runcheck("cond_i_ge_m1_0", integer_ge(-1, 0), False);
	runcheck("cond_i_ge_0_m1", integer_ge(0, -1), True);
	runcheck("cond_i_ge_m1_m1", integer_ge(-1, -1), True);
	runcheck("cond_i_lt_0_0", integer_lt(0, 0), False);
	runcheck("cond_i_lt_1_1", integer_lt(1, 1), False);
	runcheck("cond_i_lt_1_0", integer_lt(1, 0), False);
	runcheck("cond_i_lt_0_1", integer_lt(0, 1), True);
	runcheck("cond_i_lt_m1_0", integer_lt(-1, 0), True);
	runcheck("cond_i_lt_0_m1", integer_lt(0, -1), False);
	runcheck("cond_i_lt_m1_m1", integer_lt(-1, -1), False);
	runcheck("cond_i_le_0_0", integer_le(0, 0), True);
	runcheck("cond_i_le_1_1", integer_le(1, 1), True);
	runcheck("cond_i_le_1_0", integer_le(1, 0), False);
	runcheck("cond_i_le_0_1", integer_le(0, 1), True);
	runcheck("cond_i_le_m1_0", integer_le(-1, 0), True);
	runcheck("cond_i_le_0_m1", integer_le(0, -1), False);
	runcheck("cond_i_le_m1_m1", integer_le(-1, -1), True);
end;

function cardinal_eq(i1, i2: Cardinal): Boolean;
begin
	cardinal_eq := (i1 = i2);
end;

function cardinal_ne(i1, i2: Cardinal): Boolean;
begin
	cardinal_ne := (i1 <> i2);
end;

function cardinal_ge(i1, i2: Cardinal): Boolean;
begin
	cardinal_ge := (i1 >= i2);
end;

function cardinal_gt(i1, i2: Cardinal): Boolean;
begin
	cardinal_gt := (i1 > i2);
end;

function cardinal_le(i1, i2: Cardinal): Boolean;
begin
	cardinal_le := (i1 <= i2);
end;

function cardinal_lt(i1, i2: Cardinal): Boolean;
begin
	cardinal_lt := (i1 < i2);
end;

procedure cardinal_tests;
begin
	runcheck("cond_ui_eq_0_0", cardinal_eq(0, 0), True);
	runcheck("cond_ui_eq_1_1", cardinal_eq(1, 1), True);
	runcheck("cond_ui_eq_1_0", cardinal_eq(1, 0), False);
	runcheck("cond_ui_eq_1_0", cardinal_eq(0, 1), False);
	runcheck("cond_ui_ne_0_0", cardinal_ne(0, 0), False);
	runcheck("cond_ui_ne_1_1", cardinal_ne(1, 1), False);
	runcheck("cond_ui_ne_1_0", cardinal_ne(1, 0), True);
	runcheck("cond_ui_ne_0_1", cardinal_ne(0, 1), True);
	runcheck("cond_ui_gt_0_0", cardinal_gt(0, 0), False);
	runcheck("cond_ui_gt_1_1", cardinal_gt(1, 1), False);
	runcheck("cond_ui_gt_1_0", cardinal_gt(1, 0), True);
	runcheck("cond_ui_gt_0_1", cardinal_gt(0, 1), False);
	runcheck("cond_ui_gt_80000000h_0", cardinal_gt(80000000h, 0), True);
	runcheck("cond_ui_gt_0_80000000h", cardinal_gt(0, 80000000h), False);
	runcheck("cond_ui_gt_80000000h_80000000h", cardinal_gt(80000000h, 80000000h), False);
	runcheck("cond_ui_ge_0_0", cardinal_ge(0, 0), True);
	runcheck("cond_ui_ge_1_1", cardinal_ge(1, 1), True);
	runcheck("cond_ui_ge_1_0", cardinal_ge(1, 0), True);
	runcheck("cond_ui_ge_0_1", cardinal_ge(0, 1), False);
	runcheck("cond_ui_ge_80000000h_0", cardinal_ge(80000000h, 0), True);
	runcheck("cond_ui_ge_0_80000000h", cardinal_ge(0, 80000000h), False);
	runcheck("cond_ui_ge_80000000h_80000000h", cardinal_ge(80000000h, 80000000h), True);
	runcheck("cond_ui_lt_0_0", cardinal_lt(0, 0), False);
	runcheck("cond_ui_lt_1_1", cardinal_lt(1, 1), False);
	runcheck("cond_ui_lt_1_0", cardinal_lt(1, 0), False);
	runcheck("cond_ui_lt_0_1", cardinal_lt(0, 1), True);
	runcheck("cond_ui_lt_80000000h_0", cardinal_lt(80000000h, 0), False);
	runcheck("cond_ui_lt_0_80000000h", cardinal_lt(0, 80000000h), True);
	runcheck("cond_ui_lt_80000000h_80000000h", cardinal_lt(80000000h, 80000000h), False);
	runcheck("cond_ui_le_0_0", cardinal_le(0, 0), True);
	runcheck("cond_ui_le_1_1", cardinal_le(1, 1), True);
	runcheck("cond_ui_le_1_0", cardinal_le(1, 0), False);
	runcheck("cond_ui_le_0_1", cardinal_le(0, 1), True);
	runcheck("cond_ui_le_80000000h_0", cardinal_le(80000000h, 0), False);
	runcheck("cond_ui_le_0_80000000h", cardinal_le(0, 80000000h), True);
	runcheck("cond_ui_le_80000000h_80000000h", cardinal_le(80000000h, 80000000h), True);
end;

function longint_eq(i1, i2: LongInt): Boolean;
begin
	longint_eq := (i1 = i2);
end;

function longint_ne(i1, i2: LongInt): Boolean;
begin
	longint_ne := (i1 <> i2);
end;

function longint_ge(i1, i2: LongInt): Boolean;
begin
	longint_ge := (i1 >= i2);
end;

function longint_gt(i1, i2: LongInt): Boolean;
begin
	longint_gt := (i1 > i2);
end;

function longint_le(i1, i2: LongInt): Boolean;
begin
	longint_le := (i1 <= i2);
end;

function longint_lt(i1, i2: LongInt): Boolean;
begin
	longint_lt := (i1 < i2);
end;

procedure longint_tests;
begin
	runcheck("cond_l_eq_0_0", longint_eq(0, 0), True);
	runcheck("cond_l_eq_1_1", longint_eq(1, 1), True);
	runcheck("cond_l_eq_1_0", longint_eq(1, 0), False);
	runcheck("cond_l_eq_1_0", longint_eq(0, 1), False);
	runcheck("cond_l_ne_0_0", longint_ne(0, 0), False);
	runcheck("cond_l_ne_1_1", longint_ne(1, 1), False);
	runcheck("cond_l_ne_1_0", longint_ne(1, 0), True);
	runcheck("cond_l_ne_0_1", longint_ne(0, 1), True);
	runcheck("cond_l_gt_0_0", longint_gt(0, 0), False);
	runcheck("cond_l_gt_1_1", longint_gt(1, 1), False);
	runcheck("cond_l_gt_1_0", longint_gt(1, 0), True);
	runcheck("cond_l_gt_0_1", longint_gt(0, 1), False);
	runcheck("cond_l_gt_m1_0", longint_gt(-1, 0), False);
	runcheck("cond_l_gt_0_m1", longint_gt(0, -1), True);
	runcheck("cond_l_gt_m1_m1", longint_gt(-1, -1), False);
	runcheck("cond_l_ge_0_0", longint_ge(0, 0), True);
	runcheck("cond_l_ge_1_1", longint_ge(1, 1), True);
	runcheck("cond_l_ge_1_0", longint_ge(1, 0), True);
	runcheck("cond_l_ge_0_1", longint_ge(0, 1), False);
	runcheck("cond_l_ge_m1_0", longint_ge(-1, 0), False);
	runcheck("cond_l_ge_0_m1", longint_ge(0, -1), True);
	runcheck("cond_l_ge_m1_m1", longint_ge(-1, -1), True);
	runcheck("cond_l_lt_0_0", longint_lt(0, 0), False);
	runcheck("cond_l_lt_1_1", longint_lt(1, 1), False);
	runcheck("cond_l_lt_1_0", longint_lt(1, 0), False);
	runcheck("cond_l_lt_0_1", longint_lt(0, 1), True);
	runcheck("cond_l_lt_m1_0", longint_lt(-1, 0), True);
	runcheck("cond_l_lt_0_m1", longint_lt(0, -1), False);
	runcheck("cond_l_lt_m1_m1", longint_lt(-1, -1), False);
	runcheck("cond_l_le_0_0", longint_le(0, 0), True);
	runcheck("cond_l_le_1_1", longint_le(1, 1), True);
	runcheck("cond_l_le_1_0", longint_le(1, 0), False);
	runcheck("cond_l_le_0_1", longint_le(0, 1), True);
	runcheck("cond_l_le_m1_0", longint_le(-1, 0), True);
	runcheck("cond_l_le_0_m1", longint_le(0, -1), False);
	runcheck("cond_l_le_m1_m1", longint_le(-1, -1), True);
end;

function longcard_eq(i1, i2: LongCard): Boolean;
begin
	longcard_eq := (i1 = i2);
end;

function longcard_ne(i1, i2: LongCard): Boolean;
begin
	longcard_ne := (i1 <> i2);
end;

function longcard_ge(i1, i2: LongCard): Boolean;
begin
	longcard_ge := (i1 >= i2);
end;

function longcard_gt(i1, i2: LongCard): Boolean;
begin
	longcard_gt := (i1 > i2);
end;

function longcard_le(i1, i2: LongCard): Boolean;
begin
	longcard_le := (i1 <= i2);
end;

function longcard_lt(i1, i2: LongCard): Boolean;
begin
	longcard_lt := (i1 < i2);
end;

procedure longcard_tests;
begin
	runcheck("cond_ul_eq_0_0", longcard_eq(0, 0), True);
	runcheck("cond_ul_eq_1_1", longcard_eq(1, 1), True);
	runcheck("cond_ul_eq_1_0", longcard_eq(1, 0), False);
	runcheck("cond_ul_eq_1_0", longcard_eq(0, 1), False);
	runcheck("cond_ul_ne_0_0", longcard_ne(0, 0), False);
	runcheck("cond_ul_ne_1_1", longcard_ne(1, 1), False);
	runcheck("cond_ul_ne_1_0", longcard_ne(1, 0), True);
	runcheck("cond_ul_ne_0_1", longcard_ne(0, 1), True);
	runcheck("cond_ul_gt_0_0", longcard_gt(0, 0), False);
	runcheck("cond_ul_gt_1_1", longcard_gt(1, 1), False);
	runcheck("cond_ul_gt_1_0", longcard_gt(1, 0), True);
	runcheck("cond_ul_gt_0_1", longcard_gt(0, 1), False);
	runcheck("cond_ul_gt_8000000000000000h_0", longcard_gt(8000000000000000h, 0), True);
	runcheck("cond_ul_gt_0_8000000000000000h", longcard_gt(0, 8000000000000000h), False);
	runcheck("cond_ul_gt_8000000000000000h_8000000000000000h", longcard_gt(8000000000000000h, 8000000000000000h), False);
	runcheck("cond_ul_ge_0_0", longcard_ge(0, 0), True);
	runcheck("cond_ul_ge_1_1", longcard_ge(1, 1), True);
	runcheck("cond_ul_ge_1_0", longcard_ge(1, 0), True);
	runcheck("cond_ul_ge_0_1", longcard_ge(0, 1), False);
	runcheck("cond_ul_ge_8000000000000000h_0", longcard_ge(8000000000000000h, 0), True);
	runcheck("cond_ul_ge_0_8000000000000000h", longcard_ge(0, 8000000000000000h), False);
	runcheck("cond_ul_ge_8000000000000000h_8000000000000000h", longcard_ge(8000000000000000h, 8000000000000000h), True);
	runcheck("cond_ul_lt_0_0", longcard_lt(0, 0), False);
	runcheck("cond_ul_lt_1_1", longcard_lt(1, 1), False);
	runcheck("cond_ul_lt_1_0", longcard_lt(1, 0), False);
	runcheck("cond_ul_lt_0_1", longcard_lt(0, 1), True);
	runcheck("cond_ul_lt_8000000000000000h_0", longcard_lt(8000000000000000h, 0), False);
	runcheck("cond_ul_lt_0_8000000000000000h", longcard_lt(0, 8000000000000000h), True);
	runcheck("cond_ul_lt_8000000000000000h_8000000000000000h", longcard_lt(8000000000000000h, 8000000000000000h), False);
	runcheck("cond_ul_le_0_0", longcard_le(0, 0), True);
	runcheck("cond_ul_le_1_1", longcard_le(1, 1), True);
	runcheck("cond_ul_le_1_0", longcard_le(1, 0), False);
	runcheck("cond_ul_le_0_1", longcard_le(0, 1), True);
	runcheck("cond_ul_le_8000000000000000h_0", longcard_le(8000000000000000h, 0), False);
	runcheck("cond_ul_le_0_8000000000000000h", longcard_le(0, 8000000000000000h), True);
	runcheck("cond_ul_le_8000000000000000h_8000000000000000h", longcard_le(8000000000000000h, 8000000000000000h), True);
end;

function shortreal_eq(f1, f2: ShortReal): Boolean;
begin
	shortreal_eq := (f1 = f2);
end;

function shortreal_ne(f1, f2: ShortReal): Boolean;
begin
	shortreal_ne := (f1 <> f2);
end;

function shortreal_ge(f1, f2: ShortReal): Boolean;
begin
	shortreal_ge := (f1 >= f2);
end;

function shortreal_gt(f1, f2: ShortReal): Boolean;
begin
	shortreal_gt := (f1 > f2);
end;

function shortreal_le(f1, f2: ShortReal): Boolean;
begin
	shortreal_le := (f1 <= f2);
end;

function shortreal_lt(f1, f2: ShortReal): Boolean;
begin
	shortreal_lt := (f1 < f2);
end;

function shortreal_neq(f1, f2: ShortReal): Boolean;
begin
	shortreal_neq := not (f1 = f2);
end;

function shortreal_nne(f1, f2: ShortReal): Boolean;
begin
	shortreal_nne := not (f1 <> f2);
end;

function shortreal_nge(f1, f2: ShortReal): Boolean;
begin
	shortreal_nge := not (f1 >= f2);
end;

function shortreal_ngt(f1, f2: ShortReal): Boolean;
begin
	shortreal_ngt := not (f1 > f2);
end;

function shortreal_nle(f1, f2: ShortReal): Boolean;
begin
	shortreal_nle := not (f1 <= f2);
end;

function shortreal_nlt(f1, f2: ShortReal): Boolean;
begin
	shortreal_nlt := not (f1 < f2);
end;

procedure shortreal_tests;
var
	fnan :ShortReal;
begin
	fnan := sqrt(ShortReal(-1));

	runcheck("cond_f_eq_nan_0.0", shortreal_eq(fnan, 0.0), False);
	runcheck("cond_f_eq_0.0_nan", shortreal_eq(0.0, fnan), False);
	runcheck("cond_f_eq_nan_nan", shortreal_eq(fnan, fnan), False);
	runcheck("cond_f_eq_0.0_0.0", shortreal_eq(0.0, 0.0), True);
	runcheck("cond_f_eq_1.0_0.0", shortreal_eq(1.0, 0.0), False);
	runcheck("cond_f_eq_0.0_1.0", shortreal_eq(0.0, 1.0), False);
	runcheck("cond_f_neq_nan_0.0", shortreal_neq(fnan, 0.0), True);
	runcheck("cond_f_neq_0.0_nan", shortreal_neq(0.0, fnan), True);
	runcheck("cond_f_neq_nan_nan", shortreal_neq(fnan, fnan), True);
	runcheck("cond_f_neq_0.0_0.0", shortreal_neq(0.0, 0.0), False);
	runcheck("cond_f_neq_1.0_0.0", shortreal_neq(1.0, 0.0), True);
	runcheck("cond_f_neq_0.0_1.0", shortreal_neq(0.0, 1.0), True);
	runcheck("cond_f_ne_nan_0.0", shortreal_ne(fnan, 0.0), True);
	runcheck("cond_f_ne_0.0_nan", shortreal_ne(0.0, fnan), True);
	runcheck("cond_f_ne_nan_nan", shortreal_ne(fnan, fnan), True);
	runcheck("cond_f_ne_0.0_0.0", shortreal_ne(0.0, 0.0), False);
	runcheck("cond_f_ne_1.0_0.0", shortreal_ne(1.0, 0.0), True);
	runcheck("cond_f_ne_0.0_1.0", shortreal_ne(0.0, 1.0), True);
	runcheck("cond_f_nne_nan_0.0", shortreal_nne(fnan, 0.0), False);
	runcheck("cond_f_nne_0.0_nan", shortreal_nne(0.0, fnan), False);
	runcheck("cond_f_nne_nan_nan", shortreal_nne(fnan, fnan), False);
	runcheck("cond_f_nne_0.0_0.0", shortreal_nne(0.0, 0.0), True);
	runcheck("cond_f_nne_1.0_0.0", shortreal_nne(1.0, 0.0), False);
	runcheck("cond_f_nne_0.0_1.0", shortreal_nne(0.0, 1.0), False);
	runcheck("cond_f_gt_nan_0.0", shortreal_gt(fnan, 0.0), False);
	runcheck("cond_f_gt_0.0_nan", shortreal_gt(0.0, fnan), False);
	runcheck("cond_f_gt_nan_nan", shortreal_gt(fnan, fnan), False);
	runcheck("cond_f_gt_0.0_0.0", shortreal_gt(0.0, 0.0), False);
	runcheck("cond_f_gt_1.0_0.0", shortreal_gt(1.0, 0.0), True);
	runcheck("cond_f_gt_0.0_1.0", shortreal_gt(0.0, 1.0), False);
	runcheck("cond_f_ngt_nan_0.0", shortreal_ngt(fnan, 0.0), True);
	runcheck("cond_f_ngt_0.0_nan", shortreal_ngt(0.0, fnan), True);
	runcheck("cond_f_ngt_nan_nan", shortreal_ngt(fnan, fnan), True);
	runcheck("cond_f_ngt_0.0_0.0", shortreal_ngt(0.0, 0.0), True);
	runcheck("cond_f_ngt_1.0_0.0", shortreal_ngt(1.0, 0.0), False);
	runcheck("cond_f_ngt_0.0_1.0", shortreal_ngt(0.0, 1.0), True);
	runcheck("cond_f_ge_nan_0.0", shortreal_ge(fnan, 0.0), False);
	runcheck("cond_f_ge_0.0_nan", shortreal_ge(0.0, fnan), False);
	runcheck("cond_f_ge_nan_nan", shortreal_ge(fnan, fnan), False);
	runcheck("cond_f_ge_0.0_0.0", shortreal_ge(0.0, 0.0), True);
	runcheck("cond_f_ge_1.0_0.0", shortreal_ge(1.0, 0.0), True);
	runcheck("cond_f_ge_0.0_1.0", shortreal_ge(0.0, 1.0), False);
	runcheck("cond_f_nge_nan_0.0", shortreal_nge(fnan, 0.0), True);
	runcheck("cond_f_nge_0.0_nan", shortreal_nge(0.0, fnan), True);
	runcheck("cond_f_nge_nan_nan", shortreal_nge(fnan, fnan), True);
	runcheck("cond_f_nge_0.0_0.0", shortreal_nge(0.0, 0.0), False);
	runcheck("cond_f_nge_1.0_0.0", shortreal_nge(1.0, 0.0), False);
	runcheck("cond_f_nge_0.0_1.0", shortreal_nge(0.0, 1.0), True);
	runcheck("cond_f_lt_nan_0.0", shortreal_lt(fnan, 0.0), False);
	runcheck("cond_f_lt_0.0_nan", shortreal_lt(0.0, fnan), False);
	runcheck("cond_f_lt_nan_nan", shortreal_lt(fnan, fnan), False);
	runcheck("cond_f_lt_0.0_0.0", shortreal_lt(0.0, 0.0), False);
	runcheck("cond_f_lt_1.0_0.0", shortreal_lt(1.0, 0.0), False);
	runcheck("cond_f_lt_0.0_1.0", shortreal_lt(0.0, 1.0), True);
	runcheck("cond_f_nlt_nan_0.0", shortreal_nlt(fnan, 0.0), True);
	runcheck("cond_f_nlt_0.0_nan", shortreal_nlt(0.0, fnan), True);
	runcheck("cond_f_nlt_nan_nan", shortreal_nlt(fnan, fnan), True);
	runcheck("cond_f_nlt_0.0_0.0", shortreal_nlt(0.0, 0.0), True);
	runcheck("cond_f_nlt_1.0_0.0", shortreal_nlt(1.0, 0.0), True);
	runcheck("cond_f_nlt_0.0_1.0", shortreal_nlt(0.0, 1.0), False);
	runcheck("cond_f_le_nan_0.0", shortreal_le(fnan, 0.0), False);
	runcheck("cond_f_le_0.0_nan", shortreal_le(0.0, fnan), False);
	runcheck("cond_f_le_nan_nan", shortreal_le(fnan, fnan), False);
	runcheck("cond_f_le_0.0_0.0", shortreal_le(0.0, 0.0), True);
	runcheck("cond_f_le_1.0_0.0", shortreal_le(1.0, 0.0), False);
	runcheck("cond_f_le_0.0_1.0", shortreal_le(0.0, 1.0), True);
	runcheck("cond_f_nle_nan_0.0", shortreal_nle(fnan, 0.0), True);
	runcheck("cond_f_nle_0.0_nan", shortreal_nle(0.0, fnan), True);
	runcheck("cond_f_nle_nan_nan", shortreal_nle(fnan, fnan), True);
	runcheck("cond_f_nle_0.0_0.0", shortreal_nle(0.0, 0.0), False);
	runcheck("cond_f_nle_1.0_0.0", shortreal_nle(1.0, 0.0), True);
	runcheck("cond_f_nle_0.0_1.0", shortreal_nle(0.0, 1.0), False);
end;

function real_eq(f1, f2: Real): Boolean;
begin
	real_eq := (f1 = f2);
end;

function real_ne(f1, f2: Real): Boolean;
begin
	real_ne := (f1 <> f2);
end;

function real_ge(f1, f2: Real): Boolean;
begin
	real_ge := (f1 >= f2);
end;

function real_gt(f1, f2: Real): Boolean;
begin
	real_gt := (f1 > f2);
end;

function real_le(f1, f2: Real): Boolean;
begin
	real_le := (f1 <= f2);
end;

function real_lt(f1, f2: Real): Boolean;
begin
	real_lt := (f1 < f2);
end;

function real_neq(f1, f2: Real): Boolean;
begin
	real_neq := not (f1 = f2);
end;

function real_nne(f1, f2: Real): Boolean;
begin
	real_nne := not (f1 <> f2);
end;

function real_nge(f1, f2: Real): Boolean;
begin
	real_nge := not (f1 >= f2);
end;

function real_ngt(f1, f2: Real): Boolean;
begin
	real_ngt := not (f1 > f2);
end;

function real_nle(f1, f2: Real): Boolean;
begin
	real_nle := not (f1 <= f2);
end;

function real_nlt(f1, f2: Real): Boolean;
begin
	real_nlt := not (f1 < f2);
end;

procedure real_tests;
var
	dnan :Real;
begin
	dnan := sqrt(Real(-1));

	runcheck("cond_d_eq_nan_0.0", real_eq(dnan, 0.0), False);
	runcheck("cond_d_eq_0.0_nan", real_eq(0.0, dnan), False);
	runcheck("cond_d_eq_nan_nan", real_eq(dnan, dnan), False);
	runcheck("cond_d_eq_0.0_0.0", real_eq(0.0, 0.0), True);
	runcheck("cond_d_eq_1.0_0.0", real_eq(1.0, 0.0), False);
	runcheck("cond_d_eq_0.0_1.0", real_eq(0.0, 1.0), False);
	runcheck("cond_d_neq_nan_0.0", real_neq(dnan, 0.0), True);
	runcheck("cond_d_neq_0.0_nan", real_neq(0.0, dnan), True);
	runcheck("cond_d_neq_nan_nan", real_neq(dnan, dnan), True);
	runcheck("cond_d_neq_0.0_0.0", real_neq(0.0, 0.0), False);
	runcheck("cond_d_neq_1.0_0.0", real_neq(1.0, 0.0), True);
	runcheck("cond_d_neq_0.0_1.0", real_neq(0.0, 1.0), True);
	runcheck("cond_d_ne_nan_0.0", real_ne(dnan, 0.0), True);
	runcheck("cond_d_ne_0.0_nan", real_ne(0.0, dnan), True);
	runcheck("cond_d_ne_nan_nan", real_ne(dnan, dnan), True);
	runcheck("cond_d_ne_0.0_0.0", real_ne(0.0, 0.0), False);
	runcheck("cond_d_ne_1.0_0.0", real_ne(1.0, 0.0), True);
	runcheck("cond_d_ne_0.0_1.0", real_ne(0.0, 1.0), True);
	runcheck("cond_d_nne_nan_0.0", real_nne(dnan, 0.0), False);
	runcheck("cond_d_nne_0.0_nan", real_nne(0.0, dnan), False);
	runcheck("cond_d_nne_nan_nan", real_nne(dnan, dnan), False);
	runcheck("cond_d_nne_0.0_0.0", real_nne(0.0, 0.0), True);
	runcheck("cond_d_nne_1.0_0.0", real_nne(1.0, 0.0), False);
	runcheck("cond_d_nne_0.0_1.0", real_nne(0.0, 1.0), False);
	runcheck("cond_d_gt_nan_0.0", real_gt(dnan, 0.0), False);
	runcheck("cond_d_gt_0.0_nan", real_gt(0.0, dnan), False);
	runcheck("cond_d_gt_nan_nan", real_gt(dnan, dnan), False);
	runcheck("cond_d_gt_0.0_0.0", real_gt(0.0, 0.0), False);
	runcheck("cond_d_gt_1.0_0.0", real_gt(1.0, 0.0), True);
	runcheck("cond_d_gt_0.0_1.0", real_gt(0.0, 1.0), False);
	runcheck("cond_d_ngt_nan_0.0", real_ngt(dnan, 0.0), True);
	runcheck("cond_d_ngt_0.0_nan", real_ngt(0.0, dnan), True);
	runcheck("cond_d_ngt_nan_nan", real_ngt(dnan, dnan), True);
	runcheck("cond_d_ngt_0.0_0.0", real_ngt(0.0, 0.0), True);
	runcheck("cond_d_ngt_1.0_0.0", real_ngt(1.0, 0.0), False);
	runcheck("cond_d_ngt_0.0_1.0", real_ngt(0.0, 1.0), True);
	runcheck("cond_d_ge_nan_0.0", real_ge(dnan, 0.0), False);
	runcheck("cond_d_ge_0.0_nan", real_ge(0.0, dnan), False);
	runcheck("cond_d_ge_nan_nan", real_ge(dnan, dnan), False);
	runcheck("cond_d_ge_0.0_0.0", real_ge(0.0, 0.0), True);
	runcheck("cond_d_ge_1.0_0.0", real_ge(1.0, 0.0), True);
	runcheck("cond_d_ge_0.0_1.0", real_ge(0.0, 1.0), False);
	runcheck("cond_d_nge_nan_0.0", real_nge(dnan, 0.0), True);
	runcheck("cond_d_nge_0.0_nan", real_nge(0.0, dnan), True);
	runcheck("cond_d_nge_nan_nan", real_nge(dnan, dnan), True);
	runcheck("cond_d_nge_0.0_0.0", real_nge(0.0, 0.0), False);
	runcheck("cond_d_nge_1.0_0.0", real_nge(1.0, 0.0), False);
	runcheck("cond_d_nge_0.0_1.0", real_nge(0.0, 1.0), True);
	runcheck("cond_d_lt_nan_0.0", real_lt(dnan, 0.0), False);
	runcheck("cond_d_lt_0.0_nan", real_lt(0.0, dnan), False);
	runcheck("cond_d_lt_nan_nan", real_lt(dnan, dnan), False);
	runcheck("cond_d_lt_0.0_0.0", real_lt(0.0, 0.0), False);
	runcheck("cond_d_lt_1.0_0.0", real_lt(1.0, 0.0), False);
	runcheck("cond_d_lt_0.0_1.0", real_lt(0.0, 1.0), True);
	runcheck("cond_d_nlt_nan_0.0", real_nlt(dnan, 0.0), True);
	runcheck("cond_d_nlt_0.0_nan", real_nlt(0.0, dnan), True);
	runcheck("cond_d_nlt_nan_nan", real_nlt(dnan, dnan), True);
	runcheck("cond_d_nlt_0.0_0.0", real_nlt(0.0, 0.0), True);
	runcheck("cond_d_nlt_1.0_0.0", real_nlt(1.0, 0.0), True);
	runcheck("cond_d_nlt_0.0_1.0", real_nlt(0.0, 1.0), False);
	runcheck("cond_d_le_nan_0.0", real_le(dnan, 0.0), False);
	runcheck("cond_d_le_0.0_nan", real_le(0.0, dnan), False);
	runcheck("cond_d_le_nan_nan", real_le(dnan, dnan), False);
	runcheck("cond_d_le_0.0_0.0", real_le(0.0, 0.0), True);
	runcheck("cond_d_le_1.0_0.0", real_le(1.0, 0.0), False);
	runcheck("cond_d_le_0.0_1.0", real_le(0.0, 1.0), True);
	runcheck("cond_d_nle_nan_0.0", real_nle(dnan, 0.0), True);
	runcheck("cond_d_nle_0.0_nan", real_nle(0.0, dnan), True);
	runcheck("cond_d_nle_nan_nan", real_nle(dnan, dnan), True);
	runcheck("cond_d_nle_0.0_0.0", real_nle(0.0, 0.0), False);
	runcheck("cond_d_nle_1.0_0.0", real_nle(1.0, 0.0), True);
	runcheck("cond_d_nle_0.0_1.0", real_nle(0.0, 1.0), False);

end;

function longreal_eq(f1, f2: LongReal): Boolean;
begin
	longreal_eq := (f1 = f2);
end;

function longreal_ne(f1, f2: LongReal): Boolean;
begin
	longreal_ne := (f1 <> f2);
end;

function longreal_ge(f1, f2: LongReal): Boolean;
begin
	longreal_ge := (f1 >= f2);
end;

function longreal_gt(f1, f2: LongReal): Boolean;
begin
	longreal_gt := (f1 > f2);
end;

function longreal_le(f1, f2: LongReal): Boolean;
begin
	longreal_le := (f1 <= f2);
end;

function longreal_lt(f1, f2: LongReal): Boolean;
begin
	longreal_lt := (f1 < f2);
end;

function longreal_neq(f1, f2: LongReal): Boolean;
begin
	longreal_neq := not (f1 = f2);
end;

function longreal_nne(f1, f2: LongReal): Boolean;
begin
	longreal_nne := not (f1 <> f2);
end;

function longreal_nge(f1, f2: LongReal): Boolean;
begin
	longreal_nge := not (f1 >= f2);
end;

function longreal_ngt(f1, f2: LongReal): Boolean;
begin
	longreal_ngt := not (f1 > f2);
end;

function longreal_nle(f1, f2: LongReal): Boolean;
begin
	longreal_nle := not (f1 <= f2);
end;

function longreal_nlt(f1, f2: LongReal): Boolean;
begin
	longreal_nlt := not (f1 < f2);
end;

procedure longreal_tests;
var
	nnan :LongReal;
begin
	nnan := sqrt(LongReal(-1));

	runcheck("cond_n_eq_nan_0.0", longreal_eq(nnan, 0.0), False);
	runcheck("cond_n_eq_0.0_nan", longreal_eq(0.0, nnan), False);
	runcheck("cond_n_eq_nan_nan", longreal_eq(nnan, nnan), False);
	runcheck("cond_n_eq_0.0_0.0", longreal_eq(0.0, 0.0), True);
	runcheck("cond_n_eq_1.0_0.0", longreal_eq(1.0, 0.0), False);
	runcheck("cond_n_eq_0.0_1.0", longreal_eq(0.0, 1.0), False);
	runcheck("cond_n_neq_nan_0.0", longreal_neq(nnan, 0.0), True);
	runcheck("cond_n_neq_0.0_nan", longreal_neq(0.0, nnan), True);
	runcheck("cond_n_neq_nan_nan", longreal_neq(nnan, nnan), True);
	runcheck("cond_n_neq_0.0_0.0", longreal_neq(0.0, 0.0), False);
	runcheck("cond_n_neq_1.0_0.0", longreal_neq(1.0, 0.0), True);
	runcheck("cond_n_neq_0.0_1.0", longreal_neq(0.0, 1.0), True);
	runcheck("cond_n_ne_nan_0.0", longreal_ne(nnan, 0.0), True);
	runcheck("cond_n_ne_0.0_nan", longreal_ne(0.0, nnan), True);
	runcheck("cond_n_ne_nan_nan", longreal_ne(nnan, nnan), True);
	runcheck("cond_n_ne_0.0_0.0", longreal_ne(0.0, 0.0), False);
	runcheck("cond_n_ne_1.0_0.0", longreal_ne(1.0, 0.0), True);
	runcheck("cond_n_ne_0.0_1.0", longreal_ne(0.0, 1.0), True);
	runcheck("cond_n_nne_nan_0.0", longreal_nne(nnan, 0.0), False);
	runcheck("cond_n_nne_0.0_nan", longreal_nne(0.0, nnan), False);
	runcheck("cond_n_nne_nan_nan", longreal_nne(nnan, nnan), False);
	runcheck("cond_n_nne_0.0_0.0", longreal_nne(0.0, 0.0), True);
	runcheck("cond_n_nne_1.0_0.0", longreal_nne(1.0, 0.0), False);
	runcheck("cond_n_nne_0.0_1.0", longreal_nne(0.0, 1.0), False);
	runcheck("cond_n_gt_nan_0.0", longreal_gt(nnan, 0.0), False);
	runcheck("cond_n_gt_0.0_nan", longreal_gt(0.0, nnan), False);
	runcheck("cond_n_gt_nan_nan", longreal_gt(nnan, nnan), False);
	runcheck("cond_n_gt_0.0_0.0", longreal_gt(0.0, 0.0), False);
	runcheck("cond_n_gt_1.0_0.0", longreal_gt(1.0, 0.0), True);
	runcheck("cond_n_gt_0.0_1.0", longreal_gt(0.0, 1.0), False);
	runcheck("cond_n_ngt_nan_0.0", longreal_ngt(nnan, 0.0), True);
	runcheck("cond_n_ngt_0.0_nan", longreal_ngt(0.0, nnan), True);
	runcheck("cond_n_ngt_nan_nan", longreal_ngt(nnan, nnan), True);
	runcheck("cond_n_ngt_0.0_0.0", longreal_ngt(0.0, 0.0), True);
	runcheck("cond_n_ngt_1.0_0.0", longreal_ngt(1.0, 0.0), False);
	runcheck("cond_n_ngt_0.0_1.0", longreal_ngt(0.0, 1.0), True);
	runcheck("cond_n_ge_nan_0.0", longreal_ge(nnan, 0.0), False);
	runcheck("cond_n_ge_0.0_nan", longreal_ge(0.0, nnan), False);
	runcheck("cond_n_ge_nan_nan", longreal_ge(nnan, nnan), False);
	runcheck("cond_n_ge_0.0_0.0", longreal_ge(0.0, 0.0), True);
	runcheck("cond_n_ge_1.0_0.0", longreal_ge(1.0, 0.0), True);
	runcheck("cond_n_ge_0.0_1.0", longreal_ge(0.0, 1.0), False);
	runcheck("cond_n_nge_nan_0.0", longreal_nge(nnan, 0.0), True);
	runcheck("cond_n_nge_0.0_nan", longreal_nge(0.0, nnan), True);
	runcheck("cond_n_nge_nan_nan", longreal_nge(nnan, nnan), True);
	runcheck("cond_n_nge_0.0_0.0", longreal_nge(0.0, 0.0), False);
	runcheck("cond_n_nge_1.0_0.0", longreal_nge(1.0, 0.0), False);
	runcheck("cond_n_nge_0.0_1.0", longreal_nge(0.0, 1.0), True);
	runcheck("cond_n_lt_nan_0.0", longreal_lt(nnan, 0.0), False);
	runcheck("cond_n_lt_0.0_nan", longreal_lt(0.0, nnan), False);
	runcheck("cond_n_lt_nan_nan", longreal_lt(nnan, nnan), False);
	runcheck("cond_n_lt_0.0_0.0", longreal_lt(0.0, 0.0), False);
	runcheck("cond_n_lt_1.0_0.0", longreal_lt(1.0, 0.0), False);
	runcheck("cond_n_lt_0.0_1.0", longreal_lt(0.0, 1.0), True);
	runcheck("cond_n_nlt_nan_0.0", longreal_nlt(nnan, 0.0), True);
	runcheck("cond_n_nlt_0.0_nan", longreal_nlt(0.0, nnan), True);
	runcheck("cond_n_nlt_nan_nan", longreal_nlt(nnan, nnan), True);
	runcheck("cond_n_nlt_0.0_0.0", longreal_nlt(0.0, 0.0), True);
	runcheck("cond_n_nlt_1.0_0.0", longreal_nlt(1.0, 0.0), True);
	runcheck("cond_n_nlt_0.0_1.0", longreal_nlt(0.0, 1.0), False);
	runcheck("cond_n_le_nan_0.0", longreal_le(nnan, 0.0), False);
	runcheck("cond_n_le_0.0_nan", longreal_le(0.0, nnan), False);
	runcheck("cond_n_le_nan_nan", longreal_le(nnan, nnan), False);
	runcheck("cond_n_le_0.0_0.0", longreal_le(0.0, 0.0), True);
	runcheck("cond_n_le_1.0_0.0", longreal_le(1.0, 0.0), False);
	runcheck("cond_n_le_0.0_1.0", longreal_le(0.0, 1.0), True);
	runcheck("cond_n_nle_nan_0.0", longreal_nle(nnan, 0.0), True);
	runcheck("cond_n_nle_0.0_nan", longreal_nle(0.0, nnan), True);
	runcheck("cond_n_nle_nan_nan", longreal_nle(nnan, nnan), True);
	runcheck("cond_n_nle_0.0_0.0", longreal_nle(0.0, 0.0), False);
	runcheck("cond_n_nle_1.0_0.0", longreal_nle(1.0, 0.0), True);
	runcheck("cond_n_nle_0.0_1.0", longreal_nle(0.0, 1.0), False);
end;

function shortreal_beq(f1, f2: ShortReal): Boolean;
begin
	if (f1 = f2) then begin
		shortreal_beq := True;
	end else begin
		shortreal_beq := False;
	end;
end;

function shortreal_bneq(f1, f2: ShortReal): Boolean;
begin
	if not (f1 = f2) then begin
		shortreal_bneq := True;
	end else begin
		shortreal_bneq := False;
	end;
end;

function shortreal_bne(f1, f2: ShortReal): Boolean;
begin
	if (f1 <> f2) then begin
		shortreal_bne := True;
	end else begin
		shortreal_bne := False;
	end;
end;

function shortreal_bnne(f1, f2: ShortReal): Boolean;
begin
	if not (f1 <> f2) then begin
		shortreal_bnne := True;
	end else begin
		shortreal_bnne := False;
	end;
end;

function shortreal_bgt(f1, f2: ShortReal): Boolean;
begin
	if (f1 > f2) then begin
		shortreal_bgt := True;
	end else begin
		shortreal_bgt := False;
	end;
end;

function shortreal_bngt(f1, f2: ShortReal): Boolean;
begin
	if not (f1 > f2) then begin
		shortreal_bngt := True;
	end else begin
		shortreal_bngt := False;
	end;
end;

function shortreal_bge(f1, f2: ShortReal): Boolean;
begin
	if (f1 >= f2) then begin
		shortreal_bge := True;
	end else begin
		shortreal_bge := False;
	end;
end;

function shortreal_bnge(f1, f2: ShortReal): Boolean;
begin
	if not (f1 >= f2) then begin
		shortreal_bnge := True;
	end else begin
		shortreal_bnge := False;
	end;
end;

function shortreal_blt(f1, f2: ShortReal): Boolean;
begin
	if (f1 < f2) then begin
		shortreal_blt := True;
	end else begin
		shortreal_blt := False;
	end;
end;

function shortreal_bnlt(f1, f2: ShortReal): Boolean;
begin
	if not (f1 < f2) then begin
		shortreal_bnlt := True;
	end else begin
		shortreal_bnlt := False;
	end;
end;

function shortreal_ble(f1, f2: ShortReal): Boolean;
begin
	if (f1 <= f2) then begin
		shortreal_ble := True;
	end else begin
		shortreal_ble := False;
	end;
end;

function shortreal_bnle(f1, f2: ShortReal): Boolean;
begin
	if not (f1 <= f2) then begin
		shortreal_bnle := True;
	end else begin
		shortreal_bnle := False;
	end;
end;

procedure shortreal_branch_tests;
var
	fnan :ShortReal;
begin
	fnan := sqrt(ShortReal(-1));

	runcheck("cond_f_beq_nan_0.0", shortreal_beq(fnan, 0.0), False);
	runcheck("cond_f_beq_0.0_nan", shortreal_beq(0.0, fnan), False);
	runcheck("cond_f_beq_nan_nan", shortreal_beq(fnan, fnan), False);
	runcheck("cond_f_beq_1.0_0.0", shortreal_beq(1.0, 0.0), False);
	runcheck("cond_f_beq_0.0_0.0", shortreal_beq(0.0, 0.0), True);
	runcheck("cond_f_beq_0.0_1.0", shortreal_beq(0.0, 1.0), False);
	runcheck("cond_f_bneq_nan_0.0", shortreal_bneq(fnan, 0.0), True);
	runcheck("cond_f_bneq_0.0_nan", shortreal_bneq(0.0, fnan), True);
	runcheck("cond_f_bneq_nan_nan", shortreal_bneq(fnan, fnan), True);
	runcheck("cond_f_bneq_1.0_0.0", shortreal_bneq(1.0, 0.0), True);
	runcheck("cond_f_bneq_0.0_0.0", shortreal_bneq(0.0, 0.0), False);
	runcheck("cond_f_bneq_0.0_1.0", shortreal_bneq(0.0, 1.0), True);

	runcheck("cond_f_bne_nan_0.0", shortreal_bne(fnan, 0.0), True);
	runcheck("cond_f_bne_0.0_nan", shortreal_bne(0.0, fnan), True);
	runcheck("cond_f_bne_nan_nan", shortreal_bne(fnan, fnan), True);
	runcheck("cond_f_bne_1.0_0.0", shortreal_bne(1.0, 0.0), True);
	runcheck("cond_f_bne_0.0_0.0", shortreal_bne(0.0, 0.0), False);
	runcheck("cond_f_bne_0.0_1.0", shortreal_bne(0.0, 1.0), True);
	runcheck("cond_f_bnne_nan_0.0", shortreal_bnne(fnan, 0.0), False);
	runcheck("cond_f_bnne_0.0_nan", shortreal_bnne(0.0, fnan), False);
	runcheck("cond_f_bnne_nan_nan", shortreal_bnne(fnan, fnan), False);
	runcheck("cond_f_bnne_1.0_0.0", shortreal_bnne(1.0, 0.0), False);
	runcheck("cond_f_bnne_0.0_0.0", shortreal_bnne(0.0, 0.0), True);
	runcheck("cond_f_bnne_0.0_1.0", shortreal_bnne(0.0, 1.0), False);

	runcheck("cond_f_bgt_nan_0.0", shortreal_bgt(fnan, 0.0), False);
	runcheck("cond_f_bgt_0.0_nan", shortreal_bgt(0.0, fnan), False);
	runcheck("cond_f_bgt_nan_nan", shortreal_bgt(fnan, fnan), False);
	runcheck("cond_f_bgt_0.0_0.0", shortreal_bgt(0.0, 0.0), False);
	runcheck("cond_f_bgt_1.0_0.0", shortreal_bgt(1.0, 0.0), True);
	runcheck("cond_f_bgt_0.0_1.0", shortreal_bgt(0.0, 1.0), False);
	runcheck("cond_f_bngt_nan_0.0", shortreal_bngt(fnan, 0.0), True);
	runcheck("cond_f_bngt_0.0_nan", shortreal_bngt(0.0, fnan), True);
	runcheck("cond_f_bngt_nan_nan", shortreal_bngt(fnan, fnan), True);
	runcheck("cond_f_bngt_0.0_0.0", shortreal_bngt(0.0, 0.0), True);
	runcheck("cond_f_bngt_1.0_0.0", shortreal_bngt(1.0, 0.0), False);
	runcheck("cond_f_bngt_0.0_1.0", shortreal_bngt(0.0, 1.0), True);

	runcheck("cond_f_bge_nan_0.0", shortreal_bge(fnan, 0.0), False);
	runcheck("cond_f_bge_0.0_nan", shortreal_bge(0.0, fnan), False);
	runcheck("cond_f_bge_nan_nan", shortreal_bge(fnan, fnan), False);
	runcheck("cond_f_bge_0.0_0.0", shortreal_bge(0.0, 0.0), True);
	runcheck("cond_f_bge_1.0_0.0", shortreal_bge(1.0, 0.0), True);
	runcheck("cond_f_bge_0.0_1.0", shortreal_bge(0.0, 1.0), False);
	runcheck("cond_f_bnge_nan_0.0", shortreal_bnge(fnan, 0.0), True);
	runcheck("cond_f_bnge_0.0_nan", shortreal_bnge(0.0, fnan), True);
	runcheck("cond_f_bnge_nan_nan", shortreal_bnge(fnan, fnan), True);
	runcheck("cond_f_bnge_0.0_0.0", shortreal_bnge(0.0, 0.0), False);
	runcheck("cond_f_bnge_1.0_0.0", shortreal_bnge(1.0, 0.0), False);
	runcheck("cond_f_bnge_0.0_1.0", shortreal_bnge(0.0, 1.0), True);

	runcheck("cond_f_blt_nan_0.0", shortreal_blt(fnan, 0.0), False);
	runcheck("cond_f_blt_0.0_nan", shortreal_blt(0.0, fnan), False);
	runcheck("cond_f_blt_nan_nan", shortreal_blt(fnan, fnan), False);
	runcheck("cond_f_blt_0.0_0.0", shortreal_blt(0.0, 0.0), False);
	runcheck("cond_f_blt_1.0_0.0", shortreal_blt(1.0, 0.0), False);
	runcheck("cond_f_blt_0.0_1.0", shortreal_blt(0.0, 1.0), True);
	runcheck("cond_f_bnlt_nan_0.0", shortreal_bnlt(fnan, 0.0), True);
	runcheck("cond_f_bnlt_0.0_nan", shortreal_bnlt(0.0, fnan), True);
	runcheck("cond_f_bnlt_nan_nan", shortreal_bnlt(fnan, fnan), True);
	runcheck("cond_f_bnlt_0.0_0.0", shortreal_bnlt(0.0, 0.0), True);
	runcheck("cond_f_bnlt_1.0_0.0", shortreal_bnlt(1.0, 0.0), True);
	runcheck("cond_f_bnlt_0.0_1.0", shortreal_bnlt(0.0, 1.0), False);

	runcheck("cond_f_ble_nan_0.0", shortreal_ble(fnan, 0.0), False);
	runcheck("cond_f_ble_0.0_nan", shortreal_ble(0.0, fnan), False);
	runcheck("cond_f_ble_nan_nan", shortreal_ble(fnan, fnan), False);
	runcheck("cond_f_ble_0.0_0.0", shortreal_ble(0.0, 0.0), True);
	runcheck("cond_f_ble_1.0_0.0", shortreal_ble(1.0, 0.0), False);
	runcheck("cond_f_ble_0.0_1.0", shortreal_ble(0.0, 1.0), True);
	runcheck("cond_f_bnle_nan_0.0", shortreal_bnle(fnan, 0.0), True);
	runcheck("cond_f_bnle_0.0_nan", shortreal_bnle(0.0, fnan), True);
	runcheck("cond_f_bnle_nan_nan", shortreal_bnle(fnan, fnan), True);
	runcheck("cond_f_bnle_0.0_0.0", shortreal_bnle(0.0, 0.0), False);
	runcheck("cond_f_bnle_1.0_0.0", shortreal_bnle(1.0, 0.0), True);
	runcheck("cond_f_bnle_0.0_1.0", shortreal_bnle(0.0, 1.0), False);
end;

function real_beq(d1, d2: Real): Boolean;
begin
	if (d1 = d2) then begin
		real_beq := True;
	end else begin
		real_beq := False;
	end;
end;

function real_bneq(d1, d2: Real): Boolean;
begin
	if not (d1 = d2) then begin
		real_bneq := True;
	end else begin
		real_bneq := False;
	end;
end;

function real_bne(d1, d2: Real): Boolean;
begin
	if (d1 <> d2) then begin
		real_bne := True;
	end else begin
		real_bne := False;
	end;
end;

function real_bnne(d1, d2: Real): Boolean;
begin
	if not (d1 <> d2) then begin
		real_bnne := True;
	end else begin
		real_bnne := False;
	end;
end;

function real_bgt(d1, d2: Real): Boolean;
begin
	if (d1 > d2) then begin
		real_bgt := True;
	end else begin
		real_bgt := False;
	end;
end;

function real_bngt(d1, d2: Real): Boolean;
begin
	if not (d1 > d2) then begin
		real_bngt := True;
	end else begin
		real_bngt := False;
	end;
end;

function real_bge(d1, d2: Real): Boolean;
begin
	if (d1 >= d2) then begin
		real_bge := True;
	end else begin
		real_bge := False;
	end;
end;

function real_bnge(d1, d2: Real): Boolean;
begin
	if not (d1 >= d2) then begin
		real_bnge := True;
	end else begin
		real_bnge := False;
	end;
end;

function real_blt(d1, d2: Real): Boolean;
begin
	if (d1 < d2) then begin
		real_blt := True;
	end else begin
		real_blt := False;
	end;
end;

function real_bnlt(d1, d2: Real): Boolean;
begin
	if not (d1 < d2) then begin
		real_bnlt := True;
	end else begin
		real_bnlt := False;
	end;
end;

function real_ble(d1, d2: Real): Boolean;
begin
	if (d1 <= d2) then begin
		real_ble := True;
	end else begin
		real_ble := False;
	end;
end;

function real_bnle(d1, d2: Real): Boolean;
begin
	if not (d1 <= d2) then begin
		real_bnle := True;
	end else begin
		real_bnle := False;
	end;
end;

procedure real_branch_tests;
var
	dnan :Real;
begin
	dnan := sqrt(Real(-1));

	runcheck("cond_d_beq_nan_0.0", real_beq(dnan, 0.0), False);
	runcheck("cond_d_beq_0.0_nan", real_beq(0.0, dnan), False);
	runcheck("cond_d_beq_nan_nan", real_beq(dnan, dnan), False);
	runcheck("cond_d_beq_1.0_0.0", real_beq(1.0, 0.0), False);
	runcheck("cond_d_beq_0.0_0.0", real_beq(0.0, 0.0), True);
	runcheck("cond_d_beq_0.0_1.0", real_beq(0.0, 1.0), False);
	runcheck("cond_d_bneq_nan_0.0", real_bneq(dnan, 0.0), True);
	runcheck("cond_d_bneq_0.0_nan", real_bneq(0.0, dnan), True);
	runcheck("cond_d_bneq_nan_nan", real_bneq(dnan, dnan), True);
	runcheck("cond_d_bneq_1.0_0.0", real_bneq(1.0, 0.0), True);
	runcheck("cond_d_bneq_0.0_0.0", real_bneq(0.0, 0.0), False);
	runcheck("cond_d_bneq_0.0_1.0", real_bneq(0.0, 1.0), True);

	runcheck("cond_d_bne_nan_0.0", real_bne(dnan, 0.0), True);
	runcheck("cond_d_bne_0.0_nan", real_bne(0.0, dnan), True);
	runcheck("cond_d_bne_nan_nan", real_bne(dnan, dnan), True);
	runcheck("cond_d_bne_1.0_0.0", real_bne(1.0, 0.0), True);
	runcheck("cond_d_bne_0.0_0.0", real_bne(0.0, 0.0), False);
	runcheck("cond_d_bne_0.0_1.0", real_bne(0.0, 1.0), True);
	runcheck("cond_d_bnne_nan_0.0", real_bnne(dnan, 0.0), False);
	runcheck("cond_d_bnne_0.0_nan", real_bnne(0.0, dnan), False);
	runcheck("cond_d_bnne_nan_nan", real_bnne(dnan, dnan), False);
	runcheck("cond_d_bnne_1.0_0.0", real_bnne(1.0, 0.0), False);
	runcheck("cond_d_bnne_0.0_0.0", real_bnne(0.0, 0.0), True);
	runcheck("cond_d_bnne_0.0_1.0", real_bnne(0.0, 1.0), False);

	runcheck("cond_d_bgt_nan_0.0", real_bgt(dnan, 0.0), False);
	runcheck("cond_d_bgt_0.0_nan", real_bgt(0.0, dnan), False);
	runcheck("cond_d_bgt_nan_nan", real_bgt(dnan, dnan), False);
	runcheck("cond_d_bgt_0.0_0.0", real_bgt(0.0, 0.0), False);
	runcheck("cond_d_bgt_1.0_0.0", real_bgt(1.0, 0.0), True);
	runcheck("cond_d_bgt_0.0_1.0", real_bgt(0.0, 1.0), False);
	runcheck("cond_d_bngt_nan_0.0", real_bngt(dnan, 0.0), True);
	runcheck("cond_d_bngt_0.0_nan", real_bngt(0.0, dnan), True);
	runcheck("cond_d_bngt_nan_nan", real_bngt(dnan, dnan), True);
	runcheck("cond_d_bngt_0.0_0.0", real_bngt(0.0, 0.0), True);
	runcheck("cond_d_bngt_1.0_0.0", real_bngt(1.0, 0.0), False);
	runcheck("cond_d_bngt_0.0_1.0", real_bngt(0.0, 1.0), True);

	runcheck("cond_d_bge_nan_0.0", real_bge(dnan, 0.0), False);
	runcheck("cond_d_bge_0.0_nan", real_bge(0.0, dnan), False);
	runcheck("cond_d_bge_nan_nan", real_bge(dnan, dnan), False);
	runcheck("cond_d_bge_0.0_0.0", real_bge(0.0, 0.0), True);
	runcheck("cond_d_bge_1.0_0.0", real_bge(1.0, 0.0), True);
	runcheck("cond_d_bge_0.0_1.0", real_bge(0.0, 1.0), False);
	runcheck("cond_d_bnge_nan_0.0", real_bnge(dnan, 0.0), True);
	runcheck("cond_d_bnge_0.0_nan", real_bnge(0.0, dnan), True);
	runcheck("cond_d_bnge_nan_nan", real_bnge(dnan, dnan), True);
	runcheck("cond_d_bnge_0.0_0.0", real_bnge(0.0, 0.0), False);
	runcheck("cond_d_bnge_1.0_0.0", real_bnge(1.0, 0.0), False);
	runcheck("cond_d_bnge_0.0_1.0", real_bnge(0.0, 1.0), True);

	runcheck("cond_d_blt_nan_0.0", real_blt(dnan, 0.0), False);
	runcheck("cond_d_blt_0.0_nan", real_blt(0.0, dnan), False);
	runcheck("cond_d_blt_nan_nan", real_blt(dnan, dnan), False);
	runcheck("cond_d_blt_0.0_0.0", real_blt(0.0, 0.0), False);
	runcheck("cond_d_blt_1.0_0.0", real_blt(1.0, 0.0), False);
	runcheck("cond_d_blt_0.0_1.0", real_blt(0.0, 1.0), True);
	runcheck("cond_d_bnlt_nan_0.0", real_bnlt(dnan, 0.0), True);
	runcheck("cond_d_bnlt_0.0_nan", real_bnlt(0.0, dnan), True);
	runcheck("cond_d_bnlt_nan_nan", real_bnlt(dnan, dnan), True);
	runcheck("cond_d_bnlt_0.0_0.0", real_bnlt(0.0, 0.0), True);
	runcheck("cond_d_bnlt_1.0_0.0", real_bnlt(1.0, 0.0), True);
	runcheck("cond_d_bnlt_0.0_1.0", real_bnlt(0.0, 1.0), False);

	runcheck("cond_d_ble_nan_0.0", real_ble(dnan, 0.0), False);
	runcheck("cond_d_ble_0.0_nan", real_ble(0.0, dnan), False);
	runcheck("cond_d_ble_nan_nan", real_ble(dnan, dnan), False);
	runcheck("cond_d_ble_0.0_0.0", real_ble(0.0, 0.0), True);
	runcheck("cond_d_ble_1.0_0.0", real_ble(1.0, 0.0), False);
	runcheck("cond_d_ble_0.0_1.0", real_ble(0.0, 1.0), True);
	runcheck("cond_d_bnle_nan_0.0", real_bnle(dnan, 0.0), True);
	runcheck("cond_d_bnle_0.0_nan", real_bnle(0.0, dnan), True);
	runcheck("cond_d_bnle_nan_nan", real_bnle(dnan, dnan), True);
	runcheck("cond_d_bnle_0.0_0.0", real_bnle(0.0, 0.0), False);
	runcheck("cond_d_bnle_1.0_0.0", real_bnle(1.0, 0.0), True);
	runcheck("cond_d_bnle_0.0_1.0", real_bnle(0.0, 1.0), False);
end;

function longreal_beq(n1, n2: LongReal): Boolean;
begin
	if (n1 = n2) then begin
		longreal_beq := True;
	end else begin
		longreal_beq := False;
	end;
end;

function longreal_bneq(n1, n2: LongReal): Boolean;
begin
	if not (n1 = n2) then begin
		longreal_bneq := True;
	end else begin
		longreal_bneq := False;
	end;
end;

function longreal_bne(n1, n2: LongReal): Boolean;
begin
	if (n1 <> n2) then begin
		longreal_bne := True;
	end else begin
		longreal_bne := False;
	end;
end;

function longreal_bnne(n1, n2: LongReal): Boolean;
begin
	if not (n1 <> n2) then begin
		longreal_bnne := True;
	end else begin
		longreal_bnne := False;
	end;
end;

function longreal_bgt(n1, n2: LongReal): Boolean;
begin
	if (n1 > n2) then begin
		longreal_bgt := True;
	end else begin
		longreal_bgt := False;
	end;
end;

function longreal_bngt(n1, n2: LongReal): Boolean;
begin
	if not (n1 > n2) then begin
		longreal_bngt := True;
	end else begin
		longreal_bngt := False;
	end;
end;

function longreal_bge(n1, n2: LongReal): Boolean;
begin
	if (n1 >= n2) then begin
		longreal_bge := True;
	end else begin
		longreal_bge := False;
	end;
end;

function longreal_bnge(n1, n2: LongReal): Boolean;
begin
	if not (n1 >= n2) then begin
		longreal_bnge := True;
	end else begin
		longreal_bnge := False;
	end;
end;

function longreal_blt(n1, n2: LongReal): Boolean;
begin
	if (n1 < n2) then begin
		longreal_blt := True;
	end else begin
		longreal_blt := False;
	end;
end;

function longreal_bnlt(n1, n2: LongReal): Boolean;
begin
	if not (n1 < n2) then begin
		longreal_bnlt := True;
	end else begin
		longreal_bnlt := False;
	end;
end;

function longreal_ble(n1, n2: LongReal): Boolean;
begin
	if (n1 <= n2) then begin
		longreal_ble := True;
	end else begin
		longreal_ble := False;
	end;
end;

function longreal_bnle(n1, n2: LongReal): Boolean;
begin
	if not (n1 <= n2) then begin
		longreal_bnle := True;
	end else begin
		longreal_bnle := False;
	end;
end;

procedure longreal_branch_tests;
var
	nnan :LongReal;
begin
	nnan := sqrt(LongReal(-1));

	runcheck("cond_n_beq_nan_0.0", longreal_beq(nnan, 0.0), False);
	runcheck("cond_n_beq_0.0_nan", longreal_beq(0.0, nnan), False);
	runcheck("cond_n_beq_nan_nan", longreal_beq(nnan, nnan), False);
	runcheck("cond_n_beq_1.0_0.0", longreal_beq(1.0, 0.0), False);
	runcheck("cond_n_beq_0.0_0.0", longreal_beq(0.0, 0.0), True);
	runcheck("cond_n_beq_0.0_1.0", longreal_beq(0.0, 1.0), False);
	runcheck("cond_n_bneq_nan_0.0", longreal_bneq(nnan, 0.0), True);
	runcheck("cond_n_bneq_0.0_nan", longreal_bneq(0.0, nnan), True);
	runcheck("cond_n_bneq_nan_nan", longreal_bneq(nnan, nnan), True);
	runcheck("cond_n_bneq_1.0_0.0", longreal_bneq(1.0, 0.0), True);
	runcheck("cond_n_bneq_0.0_0.0", longreal_bneq(0.0, 0.0), False);
	runcheck("cond_n_bneq_0.0_1.0", longreal_bneq(0.0, 1.0), True);

	runcheck("cond_n_bne_nan_0.0", longreal_bne(nnan, 0.0), True);
	runcheck("cond_n_bne_0.0_nan", longreal_bne(0.0, nnan), True);
	runcheck("cond_n_bne_nan_nan", longreal_bne(nnan, nnan), True);
	runcheck("cond_n_bne_1.0_0.0", longreal_bne(1.0, 0.0), True);
	runcheck("cond_n_bne_0.0_0.0", longreal_bne(0.0, 0.0), False);
	runcheck("cond_n_bne_0.0_1.0", longreal_bne(0.0, 1.0), True);
	runcheck("cond_n_bnne_nan_0.0", longreal_bnne(nnan, 0.0), False);
	runcheck("cond_n_bnne_0.0_nan", longreal_bnne(0.0, nnan), False);
	runcheck("cond_n_bnne_nan_nan", longreal_bnne(nnan, nnan), False);
	runcheck("cond_n_bnne_1.0_0.0", longreal_bnne(1.0, 0.0), False);
	runcheck("cond_n_bnne_0.0_0.0", longreal_bnne(0.0, 0.0), True);
	runcheck("cond_n_bnne_0.0_1.0", longreal_bnne(0.0, 1.0), False);

	runcheck("cond_n_bgt_nan_0.0", longreal_bgt(nnan, 0.0), False);
	runcheck("cond_n_bgt_0.0_nan", longreal_bgt(0.0, nnan), False);
	runcheck("cond_n_bgt_nan_nan", longreal_bgt(nnan, nnan), False);
	runcheck("cond_n_bgt_0.0_0.0", longreal_bgt(0.0, 0.0), False);
	runcheck("cond_n_bgt_1.0_0.0", longreal_bgt(1.0, 0.0), True);
	runcheck("cond_n_bgt_0.0_1.0", longreal_bgt(0.0, 1.0), False);
	runcheck("cond_n_bngt_nan_0.0", longreal_bngt(nnan, 0.0), True);
	runcheck("cond_n_bngt_0.0_nan", longreal_bngt(0.0, nnan), True);
	runcheck("cond_n_bngt_nan_nan", longreal_bngt(nnan, nnan), True);
	runcheck("cond_n_bngt_0.0_0.0", longreal_bngt(0.0, 0.0), True);
	runcheck("cond_n_bngt_1.0_0.0", longreal_bngt(1.0, 0.0), False);
	runcheck("cond_n_bngt_0.0_1.0", longreal_bngt(0.0, 1.0), True);

	runcheck("cond_n_bge_nan_0.0", longreal_bge(nnan, 0.0), False);
	runcheck("cond_n_bge_0.0_nan", longreal_bge(0.0, nnan), False);
	runcheck("cond_n_bge_nan_nan", longreal_bge(nnan, nnan), False);
	runcheck("cond_n_bge_0.0_0.0", longreal_bge(0.0, 0.0), True);
	runcheck("cond_n_bge_1.0_0.0", longreal_bge(1.0, 0.0), True);
	runcheck("cond_n_bge_0.0_1.0", longreal_bge(0.0, 1.0), False);
	runcheck("cond_n_bnge_nan_0.0", longreal_bnge(nnan, 0.0), True);
	runcheck("cond_n_bnge_0.0_nan", longreal_bnge(0.0, nnan), True);
	runcheck("cond_n_bnge_nan_nan", longreal_bnge(nnan, nnan), True);
	runcheck("cond_n_bnge_0.0_0.0", longreal_bnge(0.0, 0.0), False);
	runcheck("cond_n_bnge_1.0_0.0", longreal_bnge(1.0, 0.0), False);
	runcheck("cond_n_bnge_0.0_1.0", longreal_bnge(0.0, 1.0), True);

	runcheck("cond_n_blt_nan_0.0", longreal_blt(nnan, 0.0), False);
	runcheck("cond_n_blt_0.0_nan", longreal_blt(0.0, nnan), False);
	runcheck("cond_n_blt_nan_nan", longreal_blt(nnan, nnan), False);
	runcheck("cond_n_blt_0.0_0.0", longreal_blt(0.0, 0.0), False);
	runcheck("cond_n_blt_1.0_0.0", longreal_blt(1.0, 0.0), False);
	runcheck("cond_n_blt_0.0_1.0", longreal_blt(0.0, 1.0), True);
	runcheck("cond_n_bnlt_nan_0.0", longreal_bnlt(nnan, 0.0), True);
	runcheck("cond_n_bnlt_0.0_nan", longreal_bnlt(0.0, nnan), True);
	runcheck("cond_n_bnlt_nan_nan", longreal_bnlt(nnan, nnan), True);
	runcheck("cond_n_bnlt_0.0_0.0", longreal_bnlt(0.0, 0.0), True);
	runcheck("cond_n_bnlt_1.0_0.0", longreal_bnlt(1.0, 0.0), True);
	runcheck("cond_n_bnlt_0.0_1.0", longreal_bnlt(0.0, 1.0), False);

	runcheck("cond_n_ble_nan_0.0", longreal_ble(nnan, 0.0), False);
	runcheck("cond_n_ble_0.0_nan", longreal_ble(0.0, nnan), False);
	runcheck("cond_n_ble_nan_nan", longreal_ble(nnan, nnan), False);
	runcheck("cond_n_ble_0.0_0.0", longreal_ble(0.0, 0.0), True);
	runcheck("cond_n_ble_1.0_0.0", longreal_ble(1.0, 0.0), False);
	runcheck("cond_n_ble_0.0_1.0", longreal_ble(0.0, 1.0), True);
	runcheck("cond_n_bnle_nan_0.0", longreal_bnle(nnan, 0.0), True);
	runcheck("cond_n_bnle_0.0_nan", longreal_bnle(0.0, nnan), True);
	runcheck("cond_n_bnle_nan_nan", longreal_bnle(nnan, nnan), True);
	runcheck("cond_n_bnle_0.0_0.0", longreal_bnle(0.0, 0.0), False);
	runcheck("cond_n_bnle_1.0_0.0", longreal_bnle(1.0, 0.0), True);
	runcheck("cond_n_bnle_0.0_1.0", longreal_bnle(0.0, 1.0), False);
end;

procedure run_tests;
begin
	const_integer_tests;
	integer_tests;
	cardinal_tests;
	longint_tests;
	longcard_tests;
	shortreal_tests;
	shortreal_branch_tests;
	real_tests;
	real_branch_tests;
	longreal_tests;
	longreal_branch_tests;
end;

begin
	failed := False;
	run_tests;
	if failed then begin
		Terminate(1);
	end;
end.
