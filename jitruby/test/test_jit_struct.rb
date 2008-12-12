require 'jit/array'
require 'jit/function'
require 'test/unit'
require 'assertions'

class TestJitStruct < Test::Unit::TestCase
  include JitAssertions

  def test_new_struct
    s_type = JIT::Struct.new(
        [ :foo, JIT::Type::INT ],
        [ :bar, JIT::Type::VOID_PTR ],
        [ :baz, JIT::Type::FLOAT32 ])
    assert_equal [ :foo, :bar, :baz ], s_type.members
  end

  def test_create
    p = proc { |f|
      s_type = JIT::Struct.new(
          [ :foo, JIT::Type::INT ],
          [ :bar, JIT::Type::VOID_PTR ],
          [ :baz, JIT::Type::FLOAT32 ])
      s = s_type.create(f)
      f.return f.const(JIT::Type::INT, 0)
    }
    assert_function_result(
        :result => [ JIT::Type::INT, 0 ],
        &p)
  end

  def test_offset_of
    s_type = JIT::Struct.new(
        [ :foo, JIT::Type::INT ],
        [ :bar, JIT::Type::FLOAT64 ],
        [ :baz, JIT::Type::VOID_PTR ])
    assert_equal 0, s_type.offset_of(:foo)
    assert_equal 4, s_type.offset_of(:bar)
    assert_equal 12, s_type.offset_of(:baz)
  end

  def test_type_of
    s_type = JIT::Struct.new(
        [ :foo, JIT::Type::INT ],
        [ :bar, JIT::Type::FLOAT64 ],
        [ :baz, JIT::Type::VOID_PTR ])
    assert_equal JIT::Type::INT, s_type.type_of(:foo)
    assert_equal JIT::Type::FLOAT64, s_type.type_of(:bar)
    assert_equal JIT::Type::VOID_PTR, s_type.type_of(:baz)
  end

  def test_instance_bracket
    p = proc { |f|
      s_type = JIT::Struct.new(
          [ :foo, JIT::Type::INT ],
          [ :bar, JIT::Type::FLOAT64 ],
          [ :baz, JIT::Type::VOID_PTR ])
      s = s_type.create(f)
      f.insn_store_relative(s.ptr, 4, f.const(JIT::Type::FLOAT64, 42.0))
      f.return s[:bar]
    }
    assert_function_result(
        :result => [ JIT::Type::FLOAT64, 42.0 ],
        &p)
  end

  def test_instance_attrget
    p = proc { |f|
      s_type = JIT::Struct.new(
          [ :foo, JIT::Type::INT ],
          [ :bar, JIT::Type::FLOAT64 ],
          [ :baz, JIT::Type::VOID_PTR ])
      s = s_type.create(f)
      f.insn_store_relative(s.ptr, 4, f.const(JIT::Type::FLOAT64, 42.0))
      f.return s.bar
    }
    assert_function_result(
        :result => [ JIT::Type::FLOAT64, 42.0 ],
        &p)
  end

  def test_instance_bracket_eq
    p = proc { |f|
      s_type = JIT::Struct.new(
          [ :foo, JIT::Type::INT ],
          [ :bar, JIT::Type::FLOAT64 ],
          [ :baz, JIT::Type::VOID_PTR ])
      s = s_type.create(f)
      s[:bar] = f.const(JIT::Type::FLOAT64, 42.0)
      f.return s[:bar]
    }
    assert_function_result(
        :result => [ JIT::Type::FLOAT64, 42.0 ],
        &p)
  end

  def test_instance_attrset
    p = proc { |f|
      s_type = JIT::Struct.new(
          [ :foo, JIT::Type::INT ],
          [ :bar, JIT::Type::FLOAT64 ],
          [ :baz, JIT::Type::VOID_PTR ])
      s = s_type.create(f)
      s.bar = f.const(JIT::Type::FLOAT64, 42.0)
      f.return s.bar
    }
    assert_function_result(
        :result => [ JIT::Type::FLOAT64, 42.0 ],
        &p)
  end
end

