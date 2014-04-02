Given(/^memcached service is( not)? running$/) do |stop|
  if stop
    `launchctl unload ~/Library/LaunchAgents/homebrew.mxcl.memcached.plist`
  else
    `launchctl load ~/Library/LaunchAgents/homebrew.mxcl.memcached.plist`
  end
end
