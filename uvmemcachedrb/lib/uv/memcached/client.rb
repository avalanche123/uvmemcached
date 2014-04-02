module UV
  module Memcached
    class Client
      def self.destroy(ptr)
        Proc.new do
          self_p = FFI::MemoryPointer.new(:pointer)
          self_p.write_pointer(ptr)
          Memcached.fn_destroy(self_p)
        end
      end

      def initialize(loop_t, pool_size = 5)
        @memcached_t = Memcached.fn_new(loop_t, pool_size)

        ObjectSpace.define_finalizer(self, Client.destroy(@memcached_t))
      end

      def connect(memcached_url, &block)
        @connect_callback = block

        Memcached.fn_connect(@memcached_t, memcached_url, nil, callback(:on_connect))
      end

      def disconnect(&block)
        @disconnect_callback = block

        Memcached.fn_disconnect(@memcached_t, nil, callback(:on_disconnect))
      end

      def get(key, &block)
        @get_callback = block

        Memcached.fn_get(@memcached, key, nil, callback(:on_get))
      end

      def set(key, value, &block)
        @set_callback = block

        Memcached.fn_get(@memcached, key, value, nil, callback(:on_set))
      end

      private

      def on_connect(memcached_t, status, context)
      end

      def on_disconnect(memcached_t, context)
      end

      def on_get(memcached_t, status, value, context)
      end

      def on_set(memcached_t, status, context)
      end

      def callback(method_name)
        instance_variable_set("@#{method_name}", method(method_name))
      end
    end
  end
end
