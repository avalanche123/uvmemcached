Feature: uvmemcached can set and get values in memcached

  uvmemcached client needs to be able to set and get values from a memcached instance.

  Scenario: uvmemcached sets a value
    Given memcached service is running
    And a file named "get_and_set.rb" with:
      """
      require 'uv'
      require 'uv/memcached'

      loop   = UV::Loop.default
      client = UV::Memcached.new(loop)

      client.connect("tcp://127.0.0.1:11211") do |e|
        abort "we're supposed to be able to connect" if e

        client.set('mykey', 'myvalue') do |e|
          abort "unexpected failure to set a value" if e

          client.get('mykey') do |e, value|
            abort "unexpected failure to get a value" if e
            abort "value returned from memcached is wrong" unless value == 'myvalue'

            client.disconnect do
              # nothing
            end
          end
        end
      end

      loop.run
      """
    When I run `ruby get_and_set.rb`
    Then the exit status should be 0
