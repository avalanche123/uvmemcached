# Libuv memcached client

This repository contains:
 * libuvmemcached - a very basic async C memcached client implementation using
   libuv.
 * uvmemcachedrb  - ruby bindings to libuvmemcached using libffi.

## Getting started

install Vagrant from vagrantup.com website.

```shell
clone https://github.com/avalanche123/uvmemcached.git
cd uvmemcached
vagrant up
vagrant ssh
```

## Installing libuvmemcached

After you've ran all of the commands in Getting started section:

```shell
cd /usr/local/src/libuvmemcached
./autogen.sh
./configure
make
make check
sudo make install
```

## Installing uvmemcachedrb

After you've executed commands in Installing libuvmemcached section:

```shell
sudo gem install bundler --no-ri --no-rdoc
cd /usr/local/src/uvmemcachedrb
bundle install
bundle exec rspec
bundle exec cucumber
```
