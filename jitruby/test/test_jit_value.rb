require 'jit/value'
require 'jit/function'
require 'test/unit'
require 'assertions'

class TestJitValue < Test::Unit::TestCase
  include JitAssertions

  def test_store
    p = proc { |f|
      v = f.value(JIT::Type::INT)
      v.store(f.const(JIT::Type::INT, 42))
      f.return v
    }
    assert_function_result(
        :result => [ JIT::Type::INT, 42 ],
        &p)
  end

  # TODO: address

  def test_int_plus
    p = proc { |f|
      v1 = f.get_param(0)
      v2 = f.get_param(1)
      f.return v1 + v2
    }
    assert_function_result(
        :arg0 => [ JIT::Type::INT, 1 ],
        :arg1 => [ JIT::Type::INT, 2 ],
        :result => [ JIT::Type::INT, 3 ],
        &p)
  end

  def test_int_minus
    p = proc { |f|
      v1 = f.get_param(0)
      v2 = f.get_param(1)
      f.return v1 - v2
    }
    assert_function_result(
        :arg0 => [ JIT::Type::INT, 3 ],
        :arg1 => [ JIT::Type::INT, 2 ],
        :result => [ JIT::Type::INT, 1 ],
        &p)
  end

  def test_int_mult
    p = proc { |f|
      v1 = f.get_param(0)
      v2 = f.get_param(1)
      f.return v1 * v2
    }
    assert_function_result(
        :arg0 => [ JIT::Type::INT, 3 ],
        :arg1 => [ JIT::Type::INT, 2 ],
        :result => [ JIT::Type::INT, 6 ],
        &p)
  end

  def test_int_div
    p = proc { |f|
      v1 = f.get_param(0)
      v2 = f.get_param(1)
      f.return v1 / v2
    }
    assert_function_result(
        :arg0 => [ JIT::Type::INT, 6 ],
        :arg1 => [ JIT::Type::INT, 2 ],
        :result => [ JIT::Type::INT, 3 ],
        &p)
  end

  def test_int_mod
    p = proc { |f|
      v1 = f.get_param(0)
      v2 = f.get_param(1)
      f.return v1 % v2
    }
    assert_function_result(
        :arg0 => [ JIT::Type::INT, 20 ],
        :arg1 => [ JIT::Type::INT, 6 ],
        :result => [ JIT::Type::INT, 2 ],
        &p)
  end

  def test_int_and
    p = proc { |f|
      v1 = f.get_param(0)
      v2 = f.get_param(1)
      f.return v1 & v2
    }
    assert_function_result(
        :arg0 => [ JIT::Type::INT, 11 ],
        :arg1 => [ JIT::Type::INT, 3 ],
        :result => [ JIT::Type::INT, 3 ],
        &p)
    assert_function_result(
        :arg0 => [ JIT::Type::INT, 8 ],
        :arg1 => [ JIT::Type::INT, 3 ],
        :result => [ JIT::Type::INT, 0 ],
        &p)
  end

  def test_int_or
    p = proc { |f|
      v1 = f.get_param(0)
      v2 = f.get_param(1)
      f.return v1 | v2
    }
    assert_function_result(
        :arg0 => [ JIT::Type::INT, 10 ],
        :arg1 => [ JIT::Type::INT, 3 ],
        :result => [ JIT::Type::INT, 11 ],
        &p)
  end

  def test_int_xor
    p = proc { |f|
      v1 = f.get_param(0)
      v2 = f.get_param(1)
      f.return v1 ^ v2
    }
    assert_function_result(
        :arg0 => [ JIT::Type::INT, 10 ],
        :arg1 => [ JIT::Type::INT, 3 ],
        :result => [ JIT::Type::INT, 9 ],
        &p)
  end

  def test_int_lt
    p = proc { |f|
      v1 = f.get_param(0)
      v2 = f.get_param(1)
      f.return v1 < v2
    }
    assert_function_result(
        :arg0 => [ JIT::Type::INT, 1 ],
        :arg1 => [ JIT::Type::INT, 2 ],
        :result => [ JIT::Type::INT, 1 ],
        &p)
    assert_function_result(
        :arg0 => [ JIT::Type::INT, 2 ],
        :arg1 => [ JIT::Type::INT, 1 ],
        :result => [ JIT::Type::INT, 0 ],
        &p)
  end

  def test_int_gt
    p = proc { |f|
      v1 = f.get_param(0)
      v2 = f.get_param(1)
      f.return v1 > v2
    }
    assert_function_result(
        :arg0 => [ JIT::Type::INT, 1 ],
        :arg1 => [ JIT::Type::INT, 2 ],
        :result => [ JIT::Type::INT, 0 ],
        &p)
    assert_function_result(
        :arg0 => [ JIT::Type::INT, 2 ],
        :arg1 => [ JIT::Type::INT, 1 ],
        :result => [ JIT::Type::INT, 1 ],
        &p)
  end

  def test_int_eq
    p = proc { |f|
      v1 = f.get_param(0)
      v2 = f.get_param(1)
      f.return v1 == v2
    }
    assert_function_result(
        :arg0 => [ JIT::Type::INT, 1 ],
        :arg1 => [ JIT::Type::INT, 2 ],
        :result => [ JIT::Type::INT, 0 ],
        &p)
    assert_function_result(
        :arg0 => [ JIT::Type::INT, 1 ],
        :arg1 => [ JIT::Type::INT, 1 ],
        :result => [ JIT::Type::INT, 1 ],
        &p)
  end

  def test_int_le
    p = proc { |f|
      v1 = f.get_param(0)
      v2 = f.get_param(1)
      f.return v1 <= v2
    }
    assert_function_result(
        :arg0 => [ JIT::Type::INT, 1 ],
        :arg1 => [ JIT::Type::INT, 2 ],
        :result => [ JIT::Type::INT, 1 ],
        &p)
    assert_function_result(
        :arg0 => [ JIT::Type::INT, 2 ],
        :arg1 => [ JIT::Type::INT, 1 ],
        :result => [ JIT::Type::INT, 0 ],
        &p)
    assert_function_result(
        :arg0 => [ JIT::Type::INT, 1 ],
        :arg1 => [ JIT::Type::INT, 1 ],
        :result => [ JIT::Type::INT, 1 ],
        &p)
  end

  def test_int_ge
    p = proc { |f|
      v1 = f.get_param(0)
      v2 = f.get_param(1)
      f.return v1 >= v2
    }
    assert_function_result(
        :arg0 => [ JIT::Type::INT, 1 ],
        :arg1 => [ JIT::Type::INT, 2 ],
        :result => [ JIT::Type::INT, 0 ],
        &p)
    assert_function_result(
        :arg0 => [ JIT::Type::INT, 2 ],
        :arg1 => [ JIT::Type::INT, 1 ],
        :result => [ JIT::Type::INT, 1 ],
        &p)
    assert_function_result(
        :arg0 => [ JIT::Type::INT, 1 ],
        :arg1 => [ JIT::Type::INT, 1 ],
        :result => [ JIT::Type::INT, 1 ],
        &p)
  end

  def test_int_lshift
    p = proc { |f|
      v1 = f.get_param(0)
      v2 = f.get_param(1)
      f.return v1 << v2
    }
    assert_function_result(
        :arg0 => [ JIT::Type::INT, 31 ],
        :arg1 => [ JIT::Type::INT, 2 ],
        :result => [ JIT::Type::INT, 124 ],
        &p)
  end

  def test_int_rshift
    p = proc { |f|
      v1 = f.get_param(0)
      v2 = f.get_param(1)
      f.return v1 >> v2
    }
    assert_function_result(
        :arg0 => [ JIT::Type::INT, 31 ],
        :arg1 => [ JIT::Type::INT, 2 ],
        :result => [ JIT::Type::INT, 7 ],
        &p)
  end
end


