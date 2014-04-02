require 'spec_helper'

describe(UV::Memcached::Client) do
  let(:loop_t)    { double("uv_loop_t pointer") }
  let(:pool_size) { 5 }
  let(:client)    { UV::Memcached::Client.new(loop_t, pool_size) }
  let(:client_t)  { double("uv_memcached_t pointer") }

  describe "#initialize" do
    it "uses uv_memcached_new" do
      expect(UV::Memcached).to receive(:fn_new).once.with(loop_t, pool_size)
      client
    end
  end

  before(:each) do
    UV::Memcached.stub(:fn_new) { client_t }
  end

  describe "#connect" do
    let(:bad_connection_url) { "qwe abc" }
    let(:connection_url)     { "tcp://127.0.0.1:11211" }

    it "expects connection to be a string" do
      expect { client.connect(123) }.to raise_error("connection endpoint must be a string")
    end

    it "expects connection to be in <protocol>://<ip>:<port> format" do
      expect { client.connect(bad_connection_url) }.to raise_error("connection format must be <protocol>://<ip>:<port>")
    end

    it "expects a block" do
      expect { client.connect(connection_url) }.to raise_error("callback block is required")
    end

    it "expects a block with arity of one" do
      expect { client.connect(connection_url) { 'do nothing' } }.to raise_error("callback must accept one argument")
    end

    it "raises error if result is not 0" do
      expect(UV::Memcached).to receive(:fn_connect).once.with(client_t, connection_url, nil, client.callback(:on_connect)).and_return(-1)
      expect { client.connect(connection_url) { |e| 'do nothing' } }.to raise_error("unable to connect to memcached at tcp://127.0.0.1:11211")
    end

    it "succeeds if result is 0" do
      expect(UV::Memcached).to receive(:fn_connect).once.with(client_t, connection_url, nil, client.callback(:on_connect)).and_return(0)
      expect { client.connect(connection_url) { |e| 'do nothing' } }.to_not raise_error
    end
  end

  describe "#disconnect" do
    it "expects a block" do
      expect { client.disconnect }.to raise_error("callback block is required")
    end

    it "expects a block with arity of 0" do
      expect { client.disconnect {|e|} }.to raise_error("callback must not accept any arguments")
    end

    it "raises error if result is not 0" do
      expect(UV::Memcached).to receive(:fn_disconnect).once.with(client_t, nil, client.callback(:on_disconnect)).and_return(-1)
      expect { client.disconnect { 'do nothing' } }.to raise_error("unable to disconnect from memcached")
    end

    it "succeeds if result 0" do
      expect(UV::Memcached).to receive(:fn_disconnect).once.with(client_t, nil, client.callback(:on_disconnect)).and_return(0)
      expect { client.disconnect { 'do nothing' } }.to_not raise_error
    end
  end

  describe "#set" do
    let(:key)   { "mykey" }
    let(:value) { "my value" }

    it "expects key to be a string" do
      expect { client.set(123, 123) }.to raise_error("key must be a string")
    end

    it "expects value to be a string" do
      expect { client.set(key, 123) }.to raise_error("value must be a string")
    end

    it "expects a block" do
      expect { client.set(key, value) }.to raise_error("callback block is required")
    end

    it "expects a block with arity of 1" do
      expect { client.set(key, value) {} }.to raise_error("callback must accept one argument")
    end

    it "raises error if results is not 0" do
      expect(UV::Memcached).to receive(:fn_set).once.with(client_t, key, value, nil, client.callback(:on_set)).and_return(-1)
      expect { client.set(key, value) {|e|} }.to raise_error("unable to set #{key.inspect}")
    end

    it "succeeds if results is 0" do
      expect(UV::Memcached).to receive(:fn_set).once.with(client_t, key, value, nil, client.callback(:on_set)).and_return(0)
      expect { client.set(key, value) {|e|} }.to_not raise_error
    end
  end

  describe "#get" do
    let(:key)   { "mykey" }
    let(:value) { "my value" }

    it "expects key to be a string" do
      expect { client.get(123) }.to raise_error("key must be a string")
    end

    it "expects a block" do
      expect { client.get(key) }.to raise_error("callback block is required")
    end

    it "expects a block with arity of 2" do
      expect { client.get(key) {} }.to raise_error("callback must accept two arguments")
    end

    it "raises error if results is not 0" do
      expect(UV::Memcached).to receive(:fn_get).once.with(client_t, key, nil, client.callback(:on_get)).and_return(-1)
      expect { client.get(key) {|e, value|} }.to raise_error("unable to get #{key.inspect}")
    end

    it "succeeds if results is 0" do
      expect(UV::Memcached).to receive(:fn_get).once.with(client_t, key, nil, client.callback(:on_get)).and_return(0)
      expect { client.get(key) {|e, value|} }.to_not raise_error
    end
  end
end
