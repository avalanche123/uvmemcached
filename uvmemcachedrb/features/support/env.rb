require 'bundler/setup'
require 'aruba/cucumber'
require 'rspec/expectations'
require 'uv/memcached'

module Helper
  def start_memcached
    @pid ||= Process.spawn("memcached", :in=>"/dev/null", :out=>"/dev/null", :err=>"/dev/null")
  end

  def stop_memcached
    if @pid
      Process.kill("INT", @pid)
      Process.wait(@pid)
      @pid = nil
    end
  end
end

World(Helper)

Before do
  @aruba_timeout_seconds = 5
end

After do
  stop_memcached
end