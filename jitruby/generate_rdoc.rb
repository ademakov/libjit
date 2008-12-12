require 'find'

def list_files(dir, pattern)
  arr = []
  Find.find(dir) do |filename|
    if filename =~ pattern then
      arr.push(filename)
    end
  end
  return arr
end

def generate_rdoc(*options)
  begin
    require 'rdoc/rdoc'
  rescue LoadError
    puts "WARNING: RDoc not installed; skipping generation of docs"
    return
  end

  r = RDoc::RDoc.new
  rdoc_files = []
  rdoc_files.concat [ 'README' ]
  rdoc_files.concat list_files('lib', /\.rb$/) if File.exist?('lib')
  rdoc_files.concat list_files('ext', /\.c$/)  if File.exist?('ext')
  rdoc_files.reject! { |file| file =~ %r{^ext/cached/} }
  r.document(options + rdoc_files)
end

if __FILE__ == $0 then
  generate_rdoc(*ARGV)
end

