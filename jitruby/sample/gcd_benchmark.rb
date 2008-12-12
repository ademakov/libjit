require 'jit'
require 'benchmark'

# GCD, JIT-compiled

jit_gcd = nil

JIT::Context.build do |context|
  signature = JIT::Type.create_signature(
      JIT::ABI::CDECL,
      JIT::Type::INT,
      [ JIT::Type::INT, JIT::Type::INT ])
  jit_gcd = JIT::Function.compile(context, signature) do |f|
    x = f.get_param(0)
    y = f.get_param(1)
    temp1 = f.insn_eq(x, y)
    label1 = JIT::Label.new
    f.insn_branch_if_not(temp1, label1)
    f.insn_return(x)
    f.insn_label(label1)
    temp2 = f.insn_lt(x, y)
    label2 = JIT::Label.new
    f.insn_branch_if_not(temp2, label2)
    s1 = f.insn_sub(y, x)
    temp3 = f.insn_call("gcd", f, 0, x, s1)
    f.insn_return(temp3)
    f.insn_label(label2)
    s2 = f.insn_sub(x, y)
    temp4 = f.insn_call("gcd", f, 0, s2, y)
    f.insn_return(temp4)

    f.optimization_level = 3
  end
end

if jit_gcd.apply(28, 21) != 7 then
  puts "jit_gcd is broken"
  exit 1
end


# GCD with tail recursion optimization

jit_gcd_tail = nil

JIT::Context.build do |context|
  signature = JIT::Type.create_signature(
      JIT::ABI::CDECL,
      JIT::Type::INT,
      [ JIT::Type::INT, JIT::Type::INT ])
  jit_gcd_tail = JIT::Function.compile(context, signature) do |f|
    x = f.get_param(0)
    y = f.get_param(1)
    temp1 = f.insn_eq(x, y)
    label1 = JIT::Label.new
    f.insn_branch_if_not(temp1, label1)
    f.insn_return(x)
    f.insn_label(label1)
    temp2 = f.insn_lt(x, y)
    label2 = JIT::Label.new
    f.insn_branch_if_not(temp2, label2)
    s1 = f.insn_sub(y, x)
    temp3 = f.insn_call("gcd", f, JIT::Call::TAIL, x, s1)
    # f.insn_return(temp3)
    f.insn_label(label2)
    s2 = f.insn_sub(x, y)
    temp4 = f.insn_call("gcd", f, JIT::Call::TAIL, s2, x)
    # f.insn_return(temp4)

    f.optimization_level = 3
  end
end

if jit_gcd_tail.apply(28, 21) != 7 then
  puts "jit_gcd_tail is broken"
  exit 1
end


# GCD in ruby with recursion

def gcd(x, y)
  if x == y
    return x
  elsif x < y
    return gcd(x, y - x)
  else
    return gcd(x - y, y)
  end
end


# GCD in ruby without recursion

def gcd2(x, y)
  while x != y do
    if x < y
      y -= x
    else
      x -= y
    end
  end
  return x
end

N = 1000

X = 1000
Y = 1005

Benchmark.bm(16) do |x|
  x.report("jit")            { N.times { jit_gcd.apply(X, Y) } }
  x.report("jit tail:")      { N.times { jit_gcd_tail.apply(X, Y) } }
  x.report("ruby recur:")    { N.times { gcd(X, Y) } }
  x.report("ruby iter:")     { N.times { gcd2(X, Y) } }
end

