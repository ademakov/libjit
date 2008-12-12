require 'jit'

module JIT
  class Function
    # An abstraction for conditionals.
    #
    # Example usage:
    #
    #   function.if(condition) {
    #     # condition is true
    #   } .elsif(condition2) {
    #     # condition2 is true
    #   } .else {
    #     # condition1 and condition2 are false
    #   } .end
    #
    # Caution: if you omit end, then the generated code will have
    # undefined behavior, but there will be no warning generated.
    def if(cond, end_label = Label.new, &block)
      false_label = Label.new
      insn_branch_if_not(cond, false_label)
      block.call
      insn_branch(end_label)
      insn_label(false_label)
      return If.new(self, end_label)
    end

    # An abstraction for an inverted conditional.
    #
    # Example usage:
    #
    #   function.unless(condition) {
    #     # condition is false
    #   } .elsunless(condition2) {
    #     # condition2 is false
    #   } .else {
    #     # condition1 and condition2 are true
    #   } .end
    #
    # Caution: if you omit end, then the generated code will have
    # undefined behavior, but there will be no warning generated.
    def unless(cond, end_label = Label.new, &block)
      true_label = Label.new
      insn_branch_if(cond, true_label)
      block.call
      insn_branch(end_label)
      insn_label(true_label)
      return If.new(self, end_label)
    end

    class If # :nodoc:
      def initialize(function, end_label)
        @function = function
        @end_label = end_label
      end

      def else(&block)
        block.call
        return self
      end

      def elsif(cond, &block)
        return @function.if(cond, @end_label, &block)
      end

      def elsunless(cond, &block)
        return @function.unless(cond, @end_label, &block)
      end

      def end
        @function.insn_label(@end_label)
      end
    end

    # An abstraction for a multi-way conditional.
    #
    # Example usage:
    #
    #   function.case(value1)
    #   .when(value2) {
    #     # value1 == value2
    #   }.when(value3) {
    #     # value1 == value3
    #   } .else {
    #     # all other cases fell through
    #   } .end
    #
    # Caution: if you omit end, then the generated code will have
    # undefined behavior, but there will be no warning generated.
    def case(value)
      return Case.new(self, value)
    end

    class Case # :nodoc:
      def initialize(function, value)
        @function = function
        @value = value
        @if = nil
      end

      def when(value, &block)
        if not @if then
          @if = @function.if(value == @value, &block)
        else
          @if.elsif(value == @value, &block)
        end
        return self
      end

      def else(&block)
        @if.else(&block)
      end

      def end
        @if.end if @if
      end
    end

    # Usage:
    #
    #   until { <condition> }.do {
    #     # loop body
    #   } .end
    #
    def until(&block)
      start_label = Label.new
      done_label = Label.new
      insn_label(start_label)
      insn_branch_if(block.call, done_label)
      loop = Loop.new(self, start_label, done_label)
      return loop
    end

    # Usage:
    #
    #   while { <condition> }.do {
    #     # loop body
    #   } .end
    #
    def while(&block)
      start_label = Label.new
      done_label = Label.new
      insn_label(start_label)
      insn_branch_if_not(block.call, done_label)
      loop = Loop.new(self, start_label, done_label)
      return loop
    end

    class Loop # :nodoc:
      def initialize(function, start_label, done_label)
        @function = function
        @start_label = start_label
        @redo_label = start_label
        @done_label = done_label
      end

      def do(&block)
        block.call(self)
        return self
      end

      def end
        @function.insn_branch(@start_label)
        @function.insn_label(@done_label)
      end

      def break
        @function.insn_branch(@done_label)
      end

      def redo
        @function.insn_branch(@redo_label)
      end

      def redo_from_here
        @redo_label = JIT::Label.new
        @function.insn_label(@redo_label)
      end
    end

    # An alias for get_param
    def param(n)
      self.get_param(n)
    end

    # An alias for insn_return
    def return(result)
      self.insn_return(result)
    end

    # Create a JIT::Context and compile a new function within that
    # context.
    def self.build(*args, &block)
      JIT::Context.build do |context|
        JIT::Function.compile(context, *args, &block)
      end
    end
  end
end

