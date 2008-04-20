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

procedure shortreal_tests;
begin
	runcheck("cond_f_eq_0.0_0.0", shortreal_eq(0.0, 0.0), True);
	runcheck("cond_f_eq_1.0_0.0", shortreal_eq(1.0, 0.0), False);
	runcheck("cond_f_eq_0.0_1.0", shortreal_eq(0.0, 1.0), False);
	runcheck("cond_f_ne_0.0_0.0", shortreal_ne(0.0, 0.0), False);
	runcheck("cond_f_ne_1.0_0.0", shortreal_ne(1.0, 0.0), True);
	runcheck("cond_f_ne_0.0_1.0", shortreal_ne(0.0, 1.0), True);
	runcheck("cond_f_gt_0.0_0.0", shortreal_gt(0.0, 0.0), False);
	runcheck("cond_f_gt_1.0_0.0", shortreal_gt(1.0, 0.0), True);
	runcheck("cond_f_gt_0.0_1.0", shortreal_gt(0.0, 1.0), False);
	runcheck("cond_f_ge_0.0_0.0", shortreal_ge(0.0, 0.0), true);
	runcheck("cond_f_ge_1.0_0.0", shortreal_ge(1.0, 0.0), True);
	runcheck("cond_f_ge_0.0_1.0", shortreal_ge(0.0, 1.0), False);
	runcheck("cond_f_lt_0.0_0.0", shortreal_lt(0.0, 0.0), False);
	runcheck("cond_f_lt_1.0_0.0", shortreal_lt(1.0, 0.0), False);
	runcheck("cond_f_lt_0.0_1.0", shortreal_lt(0.0, 1.0), True);
	runcheck("cond_f_le_0.0_0.0", shortreal_le(0.0, 0.0), True);
	runcheck("cond_f_le_1.0_0.0", shortreal_le(1.0, 0.0), False);
	runcheck("cond_f_le_0.0_1.0", shortreal_le(0.0, 1.0), True);
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

procedure real_tests;
begin
	runcheck("cond_d_eq_0.0_0.0", real_eq(0.0, 0.0), True);
	runcheck("cond_d_eq_1.0_0.0", real_eq(1.0, 0.0), False);
	runcheck("cond_d_eq_0.0_1.0", real_eq(0.0, 1.0), False);
	runcheck("cond_d_ne_0.0_0.0", real_ne(0.0, 0.0), False);
	runcheck("cond_d_ne_1.0_0.0", real_ne(1.0, 0.0), True);
	runcheck("cond_d_ne_0.0_1.0", real_ne(0.0, 1.0), True);
	runcheck("cond_d_gt_0.0_0.0", real_gt(0.0, 0.0), False);
	runcheck("cond_d_gt_1.0_0.0", real_gt(1.0, 0.0), True);
	runcheck("cond_d_gt_0.0_1.0", real_gt(0.0, 1.0), False);
	runcheck("cond_d_ge_0.0_0.0", real_ge(0.0, 0.0), true);
	runcheck("cond_d_ge_1.0_0.0", real_ge(1.0, 0.0), True);
	runcheck("cond_d_ge_0.0_1.0", real_ge(0.0, 1.0), False);
	runcheck("cond_d_lt_0.0_0.0", real_lt(0.0, 0.0), False);
	runcheck("cond_d_lt_1.0_0.0", real_lt(1.0, 0.0), False);
	runcheck("cond_d_lt_0.0_1.0", real_lt(0.0, 1.0), True);
	runcheck("cond_d_le_0.0_0.0", real_le(0.0, 0.0), True);
	runcheck("cond_d_le_1.0_0.0", real_le(1.0, 0.0), False);
	runcheck("cond_d_le_0.0_1.0", real_le(0.0, 1.0), True);
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

procedure longreal_tests;
begin
	runcheck("cond_n_eq_0.0_0.0", longreal_eq(0.0, 0.0), True);
	runcheck("cond_n_eq_1.0_0.0", longreal_eq(1.0, 0.0), False);
	runcheck("cond_n_eq_0.0_1.0", longreal_eq(0.0, 1.0), False);
	runcheck("cond_n_ne_0.0_0.0", longreal_ne(0.0, 0.0), False);
	runcheck("cond_n_ne_1.0_0.0", longreal_ne(1.0, 0.0), True);
	runcheck("cond_n_ne_0.0_1.0", longreal_ne(0.0, 1.0), True);
	runcheck("cond_n_gt_0.0_0.0", longreal_gt(0.0, 0.0), False);
	runcheck("cond_n_gt_1.0_0.0", longreal_gt(1.0, 0.0), True);
	runcheck("cond_n_gt_0.0_1.0", longreal_gt(0.0, 1.0), False);
	runcheck("cond_n_ge_0.0_0.0", longreal_ge(0.0, 0.0), true);
	runcheck("cond_n_ge_1.0_0.0", longreal_ge(1.0, 0.0), True);
	runcheck("cond_n_ge_0.0_1.0", longreal_ge(0.0, 1.0), False);
	runcheck("cond_n_lt_0.0_0.0", longreal_lt(0.0, 0.0), False);
	runcheck("cond_n_lt_1.0_0.0", longreal_lt(1.0, 0.0), False);
	runcheck("cond_n_lt_0.0_1.0", longreal_lt(0.0, 1.0), True);
	runcheck("cond_n_le_0.0_0.0", longreal_le(0.0, 0.0), True);
	runcheck("cond_n_le_1.0_0.0", longreal_le(1.0, 0.0), False);
	runcheck("cond_n_le_0.0_1.0", longreal_le(0.0, 1.0), True);
end;

procedure run_tests;
begin
	const_integer_tests;
	integer_tests;
	cardinal_tests;
	longint_tests;
	longcard_tests;
	shortreal_tests;
	real_tests;
	longreal_tests;
end;

begin
	failed := False;
	run_tests;
	if failed then begin
		Terminate(1);
	end;
end.
