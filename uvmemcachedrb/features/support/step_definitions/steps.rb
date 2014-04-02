Given(/^memcached service is not running$/) do
  `launchctl unload ~/Library/LaunchAgents/homebrew.mxcl.memcached.plist`
end
