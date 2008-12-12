require 'jit/array'
require 'jit/function'
require 'test/unit'
require 'assertions'

class TestJitArray < Test::Unit::TestCase
  include JitAssertions

  def test_new_array
    a_type = JIT::Array.new(JIT::Type::INT, 12)
    assert_equal JIT::Type::INT, a_type.type
    assert_equal 12, a_type.length
  end

  # TODO: wrap

  def test_create
    p = proc { |f|
      a_type = JIT::Array.new(JIT::Type::INT, 4)
      a = a_type.create(f)
      f.return f.const(JIT::Type::INT, 0)
    }
    assert_function_result(
        :result => [ JIT::Type::INT, 0 ],
        &p)
  end

  def test_offset_of
    a_type = JIT::Array.new(JIT::Type::INT, 4)
    assert_equal 0, a_type.offset_of(0)
    assert_equal 4, a_type.offset_of(1)
    assert_equal 8, a_type.offset_of(2)
    assert_equal 12, a_type.offset_of(3)
    # TODO: check out of bounds
  end

  def test_type_of
    a_type = JIT::Array.new(JIT::Type::INT, 4)
    assert_equal JIT::Type::INT, a_type.type_of(0)
    assert_equal JIT::Type::INT, a_type.type_of(1)
    assert_equal JIT::Type::INT, a_type.type_of(2)
    assert_equal JIT::Type::INT, a_type.type_of(3)
    # TODO: check out of bounds
  end

  def test_instance_bracket
    p = proc { |f|
      a_type = JIT::Array.new(JIT::Type::INT, 4)
      a = a_type.create(f)
      f.insn_store_relative(a.ptr, 4, f.const(JIT::Type::INT, 42))
      f.return a[1]
    }
    assert_function_result(
        :result => [ JIT::Type::INT, 42 ],
        &p)
  end

  def test_instance_bracket_eq
    p = proc { |f|
      a_type = JIT::Array.new(JIT::Type::INT, 4)
      a = a_type.create(f)
      a[1] = f.const(JIT::Type::INT, 42)
      f.return a[1]
    }
    assert_function_result(
        :result => [ JIT::Type::INT, 42 ],
        &p)
  end
end

