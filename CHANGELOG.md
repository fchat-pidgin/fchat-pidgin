# Change Log

## ?.?.?
### Added
- #55: New /uptime command : get server start date and some statistics
- #143: New /kinks command : list user custom kinks
### Fixed
- #130: Do not automatically try to re-login if kicked out from another location
- The user now gets a helpful error message instead of a popup when trying to send a message beyond server size limits
- #119: Bookmark updates from the website are now taken into account immediately
- #146: When a user is kicked or banned from a channel, the operator is now correctly reported to them

## 0.6.0
### Added
- #40: Character names in RTB notifications (reports, new notes, ...) are links to their profile
- #49: Add missing /setmode command
- /gban and /gunban now work on offline users
- #5: New option to sync status from Pidgin automatically (WARNING: on by default,  be wary of privacy implications when using multiple IM accounts at the same time)
- #52: Non-friends (or bookmarks) are now added to a temporary group when they IM the user, allowing to see their avatar and status info and to interact with them more easily
- #48: Add missing /setowner command
- Time durations for timeouts can now be given as specially formatted strings (e.g. instead of /ctimeout Nelwill, 1440 for a one day timeout, we can write /ctimeout Nelwill, 1d. Works up to weeks, e.g.: 1w3d12h24m)
- Clicking the character button in a conversation window now opens your own profile
- Add an "Alert Staff" button to every conversation
- Display a small information dialog (link to Pidgin channel, link to Bugtracker, ...) on the first login of an account. Can be re-enabled in account settings
- Users can now customize the background color of ads for better readability.
- #29: Character names are now colored according to their gender.
- /greports command to fetch pending chat reports (operators)
- "Receive Notifications" now also shows/hides broadcast messages and global op change notifications
- /report now can combine up to 6 past logs and offers a preview functionality. This is helpful when you accidentally close a tab before reporting or get disconnected.
- #122: The plugin now automatically checks for updates. This can be disabled in account options.

### Fixed
Summary :
- Fix SSL problems with the new Cloudflare setup of the site
- Removed the "Use Secure Connections" account option as this is now the only way to connect
- /showads, /hideads, /showchat, /hidechat now actually work
- Commands should now check for the correct permissions
- sub/sup-Tags now actually do something - maybe not perfectly but as perfect as it gets with Pidgin
- Make the red /warn-laser look more like the original - scarier!
- The "Position" field in character profiles now appears below "Dom/Sub-Role" (thanks Syldra Kitty)
- Fixed "Invalid JSON" errors when connecting multiple accounts
- Fixed icon loading
- Fixed a few memory leaks

Detail :
- #101: Fix flist_format_duration_str to work on 32-bit platforms
- #106: Buddy pounce messages are echoed back from your conversation partner
- #51: Can't compile on Fedora 22
- #60: Windows installer gives no opportunity to manually specify path to pidgin directory
- #65: When going away and returning, status is set as "(null)" if it was previously blank.
- #66: Pidgin auto-away sets f-list status as "busy"
- #67: Dice Rolls should count as messages, not events
- #68: Search is not working
- #69: Option to disable Has Connected/Has Disconnected
- #70: Implement a limit on non-kink searches
- #71: Windows installer disclaimer
- #73: Add /setmode and /getmode commands
- #74: Re-order "Position" field on profile info.
- #75: Support more HTML entities
- #76: Timestamps of a user's first message has wrong color
- #77: Own name colour resets after opening preferences window
- #78: Add toggle option for gender colouring applying to your own name
- #79: Apply gender colouring to private messages
- #82: Chat participants' looking status not visible on initial channel join
- #83: A trio of fixes for gender coloured names
- #84: Apply voice status to users on channel join
- #99: OTR shows encrypted text in the conversation window
- #53: Incorrect link in bug report comment notifications
- #88: /ignore command has poor feedback
- #87: Crash when closing ignore list dialog
- #97: /status should accept arbitrary case parameters
- #11: German umlauts in profile fields are not unescaped
- #90: [noparse] BBCode not functioning
- #98: Cloudflare is messing with reports
- #61: Avoid getting banned by multiple connection retries after password change. After a password change, you must now re-enable your account on pidgin
- #57: Show success notifications for /ctimeout and /ban
- #107: Having multiple accounts logged in causes text to be echoed twice
- #108: Build failing on GCC 4.9.1
- #109: Don't delete temporary IMs if a conversation window is still open
- #118: make fails on Arch

## 0.5.0
### Added
- #34: Add a command to display the plugin's version (/version)
- #36: Display disconnect notification for non-friend users in private conversations
- #37: Implement eicon support

### Fixed
- #30: Character name appears in conversations of different plugins
- #26: [url=http://...][/url] isn't parsed correctly
- #24: Using secure connection by default
- #33: Pidgin does not display channel timeouts correctly
- #50: /roll and /dice do not show their result when used in channels

### Updated
- #31: Updated GTK dependencies for Win32 cross compilation

## 0.4.3
### Added
- Added a changelog ;)
- #46: Allow /roll to be used in private conversations
- #45: Add support to the new Staff Alert functionality

### Fixed
- #47: When normal (non-operator) users send /warn, Pidgin displays them like real warnings
- #41: /who is missing colored gender names
- #38: /warn only gets formatted for the issuer
