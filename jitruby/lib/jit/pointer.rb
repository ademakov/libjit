require 'jit'
require 'jit/value'

module JIT

  # An abstraction for a pointer to a non-void type.
  #
  # Example usage:
  #
  # TODO
  #
  class Pointer < JIT::Type
    attr_reader :type

    # Create a new JIT pointer type.
    #
    # +type+::   The pointed-to type.
    #
    def self.new(type)
      pointer_type = self.create_pointer(type)
      pointer_type.instance_eval do
        @type = type
      end
      return pointer_type
    end

    # Wrap an existing void pointer.
    #
    # +ptr+:: The pointer to wrap.
    #
    def wrap(ptr)
      return Instance.wrap(self, ptr)
    end

    # Return the offset (in bytes) of the element at the given +index+.
    #
    # +index+:: The index of the desired element.
    #
    def offset_of(index)
      return index * @type.size
    end

    # Return the type of the element at the given +index+.
    #
    # +index+:: The index of the desired element.
    #
    def type_of(index)
      return @type
    end

    # An abstraction for a pointer object.
    #
    class Instance < JIT::Value
      # Wrap an existing void pointer.
      #
      # +array_type+:: The JIT::Array type to wrap.
      # +ptr+::        A pointer to the first element in the array.
      #
      def self.wrap(pointer_type, ptr)
        value = self.new_value(ptr.function, pointer_type)
        value.store(ptr)
        value.instance_eval do
          @pointer_type = pointer_type
          @pointed_type = pointer_type.type
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
            @pointer_type.offset_of(index),
            @pointer_type.type_of(index))
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
            @pointer_type.offset_of(index),
            value)
      end
    end
  end
end

