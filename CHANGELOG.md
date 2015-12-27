# Change Log

## ?.?.?
### Added
- #40: Character names in RTB notifications (reports, new notes, ...) are links to their profile
- #49: Add missing /setmode command
- /gban and /gunban now work on offline users
- #5: New option to sync status from Pidgin automatically (WARNING: on by default,  be wary of privacy implications when using multiple IM accounts at the same time)
- #61: Non-friends (or bookmarks) are now added to a temporary group when they IM the user, allowing to see their avatar and status info and to interact with them more easily
- #48: Add missing /setowner command
- Time durations for timeouts can now be given as specially formatted strings (e.g. instead of /ctimeout Nelwill, 1440 for a one day timeout, we can write /ctimeout Nelwill, 1d. Works up to weeks, e.g.: 1w3d12h24m)
- Clicking the character button in a conversation window now opens your own profile
- Add an "Alert Staff" button to every conversation
- Display a small information dialog (link to Pidgin channel, link to Bugtracker, ...) on the first login of an account. Can be re-enabled in account settings

### Fixed
- #53: Incorrect link in bug report comment notifications
- /showads, /hideads, /showchat, /hidechat now actually work
- Commands should now check for the correct permissions
- #61: Avoid getting banned by multiple connection retries after password change. After a password change, you must now re-enable your account on pidgin
- #57: Show success notifications for /ctimeout and /ban
- sub/sup-Tags now actually do something - maybe not perfectly but as perfect as it gets with Pidgin

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
