module JitAssertions
  def assert_function_result(args, &block)
    result_type = nil
    expected_result = nil
    param_types = []
    params = []

    args.each do |k, v|
      case k.to_s
      when /^arg(\d+)$/
        n = $1.to_i
        param_types[n] = v[0]
        params[n] = v[1]
      when 'result'
        result_type = v[0]
        expected_result = v[1]
      else
        raise "Bad arg #{arg}"
      end
    end

    function = nil
    JIT::Context.build do |context|
      signature = JIT::Type.create_signature(
          JIT::ABI::CDECL,
          result_type,
          param_types)
      function = JIT::Function.compile(context, signature, &block)
    end

    assert_equal(expected_result, function.apply(*params))
  end
end

