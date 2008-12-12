require 'enumerator'

spec = Gem::Specification.new do |s|
  s.name = 'ruby-libjit'
  s.version = '0.1.0'
  s.summary = 'A wrapper for the libjit library'
  s.homepage = 'http://ruby-libjit.rubyforge.org'
  s.rubyforge_project = 'ruby-libjit'
  s.author = 'Paul Brannan'
  s.email = 'curlypaul924@gmail.com'

  s.description = <<-END
A wrapper for the libjit library
  END


  patterns = [
    'COPYING',
    'LGPL',
    'LICENSE',
    'README',
    'lib/*.rb',
    'lib/jit/*.rb',
    'ext/*.rb',
    'ext/*.c',
    'ext/*.h',
    'ext/*.rpp',
    'sample/*.rb',
  ]

  s.files = patterns.collect { |p| Dir.glob(p) }.flatten

  s.test_files = Dir.glob('test/test_*.rb')

  s.extensions = 'ext/extconf.rb'

  s.has_rdoc = true
end

