require 'jit'
require 'jit/value'

module JIT

  # An abstraction for a fixed-length array type.
  #
  # Example usage:
  #
  #   array_type = JIT::Array.new(JIT::Type::INT, 4)
  #
  #   JIT::Context.build do |context|
  #     signature = JIT::Type.create_signature(
  #         JIT::ABI::CDECL, JIT::Type::INT, [ ])
  #
  #     function = JIT::Function.compile(context, signature) do |f|
  #       array_instance = array_type.create(f)
  #       array_instance[0] = f.const(JIT::Type::INT, 42)
  #       f.insn_return(array_instance[0])
  #     end
  #
  #   end
  #
  class Array < JIT::Type
    attr_reader :type
    attr_reader :length

    # Create a new JIT array type.
    #
    # +type+::   The type of the elements in the array.
    # +length+:: The number of elements in the array.
    #
    def self.new(type, length)
      array = self.create_struct([ type ] * length)
      array.instance_eval do
        @type = type
        @length = length
      end
      return array
    end

    # Wrap an existing array.
    #
    # +ptr+:: A pointer to the first element in the array.
    #
    def wrap(ptr)
      return Instance.wrap(self, ptr)
    end

    # Create a new array.
    #
    # +function+:: The JIT::Function this array will be used in.
    #
    def create(function)
      instance = function.value(self)
      ptr = function.insn_address_of(instance)
      return wrap(ptr)
    end

    # Return the offset (in bytes) of the element at the given +index+.
    #
    # +index+:: The index of the desired element.
    #
    def offset_of(index)
      return self.get_offset(index)
    end

    # Return the type of the element at the given +index+.
    #
    # +index+:: The index of the desired element.
    #
    def type_of(index)
      return @type
    end

    # An abstraction for an instance of a fixed-length array.
    #
    class Instance < JIT::Value
      attr_reader :array_type
      attr_reader :type

      # A pointer to the first element of the array.  Note that this
      # differs from +address+, which returns the address of a pointer
      # to the array.
      attr_reader :ptr

      # TODO: This breaks code below?
      # attr_reader :function

      # Wrap an existing array.
      #
      # +array_type+:: The JIT::Array type to wrap.
      # +ptr+::        A pointer to the first element in the array.
      #
      def self.wrap(array_type, ptr)
        pointer_type = JIT::Type.create_pointer(array_type)
        value = self.new_value(ptr.function, pointer_type)
        value.store(ptr)
        value.instance_eval do
          @array_type = array_type
          @type = array_type.type
          @function = ptr.function
          @ptr = ptr
        end
        return value
      end

      # Generate JIT code to retrieve the element at the given +index+.
      #
      # +index+:: The index of the desired element.  The value of the
      #           index must be known at compile-time.
      #
      def [](index)
        @function.insn_load_relative(
            @ptr,
            @array_type.offset_of(index),
            @array_type.type_of(index))
      end

      # Generate JIT code to assign to the element at the given +index+.
      #
      # +index+:: The index of the desired element.  The value of the
      #           index must be known at compile-time.
      # +value+:: The JIT::Value to assign to the element.
      #
      def []=(index, value)
        @function.insn_store_relative(
            @ptr,
            @array_type.offset_of(index),
            value)
      end
    end
  end
end

