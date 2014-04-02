Gem::Specification.new do |gem|
  gem.name          = "uvmemcachedrb"
  gem.version       = '0.1.0'
  gem.license       = 'MIT'
  gem.authors       = ["Bulat Shakirzyanov"]
  gem.email         = ["mallluhuct@gmail.com"]
  gem.homepage      = "http://avalanche123.github.com/uvmemcachedrb"
  gem.summary       = "libuvmemcached bindings for Ruby"
  gem.description   = "UV::Memcached is Ruby OOP bindings for libuvmemcached"

  gem.requirements << 'libuv'

  gem.required_ruby_version = '>= 1.9.2'
  gem.require_paths = ["lib"]

  gem.add_runtime_dependency     'uvrb',     '= 0.2.0'
  gem.add_runtime_dependency     'ffi',      '= 1.9.3'
  gem.add_development_dependency 'rspec',    '= 2.14.1'
  gem.add_development_dependency 'cucumber', '= 1.3.14'
  gem.add_development_dependency 'aruba',    '= 0.5.4'
  gem.add_development_dependency 'rake',     '= 10.2.2'

  gem.files = [
    'lib/uv/memcached.rb',
    'lib/uv/memcached/client.rb',
    'uvmemcachedrb.gemspec'
  ]
  gem.test_files = [
    'features/connection.feature',
    'features/get_and_set.feature',
    'features/support/env.rb',
    'features/support/step_definitions/steps.rb',
    'spec/spec_helper.rb',
    'spec/uv/memcached/client_spec.rb'
  ]
end