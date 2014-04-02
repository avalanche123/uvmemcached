Vagrant.configure("2") do |config|
  config.vm.box = "ubuntu/trusty"
  config.vm.box_url = "http://cloud-images.ubuntu.com/vagrant/trusty/current/trusty-server-cloudimg-i386-vagrant-disk1.box"
  config.vm.synced_folder ".", "/usr/local/src/libuvmemcached"

  config.vm.provision "shell", inline: <<-SHELL
echo "Installing all necessary packages"
sudo apt-get update
sudo apt-get install -y libtool autoconf automake libuv-dev memcached
echo "Compiling libuvmemcached"
cd /usr/local/src/libuvmemcached
./autogen.sh
./configure
make check
sudo make install
  SHELL
end
