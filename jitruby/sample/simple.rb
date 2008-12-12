require 'jit'

function = nil
JIT::Context.build do |context|
  signature = JIT::Type.create_signature(
      JIT::ABI::CDECL,
      JIT::Type::INT,       # returns an integer
      [ JIT::Type::INT ])   # and tages an integer as a parameter
  function = JIT::Function.compile(context, signature) do |f|
    value = f.get_param(0)
    f.insn_return(value)
  end
end

p function.apply(42) #=> 42
