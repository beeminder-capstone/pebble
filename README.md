# Pebble
[Beeminder](https://www.beeminder.com/) Watchapp for [Pebble](https://www.pebble.com/)

Copyright © 2017 The [PSU](https://www.pdx.edu/) [CS](https://www.pdx.edu/computer-science/) Beeminder [Capstone](http://wiki.cs.pdx.edu/capstone/fall_2016/fall_2016.html) Team

Made and maintained by: [Teal Dulcet](https://github.com/tdulcet)

This watchapp automatically gets Pebble Health data from your Pebble Smartwatch every night at 11:59PM and adds it to one of your Beeminder Goals. It works with these Pebble Health metrics:
* Step Count
* Active Time (minutes)
* Sleep Time (minutes)
* Distance Walked (Meters)
* Calories Burned (kcal)

The watchapp automatically opens to send the data to Beeminder and you will receive a message when it is successful. Otherwise, it can store up to two weeks (14 days) of data in case your phone is off or you do not have an internet connection.

The watchapp displays:
* The current value of your selected Pebble Health metric with respect to the rate (per day) of your selected Beeminder goal.
* The summary of what you need to do reach your goal.
* Your road status color for the value/rate above (on [Pebble's with a color screen](https://developer.pebble.com/guides/tools-and-resources/hardware-information/)).

## Links
* [https://www.beeminder.com/apps/authorize?client_id=b4t29518iw7a2weh97wt5wcbj&redirect_uri=https://www.beeminder.com/pebble/beeminder.html&response_type=token](https://www.beeminder.com/apps/authorize?client_id=b4t29518iw7a2weh97wt5wcbj&redirect_uri=https://www.beeminder.com/pebble/beeminder.html&response_type=token)
* [https://github.com/beeminder/pebblebee](https://github.com/beeminder/pebblebee)
* [https://developer.pebble.com/](https://developer.pebble.com/)
* [https://developer.pebble.com/guides/events-and-services/health/](https://developer.pebble.com/guides/events-and-services/health/)
* [https://developer.pebble.com/blog/2016/03/07/Take-the-Pebble-Health-API-in-your-Stride/](https://developer.pebble.com/blog/2016/03/07/Take-the-Pebble-Health-API-in-your-Stride/)
* [https://github.com/pebble/slate](https://github.com/pebble/slate)
* [https://www.beeminder.com/api](https://www.beeminder.com/api)
