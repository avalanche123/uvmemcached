require 'bundler/setup'
require 'aruba/cucumber'
require 'rspec/expectations'
require 'uv/memcached'

Before do
  @aruba_timeout_seconds = 5
end
