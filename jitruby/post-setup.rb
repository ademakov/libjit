require 'run_tests'
require 'generate_rdoc'

if config('without-tests') != 'yes' then
  run_tests()
end

