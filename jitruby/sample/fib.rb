require 'jit'

fib = nil
signature = JIT::Type.create_signature(
    :CDECL,
    :INT,
    [ :INT ])
fib = JIT::Function.build(signature) do |f|
  n = f.param(0)

  a = f.value(:INT, 0)
  b = f.value(:INT, 1)
  c = f.value(:INT, 1)

  i = f.value(:INT, 0)

  f.while{ i < n }.do {
    c.store(a + b)
    a.store(b)
    b.store(c)
    i.store(i + 1)
  }.end

  f.return(c)
end

values = (0...10).collect { |x| fib.apply(x) }
p values #=> [1, 1, 2, 3, 5, 8, 13, 21, 34, 55]

