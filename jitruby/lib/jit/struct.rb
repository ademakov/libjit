require 'jit'

module JIT

  # An abstraction for a record composed of heterogenous fields.
  #
  # Example usage:
  #
  #   point_type = JIT::Struct.new(
  #       [ :x, JIT::Type::INT ],
  #       [ :y, JIT::Type::INT ],
  #       [ :z, JIT::Type::INT ])
  #
  #   JIT::Context.build do |context|
  #     signature = JIT::Type.create_signature(
  #         JIT::ABI::CDECL, JIT::Type::INT, [ ])
  #
  #     function = JIT::Function.compile(context, signature) do |f|
  #       point = point_type.create(f)
  #       point.x = function.const(JIT::Type::INT, 1)
  #       point.y = function.const(JIT::Type::INT, 2)
  #       point.z = function.const(JIT::Type::INT, 3)
  #       f.insn_return(point.x)
  #     end
  #
  #   end
  #
  class Struct < JIT::Type

    # Construct a new JIT structure type.
    #
    # +members+:: A list of members, where each element in the list is a
    #             two-element array [ :name, type ]
    #
    def self.new(*members)
      member_names = members.map { |m| m[0].to_s.intern }
      member_types = members.map { |m| m[1] }
      type = self.create_struct(member_types)
      type.instance_eval do
        @members = members
        @member_names = member_names
        @member_types = member_types
        @index = Hash[*@member_names.zip((0..@member_names.size).to_a).flatten]
      end
      return type
    end

    # Return the names of the members in the structure.
    def members
      return @member_names
    end

    # Wrap an existing structure.
    #
    # +ptr+:: A pointer to the first element in the structure.
    #
    def wrap(ptr)
      return Instance.new(self, ptr)
    end

    # Create a new structure.
    #
    # +function+:: The JIT::Function this structure will be used in.
    #
    def create(function)
      instance = function.value(self)
      ptr = function.insn_address_of(instance)
      return wrap(ptr)
    end

    # Return the offset (in bytes) of the element with the given name.
    #
    # +name+:: The name of the desired element.
    #
    def offset_of(name)
      name = (Symbol === name) ? name : name.to_s.intern
      return self.get_offset(@index[name])
    end

    # Change the offset of the element with the given name.
    #
    # +name+::   The name of the desired element.
    # +offset+:: The new offset.
    #
    def set_offset_of(name, offset)
      name = (Symbol === name) ? name : name.to_s.intern
      return self.set_offset(@index[name], offset)
    end

    # Return the type of the element with the given name.
    #
    # +name+:: The name of the desired element.
    #
    def type_of(name)
      name = (Symbol === name) ? name : name.to_s.intern
      return @member_types[@index[name]]
    end

    # An abstraction for an instance of a JIT::Struct.
    #
    class Instance
      attr_reader :ptr

      # Wrap an existing structure.
      #
      # +struct+:: The JIT::Struct type to wrap.
      # +ptr+      A pointer to the first element of the structure.
      #
      def initialize(struct, ptr)
        @struct = struct
        @function = ptr.function
        @ptr = ptr

        mod = Module.new do
          struct.members.each do |name|
            define_method("#{name}") do
              self[name] # return
            end

            define_method("#{name}=") do |value|
              self[name] = value # return
            end
          end
        end

        extend(mod)
      end

      # Generate JIT code to retrieve the element with the given name.
      #
      # +name+:: The name of the desired element.
      #
      def [](name)
        @function.insn_load_relative(
            @ptr,
            @struct.offset_of(name),
            @struct.type_of(name))
      end

      # Generate JIT code to assign to the element with the given name.
      #
      # +name+::  The name of the desired element.
      # +value+:: The JIT::Value to assign to the element.
      #
      def []=(name, value)
        @function.insn_store_relative(
            @ptr,
            @struct.offset_of(name),
            value)
      end

      def members
        return @struct.members
      end
    end
  end
end

