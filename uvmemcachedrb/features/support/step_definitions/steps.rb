Given(/^memcached service is( not)? running$/) do |stop|
  if stop
    stop_memcached
  else
    start_memcached
  end
end
