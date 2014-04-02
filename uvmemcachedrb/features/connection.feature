Feature: uvmemcached can connect to memcached server

  uvmemcached client needs to be able to connect to a memcached instance.
  It also needs to alert if a memcached instance is unreacheable

  Scenario: memcached server is unreacheable
    Given memcached service is not running
    And a file named "connect_failure.rb" with:
      """
      require 'uv'
      require 'uv/memcached'

      loop   = UV::Loop.default
      client = UV::Memcached.new(loop)

      client.connect("tcp://127.0.0.1:11211") do |e|
        abort "we're not supposed to be able to connect" unless e
      end

      loop.run
      """
    When I run `ruby connect_failure.rb`
    Then the exit status should be 0

  Scenario: connection to memcached server succeeds
    Given memcached service is running
    And a file named "connect_success.rb" with:
      """
      require 'uv'
      require 'uv/memcached'

      loop   = UV::Loop.default
      client = UV::Memcached.new(loop)

      client.connect("tcp://127.0.0.1:11211") do |e|
        abort "we're not supposed to be able to connect" if e
      end

      loop.run
      """
    When I run `ruby connect_success.rb`
    Then the exit status should be 0

  Scenario: disconnected memcached client attempts to disconnect
    Given memcached service is not running
    And a file named "not_connected_disconnect.rb" with:
      """
      require 'uv'
      require 'uv/memcached'

      loop   = UV::Loop.default
      client = UV::Memcached.new(loop)

      client.disconnect do
        abort "shouldn't ever run this callback"
      end

      loop.run
      """
    When I run `ruby not_connected_disconnect.rb`
    Then the exit status should be 0

  Scenario: memcached client is disconnected
    Given memcached service is running
    And a file named "connected_disconnect.rb" with:
      """
      require 'uv'
      require 'uv/memcached'

      loop   = UV::Loop.default
      client = UV::Memcached.new(loop)

      client.connect("tcp://127.0.0.1:11211") do |e|
        abort "connection must succeed" if e

        client.disconnect do
          exit 0
        end
      end

      loop.run

      abort "shouldn't get here"
      """
    When I run `ruby connected_disconnect.rb`
    Then the exit status should be 0
  