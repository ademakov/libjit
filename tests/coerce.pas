(*
 * coerce.pas - Test type coercion of the primitive operators.
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

	{ Test libjit's coercion rules for binary operators }
	run("coerce_byte_short", SameType(Integer, b / s));
	run("coerce_int_byte", SameType(Integer, 3 + b));
	run("coerce_byte_uint", SameType(Cardinal, b * 080000000H));
	run("coerce_int_short", SameType(Integer, 3 + s));
	run("coerce_short_uint", SameType(Integer, s * 080000000H));
	run("coerce_int_int", SameType(Integer, 3 + 4));
	run("coerce_int_uint", SameType(Integer, 3 - 0FFFFFFFFH));
	run("coerce_uint_int", SameType(Integer, 0FFFFFFFFH mod 3));
	run("coerce_uint_int_shift", SameType(Cardinal, 0FFFFFFFFH shr 3));
	run("coerce_uint_uint", SameType(Cardinal, 080000000H + 0FFFFFFFFH));
	run("coerce_int_long", SameType(LongInt, 3 / 07FFFFFFFFFFFH));
	run("coerce_long_int", SameType(LongInt, 07FFFFFFFFFFFH * 3));
	run("coerce_long_uint", SameType(LongInt, 07FFFFFFFFFFFH * 0FFFFFFFFH));
	run("coerce_uint_ulong",
		SameType(LongCard, 0FFFFFFFFH + 08000000000000000H));
	run("coerce_int_ulong",
		SameType(LongInt, 07FFFFFFFH mod 08000000000000000H));
	run("coerce_int_float32", SameType(ShortReal, 23 * f));
	run("coerce_float32_uint", SameType(ShortReal, f / 0FFFFFFFFH));
	run("coerce_float32_long", SameType(ShortReal, f + 0FFFFFFFF0H));
	run("coerce_ulong_float32", SameType(ShortReal, 08000000000000000 mod f));
	run("coerce_float32_float32", SameType(ShortReal, f mod f));
	run("coerce_int_float64", SameType(Real, 23 * d));
	run("coerce_float64_uint", SameType(Real, d / 0FFFFFFFFH));
	run("coerce_float64_long", SameType(Real, d + 0FFFFFFFF0H));
	run("coerce_ulong_float64", SameType(Real, 08000000000000000 mod d));
	run("coerce_float32_float64", SameType(Real, f mod d));
	run("coerce_float64_float32", SameType(Real, d - f));
	run("coerce_float64_float64", SameType(Real, d * d));
	run("coerce_int_nfloat", SameType(LongReal, 23 * n));
	run("coerce_nfloat_uint", SameType(LongReal, n / 0FFFFFFFFH));
	run("coerce_nfloat_long", SameType(LongReal, n + 0FFFFFFFF0H));
	run("coerce_ulong_nfloat", SameType(LongReal, 08000000000000000 mod n));
	run("coerce_float32_nfloat", SameType(LongReal, f mod n));
	run("coerce_nfloat_float32", SameType(LongReal, n - f));
	run("coerce_float64_nfloat", SameType(LongReal, d mod n));
	run("coerce_nfloat_float64", SameType(LongReal, n - d));
	run("coerce_nfloat_nfloat", SameType(LongReal, n * n));

	{ Test libjit's coercion rules for unary operators }
	run("coerce_neg_byte", SameType(Integer, -b));
	run("coerce_neg_short", SameType(Integer, -s));
	run("coerce_neg_int", SameType(Integer, -3));
	run("coerce_neg_uint", SameType(Integer, -080000000H));
	run("coerce_neg_long", SameType(LongInt, -0800000000H));
	run("coerce_neg_ulong", SameType(LongInt, -08000000000000000H));
	run("coerce_neg_float32", SameType(ShortReal, -f));
	run("coerce_neg_float64", SameType(Real, -d));
	run("coerce_neg_nfloat", SameType(LongReal, -n));
	run("coerce_not_byte", SameType(Cardinal, not b));
	run("coerce_not_short", SameType(Integer, not s));
	run("coerce_not_int", SameType(Integer, not 3));
	run("coerce_not_uint", SameType(Cardinal, not 080000000H));
	run("coerce_not_long", SameType(LongInt, not 0800000000H));
	run("coerce_not_ulong", SameType(LongCard, not 08000000000000000H));
end;

begin
	failed := False;
	run_tests;
	if failed then begin
		Terminate(1);
	end;
end.
