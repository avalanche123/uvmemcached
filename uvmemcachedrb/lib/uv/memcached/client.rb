module UV
  module Memcached
    class Client
      @@callbacks = Hash.new { |hash, object_id| hash[object_id] = Hash.new }

      class << self
        def destroy(object_id, ptr)
          Proc.new do
            self_p = FFI::MemoryPointer.new(:pointer)
            self_p.write_pointer(ptr)
            Memcached.fn_destroy(self_p)
            undefine_callbacks(object_id)
          end
        end

        def define_callback(object_id, name, callback)
          @@callbacks[object_id][name] ||= callback
        end

        def undefine_callbacks(object_id)
          @@callbacks.delete(object_id)
          nil
        end
      end

      def initialize(loop_t, pool_size = 5)
        @memcached_t = Memcached.fn_new(loop_t, pool_size)

        ObjectSpace.define_finalizer(self, Client.destroy(object_id, @memcached_t))
      end

      def connect(memcached_url, &block)
        assert_string(memcached_url, "connection endpoint must be a string")
        assert_format(/^(\w+)\:\/\/((\d+\.){3}\d+)\:(\d+)$/, memcached_url, "connection format must be <protocol>://<ip>:<port>")
        assert_not_null(block, "callback block is required")
        assert_artity(1, block, "callback must accept one argument")

        @connect_callback = block

        check_rc(Memcached.fn_connect(@memcached_t, memcached_url, nil, callback(:on_connect)), "unable to connect to memcached at #{memcached_url}")
      end

      def disconnect(&block)
        assert_not_null(block, "callback block is required")
        assert_artity(0, block, "callback must not accept any arguments")

        @disconnect_callback = block

        check_rc(Memcached.fn_disconnect(@memcached_t, nil, callback(:on_disconnect)), "unable to disconnect from memcached")
      end

      def get(key, &block)
        assert_string(key, "key must be a string")
        assert_not_null(block, "callback block is required")
        assert_artity(2, block, "callback must accept two arguments")

        @get_callback = block

        check_rc(Memcached.fn_get(@memcached, key, nil, callback(:on_get)), "unable to get #{key.inspect}")
      end

      def set(key, value, &block)
        assert_string(key, "key must be a string")
        assert_string(value, "value must be a string")
        assert_not_null(block, "callback block is required")
        assert_artity(1, block, "callback must accept one arguments")

        @set_callback = block

        check_rc(Memcached.fn_get(@memcached, key, value, nil, callback(:on_set)), "unable to set #{key.inspect}")
      end

      private

      def on_connect(memcached_t, status, context)
        @connect_callback.call(status != 0)
      end

      def on_disconnect(memcached_t, context)
        @disconnect_callback.call()
      end

      def on_get(memcached_t, status, value, context)
        @get_callback.call(status != 0, value)
      end

      def on_set(memcached_t, status, context)
        @set_callback.call(status != 0)
      end

      def callback(method_name)
        self.class.define_callback(object_id, method_name, method(method_name))
      end

      def assert_string(string, message)
        raise message unless string.is_a?(String)
      end

      def assert_format(regexp, string, message)
        raise message if regexp.match(string).nil?
      end

      def assert_not_null(object, message)
        raise message if object.nil?
      end

      def assert_artity(arity, callable, message)
        raise message unless callable.arity == arity
      end

      def check_rc(rc, message)
        raise message unless rc == 0
      end
    end
  end
end
