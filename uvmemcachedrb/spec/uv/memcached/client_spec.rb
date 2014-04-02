require 'spec_helper'

describe(UV::Memcached::Client) do
  let(:loop_t)    { double("uv_loop_t pointer") }
  let(:pool_size) { 5 }

  describe "#initialize" do
    expect(UV::Memcached).to receive(:fn_new).once.with(loop_t, pool_size)
    UV::Memcached::Client.new(loop_t, pool_size)
  end
end
