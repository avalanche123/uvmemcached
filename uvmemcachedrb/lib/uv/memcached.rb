require 'ffi'

module UV
  module Memcached
    autoload :Client, 'uv/memcached/client'

    def self.new(*args)
      Client.new(*args)
    end

    begin
      LIB_PATHS = [
        '/usr/local/lib', '/opt/local/lib', '/usr/lib64'
      ].map{|path| "#{path}/libuvmemcached.#{FFI::Platform::LIBSUFFIX}"}

      libuvmemcached = ffi_lib(LIB_PATHS + %w{libuvmemcached}).first
    rescue LoadError
      warn <<-WARNING
        Unable to load this gem. The libuv library (or DLL) could not be found.
        If this is a Windows platform, make sure libuvmemcached.#{FFI::Platform::LIBSUFFIX} is on the PATH.
        For non-Windows platforms, make sure libuv is located in this search path:
        #{LIB_PATHS.join(":").inspect}
      WARNING
      exit 255
    end

    typedef :pointer, :uv_loop_t
    typedef :pointer, :uv_memcached_t

    callback :uv_memcached_connect_cb, [:uv_memcached_t, :int, :pointer], :void
    callback :uv_memcached_disconnect_cb, [:uv_memcached_t, :pointer], :void
    callback :uv_memcached_set_cb, [:uv_memcached_t, :int, :pointer], :void
    callback :uv_memcached_get_cb, [:uv_memcached_t, :int, :string, :pointer], :void

    attach_function :fn_new, :uv_memcached_new, [:uv_loop_t, :uint], :uv_memcached_t, :blocking => true
    attach_function :fn_destroy, :uv_memcached_destroy, [:pointer], :void, :blocking => true

    attach_function :fn_connect, :uv_memcached_connect, [:uv_memcached_t, :string, :pointer, :uv_memcached_connect_cb], :int, :blocking => true
    attach_function :fn_disconnect, :uv_memcached_disconnect, [:uv_memcached_t, :pointer, :uv_memcached_disconnect_cb], :int, :blocking => true

    attach_function :fn_set, :uv_memcached_set, [:uv_memcached_t, :string, :string, :pointer, :uv_memcached_set_cb], :int, :blocking => true
    attach_function :fn_get, :uv_memcached_get, [:uv_memcached_t, :string, :pointer, :uv_memcached_get_cb], :int, :blocking => true
  end
end