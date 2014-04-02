Feature: uvmemcached connects to memcached server

  uvmemcached client needs to be able to connect to a memcached instance.
  It also needs to alert if a memcached instance is unreacheable

  Scenario: wake up an event loop from a different thread
    # Given memcached service is not running
    And a file named "connect_failure.rb" with:
      """
      require 'bundler/setup'
      require 'uv'
      require 'uv/memcached'

      puts "creating loop"
      loop   = UV::Loop.default
      puts "creating client"
      client = UV::Memcached.new(loop)

      puts "connecting to memcached"
      client.connect("tcp://127.0.0.1:11211") do |e|
        client.disconnect do
          abort "we're not supposed to be able to connect"
        end
      end
      puts "running loop"
      loop.run
      """
    When I run `ruby connect_failure.rb`
    Then the exit status should be 0