require 'jit/pointer'
require 'jit/array'
require 'jit/function'
require 'test/unit'
require 'assertions'

class TestJitArray < Test::Unit::TestCase
  include JitAssertions

  def test_new_pointer
    p_type = JIT::Pointer.new(JIT::Type::INT)
    assert_equal JIT::Type::INT, p_type.type
  end

  # TODO: wrap

  def test_offset_of
    p_type = JIT::Pointer.new(JIT::Type::INT)
    assert_equal 0, p_type.offset_of(0)
    assert_equal 4, p_type.offset_of(1)
    assert_equal 8, p_type.offset_of(2)
    assert_equal 12, p_type.offset_of(3)
    # TODO: check out of bounds
  end

  def test_type_of
    p_type = JIT::Pointer.new(JIT::Type::INT)
    assert_equal JIT::Type::INT, p_type.type_of(0)
    assert_equal JIT::Type::INT, p_type.type_of(1)
    assert_equal JIT::Type::INT, p_type.type_of(2)
    assert_equal JIT::Type::INT, p_type.type_of(3)
    # TODO: check out of bounds
  end

  def test_instance_bracket
    p = proc { |f|
      a_type = JIT::Array.new(JIT::Type::INT, 4)
      p_type = JIT::Pointer.new(JIT::Type::INT)
      a = a_type.create(f)
      ptr = p_type.wrap(a.ptr)
      f.insn_store_relative(a.ptr, 4, f.const(JIT::Type::INT, 42))
      f.return ptr[1]
    }
    assert_function_result(
        :result => [ JIT::Type::INT, 42 ],
        &p)
  end

  def test_instance_bracket_eq
    p = proc { |f|
      a_type = JIT::Array.new(JIT::Type::INT, 4)
      p_type = JIT::Pointer.new(JIT::Type::INT)
      a = a_type.create(f)
      ptr = p_type.wrap(a.ptr)
      ptr[1] = f.const(JIT::Type::INT, 42)
      f.return a[1]
    }
    assert_function_result(
        :result => [ JIT::Type::INT, 42 ],
        &p)
  end
end

